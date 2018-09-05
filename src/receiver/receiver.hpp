#ifndef SIKRADIO_RECEIVER_RECEIVER_HPP
#define SIKRADIO_RECEIVER_RECEIVER_HPP

#include <optional>
#include <thread>
#include <mutex>
#include <chrono>
#include <atomic>
#include <future>
#include <csignal>
#include <iostream>
#include <algorithm>

#include "../common/ctrl_socket.hpp"
#include "../common/ctrl_msg.hpp"
#include "../common/address_helpers.hpp"
#include "buffer.hpp"
#include "data_socket.hpp"
#include "station_set.hpp"
#include "rexmit_manager.hpp"
#include "state_manager.hpp"
#include "ui_manager.hpp"

namespace sikradio::receiver {

    namespace {
        const auto reset_check_freq = std::chrono::milliseconds(20);
        const auto rexmit_check_freq = std::chrono::milliseconds(10);
        const auto lookup_freq = std::chrono::seconds(5);
        const size_t socket_timeout_in_ms = 5000;
    }

    class receiver {
    private:
        std::string discover_addr;
        in_port_t ctrl_port;
        sikradio::receiver::buffer buffer;
        sikradio::receiver::data_socket data_socket;
        sikradio::common::ctrl_socket ctrl_socket;
        sikradio::receiver::station_set station_set;
        sikradio::receiver::rexmit_manager rexmit_manager;
        sikradio::receiver::state_manager state_manager;
        sikradio::receiver::ui_manager ui_manager;
        std::mutex data_mut{};

        void run_playback_resetter() {  // LOCKS: 1 or 3
            std::optional<sikradio::receiver::structures::station> station;
            bool dirty;
            while(true) {
                std::tie(station, dirty) = state_manager.check_state();
                if (dirty) {  // reset playback
                    std::scoped_lock{data_mut};  // stops data_receiver
                    buffer.reset();
                    rexmit_manager.reset();
                    if (station.has_value())
                        data_socket.connect(station.value());
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
                
                auto station = sikradio::receiver::structures::as_station(msg, sender_addr);
                auto new_selected = station_set.update_get_selected(station);
                if (!new_selected.has_value()) continue;
                
                state_manager.register_address_check_change(new_selected.value());
            }
        }

        void run_lookup_sender() {  // LOCKS: 1
            while(true) {
                auto msg = sikradio::common::make_lookup();
                auto da_struct = sikradio::common::make_address(discover_addr, ctrl_port);
                ctrl_socket.force_send_to(da_struct, msg);

                std::this_thread::sleep_for(lookup_freq);
            }
        }

        void run_rexmit_sender() {  // LOCKS: 3
            std::set<sikradio::common::msg_id_t> ids_to_rexmit;
            std::set<sikradio::common::msg_id_t> ids_to_forget;
            while(true) {
                std::this_thread::sleep_for(rexmit_check_freq);

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
                if (ids_to_rexmit.empty()) continue;

                auto msg = sikradio::common::make_rexmit(ids_to_rexmit);
                auto current_station = station_set.get_selected();
                if (!current_station.has_value()) continue;
                
                auto addr = sikradio::common::make_address(
                    current_station.value().ctrl_address,
                    current_station.value().ctrl_port
                );
                ctrl_socket.send_to(addr, msg);
            }
        }

        void run_data_handler() {  // LOCKS: 1-4
            std::optional<sikradio::common::msg_t> read_msg = std::nullopt;
            std::set<sikradio::common::msg_id_t> missed_ids;
            while(true) {
                std::scoped_lock{data_mut};

                auto msg = data_socket.try_read();
                if (msg.has_value()) {
                    auto msg_session_id = msg.value().get_session_id();
                    auto ignore_msg = state_manager.register_session_check_ignore(msg_session_id);
                    
                    if (ignore_msg) {
                        read_msg = std::nullopt;
                        missed_ids.clear();
                    } else {
                        try {
                            std::tie(read_msg, missed_ids) = buffer.write_try_read_get_rexmit(msg.value());
                        } catch (sikradio::receiver::exceptions::buffer_access_exception &e) {
                            read_msg = std::nullopt;
                            missed_ids.clear();
                            state_manager.mark_dirty();
                        }
                    }
                } else {
                    try {
                        read_msg = buffer.try_read();
                    } catch (sikradio::receiver::exceptions::buffer_access_exception &e) {
                        read_msg = std::nullopt;
                        missed_ids.clear();
                        state_manager.mark_dirty();
                    }
                }
                if (read_msg.has_value()) {
                    std::cout << read_msg.value().data();
                }
                if (!missed_ids.empty()) {
                    rexmit_manager.append_ids(missed_ids);
                }
            }
        }

        void run_ui_handler() {  // LOCKS: 1-5
            while (true) {
                auto msu = ui_manager.get_update();
                if (!msu.has_value()) continue;

                auto new_selected = station_set.select_get_selected(msu.value());
                if (!new_selected.has_value()) continue;

                bool changed = state_manager.register_address_check_change(new_selected.value());
                if (!changed) continue;

                auto station_list = station_set.get_station_names();
                std::sort(station_list.begin(), station_list.end());
                auto selected_name_it = std::find(
                    station_list.begin(), 
                    station_list.end(), 
                    new_selected.value().name);
                ui_manager.send_menu(station_list, selected_name_it);
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
            data_socket{},  // TODO: Socket timeout in ms
            ctrl_socket{ctrl_port, true, false},  // TODO: Socket timeout in ms
            station_set{preferred_station},
            rexmit_manager{rtime},
            state_manager{},
            ui_manager{ui_port, socket_timeout_in_ms},
            data_mut{} {}

        void run() {
            std::thread resetter(&receiver::run_playback_resetter, this);
            std::thread ctrl_receiver(&receiver::run_ctrl_receiver, this);
            std::thread lookup_sender(&receiver::run_lookup_sender, this);
            std::thread rexmit_sender(&receiver::run_rexmit_sender, this);
            std::thread ui_handler(&receiver::run_ui_handler, this);

            run_data_handler();

            // TODO: This might be unnecessary (threads have to be terminated anyway)
            ui_handler.join();
            lookup_sender.join();
            rexmit_sender.join();
            ctrl_receiver.join();
            resetter.join();
        }
    };
}

#endif
