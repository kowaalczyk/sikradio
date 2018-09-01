#ifndef SIKRADIO_RECEIVER_RECEIVER_HPP
#define SIKRADIO_RECEIVER_RECEIVER_HPP

#include <optional>
#include <thread>
#include <mutex>
#include <chrono>
#include <atomic>
#include <future>
#include <csignal>

#include "../common/ctrl_socket.hpp"
#include "../common/ctrl_msg.hpp"
#include "buffer.hpp"
#include "data_socket.hpp"
#include "station_set.hpp"
#include "rexmit_manager.hpp"
#include "state_manager.hpp"

namespace sikradio::receiver {

    namespace {
        const auto reset_check_freq = std::chrono::milliseconds(20);
        const auto lookup_freq = std::chrono::seconds(5);
    }

    class receiver {
    private:
        std::string discover_addr;
        in_port_t ctrl_port;
        sikradio::receiver::buffer buffer;
        sikradio::receiver::data_socket data_socket;
        sikradio::common::ctrl_socket ctrl_socket;
        // TODO: UI socket
        sikradio::receiver::station_set station_set;
        sikradio::receiver::rexmit_manager rexmit_manager;
        sikradio::receiver::state_manager state_manager;
        // TODO: UI client manager
        std::mutex data_mut{};

        void run_playback_resetter() {  // LOCKS: 1 or 3
            std::optional<std::string> multicast_address;
            bool dirty;
            while(true) {
                std::tie(multicast_address, dirty) = state_manager.check_state();
                if (dirty) {  // reset playback
                    std::scoped_lock{data_mut};  // stops data receiver TODO: Make sure there is no deadlock
                    buffer.reset();
                    rexmit_manager.reset();
                    if (multicast_address.has_value())
                        data_socket.connect(multicast_address.value());
                }
                std::this_thread::sleep_for(reset_check_freq);
            }
        }

        void run_ctrl_receiver() {  // LOCKS: 0 or 2
            sikradio::common::ctrl_msg msg;
            struct sockaddr_in sender_addr;
            while(true) {
                auto rcv = ctrl_socket.try_read();
                if (!rcv.has_value()) continue;
                
                std::tie(msg, sender_addr) = rcv.value();
                if (!msg.is_reply()) continue;
                
                auto station = sikradio::receiver::as_station(msg, sender_addr);
                auto new_selected = station_set.update_get_selected(station);
                if (!new_selected.has_value()) continue;
                state_manager.register_address(new_selected.value().data_address);
            }
        }

        void run_lookup_sender() {  // LOCKS: 1
            while(true) {
                auto msg = sikradio::common::make_lookup();
                ctrl_socket.send_to(discover_addr, ctrl_port, msg);
                std::this_thread::sleep_for(lookup_freq);
            }
        }

        void run_rexmit_sender() {  // LOCKS: 3
            std::set<sikradio::common::msg_id_t> ids_to_rexmit;
            std::set<sikradio::common::msg_id_t> ids_to_forget;
            while(true) {
                ids_to_rexmit = rexmit_manager.filter_get_ids(ids_to_forget);
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
                if (!current_station.has_value()) continue;
                
                ctrl_socket.send_to(
                    current_station.value().ctrl_address, 
                    current_station.value().ctrl_port,
                    msg);
            }
        }

        void run_data_receiver() {  // LOCKS: 1 or 2 or 4
            std::optional<sikradio::common::msg_id_t> max_msg_id = std::nullopt;
            while(true) {
                std::scoped_lock{data_mut};  // TODO: Check for deadlocks

                auto msg = data_socket.try_read();
                if (!msg.has_value()) continue;

                auto msg_session_id = msg.value().get_session_id();
                auto ignore_msg = state_manager.register_session(msg_session_id);
                if (ignore_msg) continue;

                auto msg_id = msg.value().get_id();
                if (max_msg_id.has_value() && max_msg_id.value() + 1 != msg_id)
                    rexmit_manager.append_ids(max_msg_id.value() + 1, msg_id);

                buffer.write(msg.value());
                max_msg_id = std::make_optional(std::max(max_msg_id.value_or(0), msg_id));
            }
        }

        void run_data_streamer() {  // LOCKS: 1 or 2
            std::optional<sikradio::common::msg_t> msg = std::nullopt;
            while (true) {
                try {
                    msg = buffer.try_read();
                } catch (sikradio::receiver::exceptions::buffer_access_exception &e) {
                    std::cerr << e.what() << std::endl;
                    state_manager.mark_dirty();
                }
                if (!msg.has_value()) continue;
                std::cout << msg.value().data();
            }
        }

        void run_ui_receiver() {
            while (true) {
                // TODO: 
                // 1. try read from ui socket
                // 2. if read something:
                // 3. update selection in station_list
                // 4. register update in state manager
            }
        }

        void run_ui_sender() {
            while (true) {
                // TODO:
                // 1. get list of available stations
                // 2. stream them to all listening UIs
            }
        }

    public:
        receiver() = delete;
        receiver(const receiver& other) = delete;
        receiver(receiver&& other) = delete;

        receiver(std::string discover_addr, 
                 in_port_t ctrl_port, 
                 in_port_t ui_port, 
                 size_t bsize, 
                 size_t rtime, 
                 std::optional<std::string> preferred_station
        ) : discover_addr{discover_addr},
            ctrl_port{ctrl_port},
            buffer{bsize},
            data_socket{},
            ctrl_socket{ctrl_port},
            // ui_socket{ui_port},  // TODO: Implement
            station_set{preferred_station},
            rexmit_manager{rtime},
            state_manager{},
            // client_manager{},  // TODO: Implement
            data_mut{} {}

        void run() {
            std::thread resetter(&receiver::run_playback_resetter, this);
            std::thread ctrl_receiver(&receiver::run_ctrl_receiver, this);
            std::thread lookup_sender(&receiver::run_lookup_sender, this);
            std::thread rexmit_sender(&receiver::run_rexmit_sender, this);
            std::thread ui_sender(&receiver::run_ui_sender, this);
            std::thread ui_receiver(&receiver::run_ui_receiver, this);
            std::thread data_receiver(&receiver::run_data_receiver, this);

            run_data_streamer();

            // TODO: This might be unnecessary (threads have to be terminated anyway)
            data_receiver.join();
            ui_sender.join();
            ui_receiver.join();            
            lookup_sender.join();
            rexmit_sender.join();
            ctrl_receiver.join();
            resetter.join();
        }
    };
}

#endif
