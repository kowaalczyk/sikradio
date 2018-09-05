#ifndef SIKRADIO_RECEIVER_UI_MANAGER_HPP
#define SIKRADIO_RECEIVER_UI_MANAGER_HPP

#include <mutex>
#include <netinet/in.h>

#include "structures.hpp"

namespace sikradio::receiver {
    using menu_selection_update = sikradio::receiver::structures::menu_selection_update;

    class ui_manager {
    private:
        std::mutex mut;
        in_port_t ui_port;
        std::vector<int> socket_pool;

    public:
        ui_manager() = delete;
        ui_manager(const ui_manager& other) = delete;
        ui_manager(ui_manager&& other) = delete;
        explicit ui_manager(in_port_t ui_port) : ui_port{ui_port} {}

        void send_menu(
                std::vector<std::string> stations, 
                std::vector<std::string>::iterator selected) 
        {
            // TODO: 
            // 1. poll and save message to return as last_update (overriding if necessary)
            // 2. for each of new clients, transmit last_sent_menu or deactivate client
            // 3. return last_update
        }

        std::optional<menu_selection_update> get_update() {
            // TODO:
            // 1. poll, if there are updates save message to return as last_update
            // 2. for each of clients, transmit sendable_menu
            // 3. save sendable_menu as last_sent_menu
            return std::nullopt;
        }
    };
}

#endif