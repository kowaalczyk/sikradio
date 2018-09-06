#ifndef SIKRADIO_RECEIVER_DATA_SOCKET_HPP
#define SIKRADIO_RECEIVER_DATA_SOCKET_HPP

#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <mutex>
#include <string>
#include <optional>

#include "../common/exceptions.hpp"
#include "../common/address_helpers.hpp"
#include "../common/data_msg.hpp"
#include "structures.hpp"

#ifndef UDP_DATAGRAM_DATA_LEN_MAX
#define UDP_DATAGRAM_DATA_LEN_MAX 65535
#endif

namespace sikradio::receiver {
    using socket_exception = sikradio::common::exceptions::socket_exception;

    class data_socket {
    private:
        std::mutex mut{};
        sikradio::receiver::structures::station connected_station;
        sikradio::common::byte_t buffer[UDP_DATAGRAM_DATA_LEN_MAX];
        int timeout_in_ms;
        int sock = -1;

        void close_and_throw() {
            close(sock);
            throw socket_exception(strerror(errno));
        }

    public:
        data_socket() : timeout_in_ms{5000} {};
        data_socket(const data_socket& other) = delete;
        data_socket(data_socket&& other) = delete;
        explicit data_socket(int socket_timeout_in_ms) : 
            timeout_in_ms{socket_timeout_in_ms} {}

        void connect(const sikradio::receiver::structures::station new_station) {
            std::scoped_lock{mut};
            if (new_station == connected_station) return;
            // if socket was opened, reconnect
            if (sock >= 0) close(sock);
            sock = socket(AF_INET, SOCK_DGRAM, 0);
            if (sock < 0) throw socket_exception(strerror(errno));
            // subscribe to multicast address
            struct sockaddr_in new_addr = sikradio::common::make_address(
                new_station.data_address, 
                new_station.data_port);
            struct ip_mreq req;
            req.imr_multiaddr = new_addr.sin_addr;
            int err = setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void*)&req, sizeof(req));
            if (err < 0) close_and_throw();
            // set timeout to 1 second to prevent deadlocks (possible with optional returns)
            struct timeval tv{ .tv_sec = 0, .tv_usec = 1000*timeout_in_ms};
            err = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
            if (err < 0) close_and_throw();
            // bind socket to new address
            err = bind(sock, (struct sockaddr*)&new_addr, sizeof(new_addr));
            if (err < 0) close_and_throw();
            connected_station = new_station;
        }

        std::optional<sikradio::common::data_msg> try_read() {
            if (sock == -1) return std::nullopt;

            memset(buffer, 0, UDP_DATAGRAM_DATA_LEN_MAX);
            ssize_t len = read(sock, &buffer, sizeof(buffer));
            if (len < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {  
                    // timeout reached, this is not really an error
                    errno = 0;
                    return std::nullopt;
                } else {
                    throw socket_exception(strerror(errno));
                }
            }
            sikradio::common::msg_t raw_msg;
            raw_msg.assign(buffer, buffer+len);
            return std::make_optional(sikradio::common::data_msg(raw_msg));
        }

        ~data_socket() {
            if (sock >= 0) close(sock);
        }
    };
}

#endif
