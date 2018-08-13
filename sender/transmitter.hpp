#ifndef SIKRADIO_SENDER_TRANSMITTER_HPP
#define SIKRADIO_SENDER_TRANSMITTER_HPP


#include <cstdint>
#include <iostream>
#include <future>
#include <utility>
#include "../common/types.hpp"
#include "../common/data_msg.hpp"
#include "data_socket.hpp"
#include "lockable_cache.hpp"
#include "lockable_queue.hpp"
#include "ctrl_socket.hpp"


namespace sikradio::sender {
    class transmitter {
    private:
        // transmitter parameters
        size_t PSIZE;
        size_t FSIZE;
        size_t RTIME;
        std::string MCAST_ADDR;
        uint16_t DATA_PORT;
        uint16_t CTRL_PORT;
        std::string NAME;

        // transmitter state
        size_t sent_msgs_cache_size;
        sikradio::sender::lockable_queue send_q{};
        sikradio::sender::lockable_queue resend_q{};
        sikradio::sender::lockable_cache sent_msgs;
        sikradio::common::msg_id_t session_id;

        std::string compose_lookup_response() {
            std::string lookup_response = "BOREWICZ_HERE ";
            lookup_response += MCAST_ADDR;
            lookup_response += " ";
            lookup_response += std::to_string(DATA_PORT);
            lookup_response += " ";
            lookup_response += NAME;
            lookup_response += "\n";
            return lookup_response;
        }

        void process_lookup_request(sikradio::sender::ctrl_socket &sock, sikradio::common::ctrl_msg &req) {
            std::string lookup_response = compose_lookup_response();
            sock.respond_force(
                req, 
                lookup_response.c_str(), 
                strlen(lookup_response.c_str())
            );
        }

        void process_rexmit_request(sikradio::common::ctrl_msg &req) {
            std::vector<sikradio::common::msg_id_t> msg_ids = req.get_rexmit_ids();
            
            for (auto id : msg_ids) {
                optional<sikradio::common::data_msg> msg = sent_msgs.atomic_get(id);
                if (msg.has_value() && msg.value().get_id() == id) {
                    // message with desired id was still stored in cache
                    resend_q.atomic_push(msg.value());
                }
            }
        }

        void read_input() {
            sikradio::common::msg_t buf(PSIZE);
            sikradio::common::msg_id_t current_msg_id = 0;

            while (!std::cin.eof()) {
                std::cin.read(reinterpret_cast<char *>(buf.data()), buf.size());  // or `&buf[0]` on older platforms
                sikradio::common::data_msg msg(current_msg_id, buf);
                current_msg_id += PSIZE;

                send_q.atomic_push(msg);
                sent_msgs.atomic_push(msg);
            }
        }

        void run_listener(std::shared_future<void> reading_complete) {
            sikradio::sender::ctrl_socket sock{CTRL_PORT};

            while (reading_complete.wait_for(std::chrono::milliseconds(1)) == std::future_status::timeout) {
                optional<sikradio::common::ctrl_msg> req = sock.receive();
                if (!req.has_value()) continue;

                if (req.value().is_lookup()) {
                    process_lookup_request(sock, req.value());
                }
                if (req.value().is_rexmit()) {
                    process_rexmit_request(req.value());
                }
            }
        }

        void run_retransmitter(std::shared_future<void> reading_complete) {
            sikradio::sender::data_socket sock{MCAST_ADDR, static_cast<in_port_t>(DATA_PORT)};

            while (reading_complete.wait_for(std::chrono::milliseconds(1)) == std::future_status::timeout) {
                // make set not to retransmit same message twice in one batch
                std::set<sikradio::common::data_msg> unique_msgs = resend_q.atomic_get_unique();
                for (auto msg : unique_msgs) {
                    msg.set_session_id(session_id);
                    sock.transmit_force(msg.sendable());
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(RTIME));
            }
        }

        void run_sender(std::shared_future<void> reading_complete) {
            sikradio::sender::data_socket sock{MCAST_ADDR, static_cast<in_port_t>(DATA_PORT)};

            while (reading_complete.wait_for(std::chrono::milliseconds(1)) == std::future_status::timeout) {
                optional<sikradio::common::data_msg> msg = send_q.atomic_get_and_pop();
                if (msg.has_value()) {
                    msg.value().set_session_id(session_id);
                    sock.transmit_force(msg.value().sendable());
                }
            }
        }

    public:
        explicit transmitter(
            size_t PSIZE,
            size_t FSIZE,
            size_t RTIME,
            std::string MCAST_ADDR,
            uint16_t DATA_PORT,
            uint16_t CTRL_PORT,
            std::string NAME) : PSIZE(PSIZE),
                                FSIZE(FSIZE),
                                RTIME(RTIME),
                                MCAST_ADDR(std::move(MCAST_ADDR)),
                                DATA_PORT(DATA_PORT),
                                CTRL_PORT(CTRL_PORT),
                                NAME(std::move(NAME)),
                                sent_msgs_cache_size(1 + ((FSIZE - 1) / PSIZE)),
                                sent_msgs(sent_msgs_cache_size),
                                session_id(static_cast<sikradio::common::msg_id_t>(time(nullptr))) {}

        void transmit() {
            std::promise<void> reading_complete;
            std::shared_future<void> sc_future(reading_complete.get_future());

            std::thread sender(&transmitter::run_sender, this, sc_future);
            std::thread listener(&transmitter::run_listener, this, sc_future);
            std::thread retransmitter(&transmitter::run_retransmitter, this, sc_future);

            read_input();
            reading_complete.set_value();

            listener.join();
            retransmitter.join();
            sender.join();
        }
    };
}


#endif //SIKRADIO_SENDER_TRANSMITTER_HPP
