#ifndef SIKRADIO_SENDER_SK_SOCKET_HPP
#define SIKRADIO_SENDER_SK_SOCKET_HPP


#include <netinet/in.h>
#include <cstdlib>
#include <string>
#include <utility>
#include <zconf.h>
#include <cstring>
#include <arpa/inet.h>

#include "../common/data_msg.hpp"
#include "../common/exceptions.hpp"


#define TTL_VALUE 64


using socket_exception = sikradio::common::exceptions::socket_exception;


namespace sikradio::sender {
    class data_socket {
    private:
        std::string remote_dotted_address;
        in_port_t remote_port;

        bool connected{false};
        int sock{-1};

        void open_connection() {
            if (connected) return;

            sock = socket(AF_INET, SOCK_DGRAM, 0);
            if (sock < 0) throw socket_exception(strerror(errno));

            int optval = 1;
            int err = setsockopt(
                sock, 
                SOL_SOCKET, 
                SO_BROADCAST, 
                (void *) &optval, 
                sizeof optval);
            if (err < 0) throw socket_exception(strerror(errno));

            optval = TTL_VALUE;
            err = setsockopt(
                sock, 
                IPPROTO_IP, 
                IP_MULTICAST_TTL, 
                (void *) &optval, 
                sizeof optval);
            if (err < 0) throw socket_exception(strerror(errno));

            struct sockaddr_in remote_address{
                    .sin_family = AF_INET,
                    .sin_port = htons(remote_port)
            };
            err = inet_aton(remote_dotted_address.c_str(), &remote_address.sin_addr);
            if (err == 0) throw socket_exception("Invalid address");

            err = connect(sock, (struct sockaddr *) &remote_address, sizeof(remote_address));
            if (err < 0) throw socket_exception(strerror(errno));

            connected = true;
        }

    public:
        data_socket(
            std::string remote_dotted_address, 
            in_port_t remote_port) : remote_dotted_address(std::move(remote_dotted_address)), 
                                     remote_port(remote_port) {}

        void transmit(sikradio::common::msg_t sendable_msg) {
            if (!connected) open_connection();

            auto msg_len = sendable_msg.size();
            auto sent_len = write(sock, sendable_msg.data(), msg_len);
            if (sent_len != static_cast<ssize_t>(msg_len)) throw socket_exception(strerror(errno));
        }

        void transmit_force(sikradio::common::msg_t sendable_msg) {
            while (true) {
                try {
                    transmit(sendable_msg);
                    break;
                } catch (socket_exception &e) {
                    // retry
                }
            }
        }

        void close_connection() {
            if (!connected) return;

            close(sock);
        }

        ~data_socket() {
            close_connection();
        }
    };
}


#endif //SIKRADIO_SENDER_SK_SOCKET_HPP
