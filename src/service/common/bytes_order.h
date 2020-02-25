#pragma once

#include <cstdint>
#ifdef WIN32
// Need to come before any Windows.h includes
#include <Winsock2.h>
#elif defined(__linux__) || defined(MAC_OSX)
#include <netinet/in.h>
#include <arpa/inet.h>
#endif
#include <stdexcept>
#include "util.h"


#if !defined(_DARWIN_C_SOURCE)

inline uint64_t htonll(uint64_t val)
{
    if (endian_util::get() == endian_util::little_endian)
    {
        return (((unsigned long long)htonl((int)((val << 32) >> 32))) << 32) | (unsigned int)htonl((int)(val >> 32));
    }
    else if (endian_util::get() == endian_util::big_endian)
    {
        return val;
    }
    else
    {
        throw std::runtime_error("endian type error");
    }
}

inline uint64_t ntohll(uint64_t val)
{
    if (endian_util::get() == endian_util::little_endian)
    {
        return (((unsigned long long)ntohl((int)((val << 32) >> 32))) << 32) | (unsigned int)ntohl((int)(val >> 32));
    }
    else if (endian_util::get() == endian_util::big_endian)
    {
        return val;
    }
    else
    {
        throw std::runtime_error("endian type error");
    }
}
#endif

class bytes_order
{
public:

    static uint16_t hton16(uint16_t x) { return htons(x); }
    static uint32_t hton32(uint32_t x) { return htonl(x); }
    static uint64_t hton64(uint64_t x) { return htonll(x); }
    static uint16_t ntoh16(uint16_t x) { return ntohs(x); }
    static uint32_t ntoh32(uint32_t x) { return ntohl(x); }
    static uint64_t ntoh64(uint64_t x) { return ntohll(x); }
};