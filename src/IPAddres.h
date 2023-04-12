#pragma once
#include "Platform.h"

namespace net {
    extern bool is_ipv4(const char* host);
    extern bool is_ipv6(const char* host);
    inline bool is_ipaddr(const char* host) {
        return is_ipv4(host) || is_ipv6(host);
    }

    class IPAddres
    {
    public:
        IPAddres(bool isv6 = false);
        IPAddres(const char * addr, uint16_t port);
        IPAddres(const sockaddr* addr);
        IPAddres(const IPAddres& addr);

        void operator = (const sockaddr* addr);
        void operator = (const IPAddres& addr);

        inline void setIpv6(bool b)
        {
            mbIpv6 = b;
        }

        inline bool isIpv6()const
        {
            return mbIpv6;
        }

        char const* getAddr(void) const;
        uint16_t    getPort(void) const;

        operator sockaddr& ()
        {
            return mbIpv6 ? *(sockaddr*)&mAddr6 : *(sockaddr*)&mAddr4;
        }

        operator const sockaddr& (void) const
        {
            return mbIpv6 ? *(sockaddr*)&mAddr6 : *(sockaddr*)&mAddr4;
        }

        operator sockaddr* ()
        {
            return mbIpv6 ? (sockaddr*)&mAddr6 : (sockaddr*)&mAddr4;
        }

        operator const sockaddr* (void) const
        {
            return mbIpv6 ? (sockaddr*)&mAddr6 : (sockaddr*)&mAddr4;
        }

        int addrSizeof() const
        {
            return mbIpv6 ? sizeof(sockaddr_in6) : sizeof(sockaddr_in);
        }

    private:
        bool mbIpv6 = false;
        union
        {
            sockaddr_in     mAddr4;
            sockaddr_in6    mAddr6;
        };
    };
}