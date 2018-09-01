#ifndef SIKRADIO_COMMON_ADDRESS_HELPERS_HPP
#define SIKRADIO_COMMON_ADDRESS_HELPERS_HPP

#include <arpa/inet.h>
#include <netinet/in.h>
#include "exceptions.hpp"

namespace sikradio::common {
    using address_exception = sikradio::common::exceptions::address_exception;

    sockaddr_in make_address(const std::string& addr, in_port_t port) {
        // std::cerr << "'" << addr << "'" << std::endl;

        struct sockaddr_in ret{
            .sin_family=AF_INET,
            .sin_port=htons(port)
        };
        int err = inet_aton(addr.data(), &ret.sin_addr);
        if (err == 0) throw address_exception(addr);
        return ret;
    }
}

#endif
