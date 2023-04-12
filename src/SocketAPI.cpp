#include "SocketAPI.h"
#include "IPAddres.h"

namespace net
{

    bool is_ipv4(const char* host) {
        struct sockaddr_in sin;
        return inet_pton(AF_INET, host, &sin) == 1;
    }

    bool is_ipv6(const char* host) {
        struct sockaddr_in6 sin6;
        return inet_pton(AF_INET6, host, &sin6) == 1;
    }

    IPAddres::IPAddres(bool isv6):
        mbIpv6(isv6)
    {
        memset(&mAddr6, 0, sizeof(mAddr6));
        if (isv6)
            mAddr6.sin6_family = AF_INET6;
        else
            mAddr4.sin_family = AF_INET;
    }

    IPAddres::IPAddres(const char* addr, uint16_t port)
    {
        struct addrinfo* addrinfo_result = NULL;
        int error_result = getaddrinfo(addr, NULL, NULL, &addrinfo_result);
        if (0 != error_result)
        {
            if(addrinfo_result) freeaddrinfo(addrinfo_result);
            return;
        }
        if (addrinfo_result->ai_family == AF_INET6)
        {
            mbIpv6 = true;

            //             ::inet_ntop(mAddr6.sin6_family, &(((struct sockaddr_in6*)addrinfo_result)->sin6_addr), (char*)addr.c_str(), INET6_ADDRSTRLEN);
            //             ::inet_pton(mAddr6.sin6_family, addr.c_str(), &mAddr6.sin6_addr);
            memcpy(&mAddr6, addrinfo_result->ai_addr, addrinfo_result->ai_addrlen);
            //mAddr6.sin6_family = addrinfo_result->ai_family;
            mAddr6.sin6_port = htons(port);
        }
        else
        {
            mbIpv6 = false;
            memcpy(&mAddr4, addrinfo_result->ai_addr, addrinfo_result->ai_addrlen);
            //             mAddr4.sin_addr.s_addr = inet_addr(addr.c_str());          
            //             if (mAddr4.sin_addr.s_addr == -1)
            //             {
            //                 hostent* ht = gethostbyname(addr.c_str());
            //                 mAddr4.sin_addr.s_addr = (*reinterpret_cast<uint32_t*>(ht->h_addr_list[0]));             
            //             }
                        //mAddr4.sin_family = addrinfo_result->ai_family;
            mAddr4.sin_port = htons(port);
        }
    }

    IPAddres::IPAddres(const IPAddres& addr)
    {
        *this = addr;
    }

    IPAddres::IPAddres(const sockaddr* addr)
    {
        *this = addr;
    }

    void IPAddres::operator = (const sockaddr* addr)
    {
        if (addr->sa_family == AF_INET6)
        {
            mbIpv6 = true;
            mAddr6 = *(sockaddr_in6*)addr;
        }
        else
        {
            mbIpv6 = false;
            mAddr4 = *(sockaddr_in*)addr;
        }
    }

    void IPAddres::operator = (const IPAddres& addr)
    {
        mbIpv6 = addr.mbIpv6;
        mAddr4 = addr.mAddr4;
        mAddr6 = addr.mAddr6;
    }

    char const* IPAddres::getAddr(void) const
    {
        static char addrbuff[INET6_ADDRSTRLEN];

        if (mbIpv6)
        {
            ::inet_ntop(mAddr6.sin6_family, &mAddr6.sin6_addr, (char*)addrbuff, INET6_ADDRSTRLEN);
        }
        else
        {
            ::inet_ntop(mAddr4.sin_family, &mAddr4.sin_addr, (char*)addrbuff, INET_ADDRSTRLEN);
        }
        return addrbuff;
    }

    uint16_t IPAddres::getPort(void) const
    {
        if (mbIpv6)
        {
            return ntohs(mAddr6.sin6_port);
        }
        else
        {
            return ntohs(mAddr4.sin_port);
        }
    }


    int bind(SOCKET fd, IPAddres const& addr)
    {
        if (addr.isIpv6())
        {
            return ::bind(fd, &(sockaddr const&)addr, INET6_ADDRSTRLEN);
        }
        else
        {
            return ::bind(fd, &(sockaddr const&)addr, INET_ADDRSTRLEN);
        }
    }

    int listen(SOCKET fd, int backlog)
    {
        return ::listen(fd, backlog);
    }

    SOCKET accept(SOCKET fd, IPAddres& addr)
    {
#ifdef WIN_OS
        int len = addr.addrSizeof();
#else
        uint32 len = addr.addrSizeof();
#endif
        return ::accept(fd, addr, &len);
    }

    int connect(SOCKET fd, IPAddres addr)
    {
        if (addr.isIpv6())
        {
            return ::connect(fd, addr, INET6_ADDRSTRLEN);
        }
        else
        {
            return ::connect(fd, addr, INET_ADDRSTRLEN);
        }
    }
}