#pragma once

#include "NetLoop.h"
#include "IoSocket.h"
#include <cassert>
#include "Accept.h"

namespace net {

#if 0
    inline HANDLE poll_add(HANDLE iocp, int fd, ULONG_PTR key)
    {
        return CreateIoCompletionPort((HANDLE)fd, iocp, key, 0);
    }

    inline bool poll_del(HANDLE iocp, int fd)
    {
        return CancelIo((HANDLE)fd);
    }

    void post_iocp(HANDLE iocp)
    {
        s_iocp_context* user = new s_iocp_context;
        PostQueuedCompletionStatus(iocp, 0, IOCP_POST_USER_DATA, (LPOVERLAPPED)user);
    }
#endif

    NetLoop::NetLoop():
        _io(NULL)
    {
 
    }
    NetLoop::~NetLoop()
    {
        destroy();
    }

    bool NetLoop::init()
    {
        assert(_io == NULL);

        _io = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
        if (!_io) {
            
            return false;
        }

        return true;
    }

    void NetLoop::destroy()
    {
        if (_io) {
            CloseHandle(_io);
            _io = NULL;
        }
    }

    void NetLoop::post(void* data)
    {
        if (_io) {
            PostQueuedCompletionStatus(_io, 0, POST_USER_TYPE, (LPOVERLAPPED)data);
        }
    }

    int NetLoop::run(int timeout)
    {
        DWORD bytes = 0;
        ULONG_PTR key = 0;
        LPOVERLAPPED povlp = NULL;
        BOOL bRet = GetQueuedCompletionStatus(_io, &bytes, &key, &povlp, timeout);
        int err = ERROR_SUCCESS;
        if (povlp == NULL) {
            err = WSAGetLastError();
            if (err == WAIT_TIMEOUT || err == ERROR_NETNAME_DELETED || err == ERROR_OPERATION_ABORTED) {
                return 0;
            }
            return err;
        }

        if (bRet == FALSE) {
            err = WSAGetLastError();
        }

        if (key == POST_USER_TYPE) {
            PostBase* async = (PostBase*)povlp;
            if (async) {
                async->call();
                delete async;
            }
            return err;
        }

        IocpHandle * handle = CONTAINING_RECORD(povlp, IocpHandle, _overlapped);
        switch (handle->_type)
        {
        case IocpHandle::eIH_CONNECT:
        case IocpHandle::eIH_READ:
        case IocpHandle::eIH_WRITE:
        {
            IocpHandleContext* context = (IocpHandleContext*)handle;
            if (context->sockptr) {
                //Reference Count
                IoSockptr tmpptr(std::move(context->sockptr));
                tmpptr->onIocpMsg(handle->_type, bytes, err);
            }
        }
            break;
        case IocpHandle::eIH_ACCPET:
        {
            IocpHandleAccept* accept = (IocpHandleAccept*)handle;
            if (accept->accept) {
                accept->accept->onIocpAccept(err);
            }
        }
            break;
        }

        return err;
    }
}//namespace