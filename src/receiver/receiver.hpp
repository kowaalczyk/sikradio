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
        // parameters for tuning performance for computers with lower multithreading capabilities
        const auto reset_check_freq = std::chrono::milliseconds(20);
        const auto rexmit_check_freq = std::chrono::milliseconds(10);
        const auto lookup_freq = std::chrono::seconds(5);
        // timeout applies to all receiver sockets
        const int socket_timeout_in_ms = 500; // has to be smaller than 1000 (1s)  TODO: Split in setsockopt
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
            while (true) {
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
            while (true) {
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
            while (true) {
                auto msg = sikradio::common::make_lookup();
                auto da_struct = sikradio::common::make_address(discover_addr, ctrl_port);
                ctrl_socket.force_send_to(da_struct, msg);

                std::this_thread::sleep_for(lookup_freq);
            }
        }

        void run_rexmit_sender() {  // LOCKS: 3
            std::set<sikradio::common::msg_id_t> ids_to_rexmit;
            std::set<sikradio::common::msg_id_t> ids_to_forget;
            while (true) {
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

        void run_data_receiver() {  // LOCKS: 1-4
            std::set<sikradio::common::msg_id_t> missed_ids;
            while (true) {
                std::scoped_lock{data_mut};

                auto msg = data_socket.try_read();
                if (!msg.has_value()) continue;

                auto msg_session_id = msg.value().get_session_id();
                auto ignore_msg = state_manager.register_session_check_ignore(msg_session_id);
                if (ignore_msg) continue;

                try {
                    missed_ids = buffer.write_get_missed(msg.value());
                } catch (sikradio::receiver::exceptions::buffer_access_exception &e) {
                    state_manager.mark_dirty();
                    missed_ids.clear();
                }
                if (!missed_ids.empty())
                    rexmit_manager.append_ids(missed_ids);
                missed_ids.clear();
            }
        }

        void run_data_streamer() {  // LOCKS: 1-2
            std::optional<sikradio::common::msg_t> read_msg = std::nullopt;
            while (true) {
                try {
                    read_msg = buffer.try_read();
                } catch (sikradio::receiver::exceptions::buffer_access_exception &e) {
                    state_manager.mark_dirty();
                    read_msg = std::nullopt;
                }
                if (read_msg.has_value())
                    std::cout << std::string(read_msg.value().begin(), read_msg.value().end());
            }
        }

        void run_ui_handler() {  // LOCKS: 4-5
            std::optional<sikradio::receiver::structures::menu_selection_update> enqueued_msu;
            std::optional<sikradio::receiver::structures::station> new_selected;
            std::vector<std::string> station_names;
            enqueued_msu = std::nullopt;
            new_selected = std::nullopt;
            while (true) {
                // if update is present, change station
                if (enqueued_msu.has_value())
                    new_selected = station_set.select_get_selected(enqueued_msu.value());                
                else
                    new_selected = station_set.get_selected();
                // register current state of station list in ui manager
                if (new_selected.has_value()) {
                    state_manager.register_address_check_change(new_selected.value());
                    station_names = station_set.get_station_names();
                    std::sort(station_names.begin(), station_names.end());
                    ui_manager.send_menu(station_names, new_selected.value().name);
                } else {
                    state_manager.mark_dirty();
                    station_names.clear();
                    ui_manager.send_menu(station_names);
                }
                // get update from ui manager
                enqueued_msu = ui_manager.get_update();
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
                 std::optional<std::string> preferred_station) : 
            discover_addr{discover_addr},
            ctrl_port{ctrl_port},
            buffer{bsize},
            data_socket{socket_timeout_in_ms},
            ctrl_socket{ctrl_port, socket_timeout_in_ms, true, false},
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
            std::thread data_receiver(&receiver::run_data_receiver, this);
            std::thread ui_handler(&receiver::run_ui_handler, this);

            run_data_streamer();

            ui_handler.join();
            data_receiver.join();
            lookup_sender.join();
            rexmit_sender.join();
            ctrl_receiver.join();
            resetter.join();
        }
    };
}

#endif
