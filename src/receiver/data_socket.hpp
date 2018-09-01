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
#include "../common/data_msg.hpp"

#ifndef UDP_DATAGRAM_DATA_LEN_MAX
#define UDP_DATAGRAM_DATA_LEN_MAX 65535
#endif

using socket_exception = sikradio::common::exceptions::socket_exception;

namespace sikradio::receiver {
    class data_socket {
    private:
        std::mutex mut{};
        std::string multicast_dotted_address{};
        in_port_t local_port;
        int sock = -1;

        void close_and_throw() {
            close(sock);
            throw socket_exception(strerror(errno));
        }

    public:
        data_socket() = default;
        data_socket(const data_socket& other) = delete;
        data_socket(data_socket&& other) = delete;

        explicit data_socket(in_port_t local_port) : local_port{local_port} {}

        void connect(const std::string& multicast_dotted_address) {
            std::scoped_lock{mut};
            if (multicast_dotted_address == this->multicast_dotted_address && sock >= 0) return;
            this->multicast_dotted_address = multicast_dotted_address;
            // if socket was opened, reconnect
            if (sock >= 0) close(sock);
            sock = socket(AF_INET, SOCK_DGRAM, 0);
            if (sock < 0) throw socket_exception(strerror(errno));
            // subscribe to multicast address
            struct ip_mreq ip_mreq;
            ip_mreq.imr_interface.s_addr = htonl(INADDR_ANY);
            int err = inet_aton(multicast_dotted_address.data(), &ip_mreq.imr_multiaddr);
            if (err == 0) close_and_throw();
            err = setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void*)&ip_mreq, sizeof(ip_mreq));
            if (err < 0) close_and_throw();
            // set timeout to 1 second to prevent deadlocks (possible with optional returns)
            struct timeval tv{ .tv_sec = 1, .tv_usec = 0};
            err = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
            if (err < 0) close_and_throw();
            // bind socket to local port
            struct sockaddr_in local_address;
            local_address.sin_family = AF_INET;
            local_address.sin_addr.s_addr = htonl(INADDR_ANY);
            local_address.sin_port = htons(local_port);
            err = bind(sock, (struct sockaddr*)&local_address, sizeof(local_address));
            if (err < 0) close_and_throw();
        }

        std::optional<sikradio::common::data_msg> try_read() {
            if (sock == -1) return std::nullopt;

            sikradio::common::byte_t buffer[UDP_DATAGRAM_DATA_LEN_MAX];  // TODO: Allocate once
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
            raw_msg.assign(buffer, buffer+sizeof(buffer));
            return std::make_optional(sikradio::common::data_msg(raw_msg));
        }

        ~data_socket() {
            if (sock >= 0) close(sock);
        }
    };
}

#endif