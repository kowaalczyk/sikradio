#include <iostream>
#include <thread>
#include <future>
#include "sk_transmitter/internal_msg.hpp"
#include "sk_transmitter/types.hpp"
#include "sk_transmitter/lockable_queue.hpp"
#include "sk_transmitter/lockable_cache.hpp"
#include "sk_transmitter/sk_socket.hpp"


// TODO: This should be set based on cmd line args
size_t PSIZE = 512;
size_t FSIZE = 65536;
size_t RTIME = 0;
std::string MCAST_ADDR = "255.255.255.255";
uint16_t DATA_PORT = 40000;


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


    void retransmit(std::shared_future<void> sending_completed) {
        while (sending_completed.wait_for(std::chrono::milliseconds(1)) == std::future_status::timeout) {
            sk_transmitter::msg_id_t id;  // TODO: Read from socket

            sk_transmitter::internal_msg msg = sent_msgs.atomic_get(id);
            if (msg.id == id) {  // message with desired id was still stored in cache
                awaiting_msgs.atomic_push(msg);
            }

            // TODO: Sleep EXACTLY 250 milis
        }
    }


    void listen(std::shared_future<void> sending_completed) {
        while (sending_completed.wait_for(std::chrono::milliseconds(1)) == std::future_status::timeout) {
            // TODO
        }
    }


    void send(std::shared_future<void> sending_completed) {
        sk_transmitter::msg_id_t session_id;  // TODO: Initialize as specified
        sk_transmitter::sk_socket sock{MCAST_ADDR, static_cast<in_port_t>(DATA_PORT)};

        while (sending_completed.wait_for(std::chrono::milliseconds(1)) == std::future_status::timeout) {
            sk_transmitter::internal_msg msg = awaiting_msgs.atomic_get_and_pop();  // TODO: Prevent deadlock here (when sending_completed but atomic_get waiting)
            sock.send(msg.sendable_with_session_id(session_id));

            if (msg.initial) {
                sent_msgs.atomic_push(msg);
            }
        }

    }

}

int main() {
    // TODO: Parse command line args

    std::promise<void> sending_completed;
    std::shared_future<void> sc_future(sending_completed.get_future());

    std::thread sender(sk_workers::send, sc_future);
    std::thread listener(sk_workers::listen, sc_future);
    std::thread retransmitter(sk_workers::retransmit, sc_future);
    std::thread transmitter(sk_workers::transmit);

    transmitter.join();
    sending_completed.set_value();

    listener.join();
    retransmitter.join();
    sender.join();

    return 0;
}
