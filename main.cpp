#include <iostream>
#include <thread>
#include "sk_transmitter/internal_msg.hpp"
#include "sk_transmitter/types.hpp"
#include "sk_transmitter/lockable_queue.hpp"
#include "sk_transmitter/lockable_cache.hpp"
#include "sk_transmitter/sk_socket.hpp"
//#include "sk_transmitter/sk_socket.hpp"


// TODO: This should be set based on cmd line args
size_t PSIZE = 0;
size_t FSIZE = 0;
size_t RTIME = 0;
std::string test_broadcast_ip = "255.255.255.255";
uint16_t test_broadcast_port = 40000;


sk_transmitter::lockable_queue awaiting_msgs;
sk_transmitter::lockable_cache sent_msgs(FSIZE);

namespace sk_workers {
    void transmit() {
        sk_transmitter::msg_t buf(PSIZE);  // TODO: Set appropriate size of read chunk

        while (!std::cin.eof()) {
            std::cin.read(reinterpret_cast<char *>(buf.data()), buf.size());  // or `&buf[0]` on older platforms
            sk_transmitter::internal_msg msg(true, buf);
            awaiting_msgs.atomic_push(msg);
        }
    }


    void retransmit() {
        while (true) {  // TODO: Stop condition
            sk_transmitter::msg_id_t id;  // TODO: Read from socket

            sk_transmitter::internal_msg msg = sent_msgs.atomic_get(id);
            if (msg.id == id) {  // message with desired id was still stored in cache
                awaiting_msgs.atomic_push(msg);
            }

            // TODO: Sleep EXACTLY 250 milis
        }
    }


    void listen() {
        while (true) {  // TODO: Stop condition
            // TODO
        }
    }


    void send() {
        sk_transmitter::msg_id_t session_id;  // TODO: Initialize as specified
        sk_transmitter::sk_socket sock{test_broadcast_ip, static_cast<in_port_t>(test_broadcast_port)};

        while (true) {  // TODO: Stop condition
            sk_transmitter::internal_msg msg = awaiting_msgs.atomic_get_and_pop();

            sock.send(msg.sendable_with_session_id(session_id));

            if (msg.initial) {
                sent_msgs.atomic_push(msg);
            }
        }
    }

}

int main() {
    // TODO: Parse command line args

    std::thread sender(sk_workers::send);
    std::thread listener(sk_workers::listen);
    std::thread retransmitter(sk_workers::retransmit);
    std::thread transmitter(sk_workers::transmit);
    return 0;
}
