//
// Created by kowal on 02.06.18.
//

#ifndef SIK_NADAJNIK_CTRL_SOCKET_HPP
#define SIK_NADAJNIK_CTRL_SOCKET_HPP


#include <sys/socket.h>
#include <cstring>
#include <utility>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <zconf.h>
#include <iostream>
#include "exceptions.hpp"
#include "../lib/optional.hpp"
#include "ctrl_msg.hpp"


#define UDP_DATAGRAM_DATA_LEN_MAX 65535


using nonstd::optional;
using nonstd::nullopt;


namespace sk_transmitter {
    class ctrl_socket {
    private:
        sockaddr_in local_addr{};
        bool set_up{false};
        int sock{-1};

        in_port_t local_port;

        void setup_socket() {
            if (set_up) return;

            // standard socket setup
            sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
            if (sock < 0) throw sk_transmitter::exceptions::socket_exception(strerror(errno));

            // listen on broadcast
            int broadcast = 1;
            int err = setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
            if (err < 0) throw sk_transmitter::exceptions::socket_exception(strerror(errno));

            // set timeout to 1 second to prevent deadlocks (possible with optional returns)
            struct timeval tv{
                    .tv_sec = 1,
                    .tv_usec = 0
            };
            err = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
            if (err < 0) throw sk_transmitter::exceptions::socket_exception(strerror(errno));

            // bind socket
            local_addr.sin_family = AF_INET;
            local_addr.sin_port = htons(local_port);
            local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
            err = bind(sock, (struct sockaddr *) &local_addr, sizeof(local_addr));
            if (err < 0) throw sk_transmitter::exceptions::socket_exception(strerror(errno));

            set_up = true;
        }

    public:
        explicit ctrl_socket(in_port_t local_port) : local_port(local_port) {}

        optional<sk_transmitter::ctrl_msg> receive() {
            if (!set_up) setup_socket();

            struct sockaddr_in client_address{};
            auto rcva_len = (socklen_t) sizeof(client_address);
            char buffer[UDP_DATAGRAM_DATA_LEN_MAX];

            ssize_t len = recvfrom(sock, (void *) buffer, sizeof(buffer), 0, (struct sockaddr *) &client_address,
                                   &rcva_len);
            if (len < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {  // timeout reached, this is not really an error
                    errno = 0;
                    return nullopt;
                } else {
                    throw sk_transmitter::exceptions::socket_exception(strerror(errno));
                }
            }
            std::string str_buffer(buffer, buffer+len);
            return optional<sk_transmitter::ctrl_msg>(sk_transmitter::ctrl_msg(str_buffer, client_address));
        }

        void respond(const ctrl_msg &request, const char *response, size_t response_len) {
            sockaddr_in addr = request.get_sender_address();

            ssize_t snd_len = sendto(sock, response, response_len, 0,
                                     reinterpret_cast<const sockaddr *>(&addr),
                                     (socklen_t) sizeof(request.get_sender_address()));
            if (snd_len < 0) throw sk_transmitter::exceptions::socket_exception(strerror(errno));
            if (snd_len != response_len)
                throw sk_transmitter::exceptions::socket_exception("Failed to send entire response");
        }
    };
}


#endif //SIK_NADAJNIK_CTRL_SOCKET_HPP
