#ifndef SIKRADIO_COMMON_CTRL_SOCKET_HPP
#define SIKRADIO_COMMON_CTRL_SOCKET_HPP

#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <mutex>
#include <string>
#include <tuple>
#include <optional>

#include "exceptions.hpp"
#include "../common/ctrl_msg.hpp"

#ifndef TTL_VALUE
#define TTL_VALUE 64
#endif

#ifndef UDP_DATAGRAM_DATA_LEN_MAX
#define UDP_DATAGRAM_DATA_LEN_MAX 65535
#endif

using socket_exception = sikradio::common::exceptions::socket_exception;

namespace sikradio::common {
    class ctrl_socket {
    private:
        // simultaneous reading and writing to same socket is generally possible
        std::mutex read_mut{};
        std::mutex write_mut{};
        int sock = -1;

        void close_and_throw() {
            close(sock);
            throw socket_exception(strerror(errno));
        }

    public:
        ctrl_socket() = delete;
        ctrl_socket(const ctrl_socket& other) = delete;
        ctrl_socket(ctrl_socket &&other) = delete;

        explicit ctrl_socket(
                in_port_t local_port, 
                bool enable_broadcast=false, 
                bool bind_local=true) {
            sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
            if (sock < 0) throw socket_exception(strerror(errno));
            // reuse address if necessary
            int enable = 1;
            int err = setsockopt(
                sock, 
                SOL_SOCKET, 
                SO_REUSEADDR, 
                &enable, 
                sizeof(enable));
            if (err < 0) close_and_throw();
            // set timeout to prevent deadlocks
            struct timeval tv{
                    .tv_sec = 1,
                    .tv_usec = 0
            };
            err = setsockopt(
                sock, 
                SOL_SOCKET, 
                SO_RCVTIMEO, 
                &tv, 
                sizeof(tv));
            if (err < 0) close_and_throw();
            if (enable_broadcast) {
                // enable broadcast
                int broadcast = 1;
                err = setsockopt(
                    sock, 
                    SOL_SOCKET, 
                    SO_BROADCAST, 
                    &broadcast, 
                    sizeof(broadcast));
                if (err < 0) close_and_throw();
                // set TTL value
                int ttl = TTL_VALUE;
                err = setsockopt(
                    sock,
                    IPPROTO_IP,
                    IP_MULTICAST_TTL,
                    &ttl,
                    sizeof(ttl)
                );
                if (err < 0) close_and_throw();
            }
            if (bind_local) {
                // bind socket to local port
                struct sockaddr_in local_addr;
                local_addr.sin_family = AF_INET;
                local_addr.sin_port = htons(local_port);
                local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
                err = bind(sock, (struct sockaddr *) &local_addr, sizeof(local_addr));
                if (err < 0) close_and_throw();
            }
        }

        std::optional<std::tuple<sikradio::common::ctrl_msg, struct sockaddr_in>> try_read() {
            std::scoped_lock{read_mut};
            struct sockaddr_in sender_address{};
            auto rcva_len = (socklen_t) sizeof(sender_address);
            char buffer[UDP_DATAGRAM_DATA_LEN_MAX];  // TODO: Allocate once
            memset(buffer, 0, UDP_DATAGRAM_DATA_LEN_MAX);
            ssize_t len = recvfrom(
                sock, 
                (void *) buffer, 
                sizeof(buffer), 
                0, 
                (struct sockaddr *) &sender_address,
                &rcva_len);
            if (len < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {  
                    // timeout reached, this is not really an error
                    errno = 0;
                    return std::nullopt;
                } else {
                    throw socket_exception(strerror(errno));
                }
            }
            sikradio::common::ctrl_msg msg(std::string(buffer, buffer+len));
            return std::make_optional(std::make_tuple(msg, sender_address));
        }

        void send_to(const struct sockaddr_in& destination, sikradio::common::ctrl_msg msg) {
            std::scoped_lock{write_mut};
            size_t data_len = msg.sendable().size();
            ssize_t snd_len = sendto(
                sock, 
                msg.sendable().data(), 
                data_len, 
                0,
                reinterpret_cast<const sockaddr *>(&destination),
                (socklen_t) sizeof(destination));
            if (snd_len < 0) throw socket_exception(strerror(errno));
            if (snd_len != static_cast<ssize_t>(data_len)) 
                throw socket_exception("Failed to send entire response");
        }

        void force_send_to(const struct sockaddr_in& destination, sikradio::common::ctrl_msg msg) {
            while (true) {
                try {
                    send_to(destination, msg);
                    break;
                } catch (socket_exception &e) {
                    // retry
                }
            }
        }

        ~ctrl_socket() {
            if (sock >= 0) close(sock);
        }
    };
}


#endif
