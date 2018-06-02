#include <iostream>
#include <thread>
#include <future>
#include "sk_transmitter/internal_msg.hpp"
#include "sk_transmitter/types.hpp"
#include "sk_transmitter/lockable_queue.hpp"
#include "sk_transmitter/lockable_cache.hpp"
#include "sk_transmitter/sk_socket.hpp"

using nonstd::optional;  // TODO: Change in deployment


// TODO: This should be set based on cmd line args
size_t PSIZE = 512;
size_t FSIZE = 65536;
size_t RTIME = 0;
std::string MCAST_ADDR = "255.255.255.255";
uint16_t DATA_PORT = 40000;

const size_t sent_msgs_cache_size = 1 + ((FSIZE - 1) / PSIZE);  // FSIZE and PSIZE > 0

sk_transmitter::lockable_queue awaiting_msgs;
sk_transmitter::lockable_cache sent_msgs(sent_msgs_cache_size);

namespace sk_workers {
    void transmit() {
        sk_transmitter::msg_t buf(PSIZE);
        sk_transmitter::msg_id_t current_msg_id = 0;

        while (!std::cin.eof()) {
            std::cin.read(reinterpret_cast<char *>(buf.data()), buf.size());  // or `&buf[0]` on older platforms
            sk_transmitter::internal_msg msg(true, current_msg_id, buf);
            current_msg_id++;

            awaiting_msgs.atomic_push(msg); // TODO: Make sure last chunk will be omitted
        }
    }


    void retransmit(std::shared_future<void> sending_completed) {
        while (sending_completed.wait_for(std::chrono::milliseconds(1)) == std::future_status::timeout) {
            auto wake_up_time = std::chrono::system_clock::now().time_since_epoch();
            sk_transmitter::msg_id_t id;  // TODO: Read from socket

            optional<sk_transmitter::internal_msg> msg = sent_msgs.atomic_get(id);
            if (msg.has_value()) {
                if (msg.value().id == id) {  // message with desired id was still stored in cache
                    awaiting_msgs.atomic_push(msg.value());
                }
            }

            // Sleep EXACTLY 250 milis
            auto time_diff = std::chrono::system_clock::now().time_since_epoch() - wake_up_time;
            std::this_thread::sleep_for(std::chrono::milliseconds(250)-time_diff);
            // TODO: Send this to another socket opened on same port
        }
    }


    void listen(std::shared_future<void> sending_completed) {
        while (sending_completed.wait_for(std::chrono::milliseconds(1)) == std::future_status::timeout) {
            // TODO
        }
    }


    void send(std::shared_future<void> sending_completed) {
        sk_transmitter::msg_id_t session_id = 1;  // TODO: Initialize as specified
        sk_transmitter::sk_socket sock{MCAST_ADDR, static_cast<in_port_t>(DATA_PORT)};

        while (sending_completed.wait_for(std::chrono::milliseconds(1)) == std::future_status::timeout) {
            // even though this is blocking, deadlock is impossible: setting sending_completed flag means there is something in the queue.
            optional<sk_transmitter::internal_msg> msg = awaiting_msgs.atomic_get_and_pop();
            if (msg.has_value()) {
                sock.send(msg.value().sendable_with_session_id(session_id));

                if (msg.value().initial) {
                    sent_msgs.atomic_push(msg.value());
                }
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
