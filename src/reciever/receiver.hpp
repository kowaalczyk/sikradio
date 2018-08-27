#ifndef SIKRADIO_RECEIVER_RECEIVER_HPP
#define SIKRADIO_RECEIVER_RECEIVER_HPP

#include <optional>
#include <thread>
#include <mutex>
#include <chrono>
#include <atomic>

#include "../common/ctrl_socket.hpp"
#include "../common/ctrl_msg.hpp"
#include "buffer.hpp"
#include "data_socket.hpp"
#include "station_set.hpp"

namespace sikradio::receiver {

    namespace {
        const size_t check_reset_frequency_in_ms = 20;
        const size_t lookup_sender_frequency_in_s = 5;
    }

    class receiver {
    private:
        // TODO: UI socket
        sikradio::receiver::data_socket data_socket;
        sikradio::common::ctrl_socket ctrl_socket;
        sikradio::receiver::buffer buffer;
        sikredio::receiver::station_set station_set;
        sikradio::receiver::rexmit_manager rexmit_manager;  // TODO: list of rexmit_ids + filtering + timestamps
        // TODO: UI client manager
        sikradio::receiver::state_manager state_manager;
        std::mutex data_mut{};

        void run_playback_resetter() {  // LOCKS: 1 or 3
            std::optional<std::string> multicast_address;
            bool dirty;
            while(true) {
                std::tie(multicast_address, dirty) = state_manager.check_state();
                if (dirty) {
                    // reset playback
                    std::lock_guard<std::mutex> lock(data_mut);  // stops data reciever
                    buffer.reset();
                    rexmit_manager.reset();  // TODO: Make sure there is no deadlock
                    data_socket.connect(new_multicast_address);
                }
                std::this_thread::sleep_for(std::chrono::miliseconds(check_reset_frequency_in_ms));
            }
        }

        void run_ctrl_reciever() {  // LOCKS: 0 or 2
            sikradio::common::ctrl_msg msg;
            struct in_addr sender_addr;
            while(true) {
                auto rcv = ctrl_socket.try_read();
                if (!rcv.has_value()) continue;
                
                std::tie(msg, sender_addr) = rcv.value();
                if (!msg.is_reply()) continue;
                
                auto station = sikradio::receiver::as_station(msg, sender_addr);  // TODO: Implement, remember about std::chrono::time_point now = std::chrono::system_clock::now();
                auto new_selected = station_set.update_get_selected(station);  // TODO: Implement
                state_manager.register_address(new_station.addres);
            }
        }

        void run_lookup_sender(ctrl_socket*) {  // LOCKS: 1
            while(true) {
                auto msg = sikradio::common::make_lookup();
                ctrl_socket.send_to(discover_addr, msg);
                std::this_thread::sleep_for(std::chrono::seconds(lookup_sender_frequency_in_s));
            }
        }

        void run_rexmit_sender() {  // LOCKS: 3
            std::set<msg_id_t> ids_to_rexmit;
            std::set<msg_id_t> ids_to_forget;
            while(true) {
                // TODO: rexmit_manager.filter_ids should take ids_to_forget, remove them from internal storage
                // and return list of ids that can be retransmitted in this iteration, 
                // according to condition: previous_rexmit < next_rexmit_time_for_msg <= now;
                // (saving now as previous rexmit, and updating next_rexmit_time += RTIME for all returned values)
                ids_to_rexmit = rexmit_manager.filter_ids(ids_to_forget);  // TODO: Implement
                ids_to_forget.clear();
                for (auto it = ids_to_rexmit.begin(); it != ids_to_rexmit.end();) {
                    if (!buffer.has_space_for(*it)) {
                        ids_to_forget.emplace(*it);
                        it = ids_to_rexmit.erase(it);
                    } else {
                        it++;
                    }
                }
                auto msg = sikradio::common::make_rexmit(ids_to_rexmit);
                auto current_station = station_set.get_selected();
                ctrl_socket.send_to(
                    current_station.ctrl_address, 
                    current_station.ctrl_port,
                    msg);  // TODO: send_to with this interface (string:ip, port, msg)
            }
        }

        void run_data_reciever(data_socket*, buffer*, station_list*) {  // LOCKS: 1 or 2 or 4
            std::optional<sikradio::common::msg_id_t> max_msg_id = std::nullopt;
            while(true) {
                std::scoped_lock{mut};  // TODO: Check for deadlocks

                auto msg = data_socket.try_read();
                if (!msg.has_value()) continue;

                auto msg_session_id = msg.value().session_id;
                auto ignore_msg = state_manager.register_session(msg_session_id);
                if (ignore_msg) continue;

                auto msg_id = msg.value().id;
                if (max_msg_id.has_value() && max_msg_id.value() + 1 != msg_id)
                    rexmit_manager.append_ids(max_msg_id.value() + 1, msg_id);  // TODO: Implement

                buffer.write(msg.value());
                max_msg_id = std::optional<sikradio::common::msg_id_t>(std::max(max_msg_id.value_or(0), current_id));
            }
        }

        void run_data_streamer(buffer*) {  // LOCKS: 1 or 2
            // TODO:
            std::optional<sikradio::common::msg_t> msg = std::nullopt;
            while (true) {
                try {
                    msg = buffer.try_read();
                } catch sikradio::reciever::exceptions::buffer_access_exception &e {
                    std::cerr << e.what() << std::endl;
                    state_manager.mark_dirty();  // TODO: Implement
                }
                if (!msg.has_value()) continue;
                std::cout << msg.value().data();
            }
        }

        void run_ui_reciever(ui_socket*, station_list*) {
            // TODO: 
            // 1. try read from ui socket
            // 2. if read something:
            // 3. update selection in station_list
            // 4. register update in state manager
        }

        void run_ui_sender() {
            // TODO:
            // 1. get list of available stations
            // 2. stream them to all listening UIs
        }
    public:
        receiver() = delete;
    };
}

#endif
