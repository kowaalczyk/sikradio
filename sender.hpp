#ifndef SIKRADIO_SENDER_TRANSMITTER_HPP
#define SIKRADIO_SENDER_TRANSMITTER_HPP


#include <cstdint>
#include <iostream>
#include <future>
#include <utility>
#include "sender/types.hpp"
#include "sender/data_msg.hpp"
#include "sender/data_socket.hpp"
#include "sender/lockable_cache.hpp"
#include "sender/lockable_queue.hpp"
#include "sender/ctrl_socket.hpp"


namespace sender {
    class transmitter {
    private:
        size_t PSIZE = 512;
        size_t FSIZE = 65536;
        size_t RTIME = 250;
        std::string MCAST_ADDR = "239.10.11.12";
        uint16_t DATA_PORT = 25830;
        uint16_t CTRL_PORT = 35830;
        std::string NAME = "";

        size_t sent_msgs_cache_size;
        sender::lockable_queue send_q{};
        sender::lockable_queue resend_q{};
        sender::lockable_cache sent_msgs;
        sender::msg_id_t session_id;

        void read() {
            sender::msg_t buf(PSIZE);
            sender::msg_id_t current_msg_id = 0;

            while (!std::cin.eof()) {
                std::cin.read(reinterpret_cast<char *>(buf.data()), buf.size());  // or `&buf[0]` on older platforms
                sender::data_msg msg(current_msg_id, buf);
                current_msg_id += PSIZE;

                send_q.atomic_push(msg);
                sent_msgs.atomic_push(msg);
            }
        }

        void listen(std::shared_future<void> reading_complete) {
            sender::ctrl_socket sock{CTRL_PORT};

            while (reading_complete.wait_for(std::chrono::milliseconds(1)) == std::future_status::timeout) {
                optional<sender::ctrl_msg> req = sock.receive();
                if (req.has_value()) {

                    if (req.value().is_lookup()) {
                        std::string lookup_response = "BOREWICZ_HERE ";
                        lookup_response += MCAST_ADDR;
                        lookup_response += " ";
                        lookup_response += std::to_string(DATA_PORT);
                        lookup_response += " ";
                        lookup_response += NAME;
                        lookup_response += "\n";

                        while (true) {
                            try {
                                sock.respond(req.value(), lookup_response.c_str(), sizeof(lookup_response.c_str()));
                                break;
                            } catch (sender::exceptions::socket_exception &e) {
                                // retry
                            }
                        }
                    }

                    if (req.value().is_rexmit()) {
                        // msg_ids are present (in case there are no ids, vector will be empty but never null)
                        std::vector<sender::msg_id_t> msg_ids = req.value().get_rexmit_ids().value();
                        for (auto id : msg_ids) {
                            optional<sender::data_msg> msg = sent_msgs.atomic_get(id);
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
            sender::data_socket sock{MCAST_ADDR, static_cast<in_port_t>(DATA_PORT)};

            while (reading_complete.wait_for(std::chrono::milliseconds(1)) == std::future_status::timeout) {
                std::set<sender::data_msg> unique_msgs = resend_q.atomic_get_unique();  // Assuming we need to only retransmit each requested messag once
                for (auto msg : unique_msgs) {
                    while (true) {
                        try {
                            sock.send(msg.sendable_with_session_id(session_id));
                            break;
                        } catch (sender::exceptions::socket_exception &e) {
                            // retry
                        }
                    }
                }

                // Assuming RTIME is a time between retransmissions (not between beinnnings of consecutive retransmissions)
                std::this_thread::sleep_for(std::chrono::milliseconds(RTIME));
            }
        }

        void send(std::shared_future<void> reading_complete) {
            sender::data_socket sock{MCAST_ADDR, static_cast<in_port_t>(DATA_PORT)};

            while (reading_complete.wait_for(std::chrono::milliseconds(1)) == std::future_status::timeout) {
                optional<sender::data_msg> msg = send_q.atomic_get_and_pop();

                if (msg.has_value()) {
                    while (true) {
                        try {
                            sock.send(msg.value().sendable_with_session_id(session_id));
                            break;
                        } catch (sender::exceptions::socket_exception &e) {
                            // retry
                        }
                    }
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


#endif //SIKRADIO_SENDER_TRANSMITTER_HPP
