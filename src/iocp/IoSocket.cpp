#include "IoSocket.h"
#include <assert.h>

namespace net
{
    LPFN_CONNECTEX getConnectEx(SOCKET fd)
    {
        GUID guid = WSAID_CONNECTEX;
        LPFN_CONNECTEX ptr = NULL;
        DWORD bytes = 0;
        if (::WSAIoctl(fd, SIO_GET_EXTENSION_FUNCTION_POINTER,
            &guid, sizeof(guid), &ptr, sizeof(ptr), &bytes, 0, 0) != 0)
        {
            WRITE_LOG("getConnectEx()->WSAIoctl() SIO_GET_EXTENSION_FUNCTION_POINTER error! errno:%d\n", WSAGetLastError());
        }

        return ptr;
    }
    LPFN_ACCEPTEX  getAcceptEx(SOCKET fd)
    {
        GUID guid = WSAID_ACCEPTEX;
        LPFN_ACCEPTEX ptr = NULL;
        DWORD bytes = 0;
        if (::WSAIoctl(fd, SIO_GET_EXTENSION_FUNCTION_POINTER,
            &guid, sizeof(guid), &ptr, sizeof(ptr), &bytes, 0, 0) != 0)
        {
            WRITE_LOG("getAcceptEx()->WSAIoctl() SIO_GET_EXTENSION_FUNCTION_POINTER error! errno:%d\n", WSAGetLastError());
        }

        return ptr;
    }

    LPFN_GETACCEPTEXSOCKADDRS getAcceptExAddrs(SOCKET fd)
    {
        GUID guid = WSAID_GETACCEPTEXSOCKADDRS;

        LPFN_GETACCEPTEXSOCKADDRS ptr = NULL;
        DWORD bytes = 0;
        if (0 != WSAIoctl(fd, SIO_GET_EXTENSION_FUNCTION_POINTER,
            &guid, sizeof(guid), &ptr, sizeof(ptr), &bytes, NULL, NULL) != 0)
        {
            WRITE_LOG("getAcceptExAddrs()->WSAIoctl() SIO_GET_EXTENSION_FUNCTION_POINTER error! errno:%d\n", WSAGetLastError());
        }

        return ptr;
    }


    IoSocket::IoSocket() :
        _fd(INVALID_SOCKET)
    {
        _connHandle._type = IocpHandle::eIH_CONNECT;
        memset(&_connHandle._overlapped, 0, sizeof(OVERLAPPED));

        _recvHandle._type = IocpHandle::eIH_READ;
        memset(&_recvHandle._overlapped, 0, sizeof(OVERLAPPED));

        _writeHandle._type = IocpHandle::eIH_WRITE;
        memset(&_writeHandle._overlapped, 0, sizeof(OVERLAPPED));
    }

    IoSocket::~IoSocket()
    {
        close();
    }

    bool IoSocket::createIoPort(NetLoop* loop, SOCKET fd)
    {
        ULONG_PTR key = 0;
        if (CreateIoCompletionPort((HANDLE)fd, loop->io(), (ULONG_PTR)key, 1) == NULL)
        {
            WRITE_LOG("CreateIoCompletionPort() error! errno:%d", GetLastError());
            return false;
        }

        _fd = fd;
        return true;
    }

    bool IoSocket::connect(NetLoop* loop, const char* ip, unsigned short port)
    {
        _ipaddr = IPAddres(ip, port);
        int type = (_ipaddr.isIpv6() ? AF_INET6 : AF_INET);
        SOCKET fd = WSASocket(type, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);

        if (fd == INVALID_SOCKET)
        {
            WRITE_LOG("WSASocket() error! errno:%d\n", WSAGetLastError());
            return false;
        }

        {// bind
            IPAddres localaddr(_ipaddr.isIpv6());
            if (net::bind(fd, localaddr) != 0)
            {
                closesocket(fd);
                WRITE_LOG("WSASocket() error! errno:%d\n", WSAGetLastError());
                return false;
            }
        }

        bool ret = createIoPort(loop, fd);
        if (!ret) {
            closesocket(fd);
            return ret;
        }

        {//环回快速路径
            int OptionValue = 1;
            DWORD NumberOfBytesReturned = 0;
            DWORD SIO_LOOPBACK_FAST_PATH_A = 0x98000010;

            WSAIoctl(
                fd,
                SIO_LOOPBACK_FAST_PATH_A,
                &OptionValue,
                sizeof(OptionValue),
                NULL,
                0,
                &NumberOfBytesReturned,
                0,
                0);
        }

        return reconnect();
    }

    bool IoSocket::reconnect()
    {
        DWORD dwBytes = 0;
        LPFN_CONNECTEX lpConnectEx = getConnectEx(_fd);
        assert(lpConnectEx);
        if (!lpConnectEx(_fd, _ipaddr, _ipaddr.addrSizeof(), NULL, 0, &dwBytes, &_connHandle._overlapped))
        {
            if (WSAGetLastError() != ERROR_IO_PENDING)
            {
                close();
                WRITE_LOG("lpConnectEx() failed errno!=ERROR_IO_PENDING, errno:%d\n", WSAGetLastError());
                return false;
            }
        }
        _connHandle.sockptr = shared_from_this();
        
        return true;
    }

    bool IoSocket::write(char* buf, int len)
    {
        _writeBuf.buf = buf;
        _writeBuf.len = len;

        DWORD dwBytes = 0;
        //post send
        if (WSASend(_fd, &_writeBuf, 1, &dwBytes, 0, &_writeHandle._overlapped, NULL) != 0)
        {
            if (WSAGetLastError() != WSA_IO_PENDING)
            {
                WRITE_LOG("WSASend() failed errno!=WSA_IO_PENDING, errno:%d\n", WSAGetLastError());
                _writeBuf.buf = nullptr;
                _writeBuf.len = 0;
                return false;
            }
        }

        _writeHandle.sockptr = shared_from_this();
        return true;
    }

    bool IoSocket::read(char* buf, int len)
    {
        _recvBuf.buf = buf;
        _recvBuf.len = len;

        DWORD dwBytes = 0;
        DWORD dwFlag = 0;
        //post recv
        if (WSARecv(_fd, &_recvBuf, 1, &dwBytes, &dwFlag, &_recvHandle._overlapped, NULL) != 0)
        {
            if (WSAGetLastError() != WSA_IO_PENDING)
            {
                WRITE_LOG("WSARecv() failed errno!=WSA_IO_PENDING, errno:%d\n", WSAGetLastError());
                _recvBuf.buf = nullptr;
                _recvBuf.len = 0;
                return false;
            }
        }

        _recvHandle.sockptr = shared_from_this();
        return true;
    }

    void IoSocket::onIocpMsg(int type, int bytes, int err)
    {
        if (type == IocpHandle::eIH_CONNECT)
        {
            if (!err) {
                //solve: [shutdown() errno == WSAENOTCONN]
                setsockopt(_fd,
                    SOL_SOCKET,
                    SO_UPDATE_CONNECT_CONTEXT,
                    NULL,
                    0);
            }
            onConnect(err);
        }
        else if (type == IocpHandle::eIH_READ)
        {
            onRead(bytes, err);
        }
        else if (type == IocpHandle::eIH_WRITE)
        {
            onWrite(bytes, err);
        }
    }

    void IoSocket::close()
    {
        if (_fd != INVALID_SOCKET)
        {
            //shutdown(_fd, SD_BOTH);
            //CancelIo((HANDLE)_fd);
            closesocket(_fd);
            _fd = INVALID_SOCKET;
        }
    }
}//namespace