#include <iostream>
#include <thread>
#include <future>
#include "sk_transmitter/data_msg.hpp"
#include "sk_transmitter/types.hpp"
#include "sk_transmitter/lockable_queue.hpp"
#include "sk_transmitter/lockable_cache.hpp"
#include "sk_transmitter/data_socket.hpp"
#include "sk_transmitter/ctrl_msg.hpp"
#include "sk_transmitter/ctrl_socket.hpp"

using nonstd::optional;  // TODO: Change in deployment


// TODO: This should be set based on cmd line args
size_t PSIZE = 512;
size_t FSIZE = 65536;
size_t RTIME = 250;
std::string MCAST_ADDR = "239.10.11.12";
uint16_t DATA_PORT = 40000;
uint16_t CTRL_PORT = 30000;
sk_transmitter::msg_id_t session_id = 1;  // TODO: Initialize as specified

const size_t sent_msgs_cache_size = 1 + ((FSIZE - 1) / PSIZE);  // FSIZE and PSIZE > 0

sk_transmitter::lockable_queue send_q;
sk_transmitter::lockable_queue resend_q;
sk_transmitter::lockable_cache sent_msgs(sent_msgs_cache_size);

namespace sk_workers {
    void read() {
        sk_transmitter::msg_t buf(PSIZE);
        sk_transmitter::msg_id_t current_msg_id = 0;

        while (!std::cin.eof()) {
            std::cin.read(reinterpret_cast<char *>(buf.data()), buf.size());  // or `&buf[0]` on older platforms
            sk_transmitter::data_msg msg(current_msg_id, buf);
            current_msg_id++;

            send_q.atomic_push(msg); // TODO: Make sure last chunk will be omitted
            sent_msgs.atomic_push(msg);
        }
    }


    void listen(std::shared_future<void> reading_complete) {
        sk_transmitter::ctrl_socket sock{CTRL_PORT};

        while (reading_complete.wait_for(std::chrono::milliseconds(1)) == std::future_status::timeout) {
            optional<sk_transmitter::ctrl_msg> req  = sock.receive();
            if (req.has_value()) {

                if (req.value().is_lookup()) {
                    // TODO: Fill response with correct data
                    sock.respond(req.value(), "", sizeof(""));
                }

                if (req.value().is_rexmit()) {
                    // msg_ids are present (in case there are no ids, vector will be empty but never null)
                    std::vector<sk_transmitter::msg_id_t> msg_ids = req.value().get_rexmit_ids().value();
                    for (auto id : msg_ids) {
                        optional<sk_transmitter::data_msg> msg = sent_msgs.atomic_get(id);
                        if (msg.has_value()) {
                            if (msg.value().id == id) {  // message with desired id was still stored in cache
                                resend_q.atomic_push(msg.value());
                            }
                        }
                    }
                }
            }
        }
    }


    void resend(std::shared_future<void> reading_complete) {
        sk_transmitter::data_socket sock{MCAST_ADDR, static_cast<in_port_t>(DATA_PORT)};

        while (reading_complete.wait_for(std::chrono::milliseconds(1)) == std::future_status::timeout) {
            optional<sk_transmitter::data_msg> msg = resend_q.atomic_get_and_pop();
            while (msg.has_value()) {  // Send all messages from the queue
                sock.send(msg.value().sendable_with_session_id(session_id));

                msg = resend_q.atomic_get_and_pop();
            }

            // Assuming RTIME is a time between retransmissions (not between beinnnings of consecutive retransmissions)
            std::this_thread::sleep_for(std::chrono::milliseconds(RTIME));
        }
    }


    void send(std::shared_future<void> reading_complete) {
        sk_transmitter::data_socket sock{MCAST_ADDR, static_cast<in_port_t>(DATA_PORT)};

        while (reading_complete.wait_for(std::chrono::milliseconds(1)) == std::future_status::timeout) {
            optional<sk_transmitter::data_msg> msg = send_q.atomic_get_and_pop();

            if (msg.has_value()) {
                sock.send(msg.value().sendable_with_session_id(session_id));
            }
        }
    }

}

int main() {
    // TODO: Parse command line args

    std::promise<void> reading_complete;
    std::shared_future<void> sc_future(reading_complete.get_future());

    std::thread sender(sk_workers::send, sc_future);
    std::thread listener(sk_workers::listen, sc_future);
    std::thread retransmitter(sk_workers::resend, sc_future);
    std::thread transmitter(sk_workers::read);

    transmitter.join();
    reading_complete.set_value();

    listener.join();
    retransmitter.join();
    sender.join();

    return 0;
}
