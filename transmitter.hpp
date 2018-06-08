//
// Created by kowal on 08.06.18.
//

#ifndef SIK_NADAJNIK_TRANSMITTER_HPP
#define SIK_NADAJNIK_TRANSMITTER_HPP

#include <cstdint>
#include <iostream>
#include <future>
#include <utility>
#include "sk_transmitter/types.hpp"
#include "sk_transmitter/data_msg.hpp"
#include "sk_transmitter/data_socket.hpp"
#include "sk_transmitter/lockable_cache.hpp"
#include "sk_transmitter/lockable_queue.hpp"
#include "sk_transmitter/ctrl_socket.hpp"

namespace sk_transmitter {
    class transmitter {
    private:
        size_t PSIZE = 512;
        size_t FSIZE = 65536;
        size_t RTIME = 250;
        std::string MCAST_ADDR = "239.10.11.12";
        uint16_t DATA_PORT = 25830;
        uint16_t CTRL_PORT = 35830;
        std::string NAME = "";

        sk_transmitter::msg_id_t session_id;
        size_t sent_msgs_cache_size;

        sk_transmitter::lockable_queue send_q{};
        sk_transmitter::lockable_queue resend_q{};
        sk_transmitter::lockable_cache sent_msgs;

        void read() {
            sk_transmitter::msg_t buf(PSIZE);
            sk_transmitter::msg_id_t current_msg_id = 0;

            while (!std::cin.eof()) {
                std::cin.read(reinterpret_cast<char *>(buf.data()), buf.size());  // or `&buf[0]` on older platforms
                sk_transmitter::data_msg msg(current_msg_id, buf);
                current_msg_id += PSIZE;

                send_q.atomic_push(msg); // TODO: Make sure last chunk will be omitted
                sent_msgs.atomic_push(msg);
            }
        }

        void listen(std::shared_future<void> reading_complete) {
            sk_transmitter::ctrl_socket sock{CTRL_PORT};

            while (reading_complete.wait_for(std::chrono::milliseconds(1)) == std::future_status::timeout) {
                optional<sk_transmitter::ctrl_msg> req = sock.receive();
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
                optional<sk_transmitter::data_msg> msg = resend_q.atomic_get_and_pop();  // TODO: Atomic get all and make set to prevent double rexmits
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

    public:

        explicit transmitter(size_t PSIZE = 512,
                             size_t FSIZE = 65536,
                             size_t RTIME = 250,
                             std::string MCAST_ADDR = "239.10.11.12",
                             uint16_t DATA_PORT = 25830,
                             uint16_t CTRL_PORT = 35830,
                             std::string NAME = "Nienazwany nadajnik") : PSIZE(PSIZE),
                                                                         FSIZE(FSIZE),
                                                                         RTIME(RTIME),
                                                                         MCAST_ADDR(std::move(MCAST_ADDR)),
                                                                         DATA_PORT(DATA_PORT),
                                                                         CTRL_PORT(CTRL_PORT),
                                                                         NAME(std::move(NAME)),
                                                                         sent_msgs_cache_size(1 + ((FSIZE - 1) / PSIZE)),
                                                                         sent_msgs(sent_msgs_cache_size),
                                                                         session_id(static_cast<msg_id_t>(time(nullptr))) {}

        void transmit() {
            std::promise<void> reading_complete;
            std::shared_future<void> sc_future(reading_complete.get_future());

            std::thread sender(&transmitter::send, this, sc_future);
            std::thread listener(&transmitter::listen, this, sc_future);
            std::thread retransmitter(&transmitter::resend, this, sc_future);
            std::thread transmitter(&transmitter::read, this);

            transmitter.join();
            reading_complete.set_value();

            listener.join();
            retransmitter.join();
            sender.join();
        }
    };
}

#endif //SIK_NADAJNIK_TRANSMITTER_HPP
