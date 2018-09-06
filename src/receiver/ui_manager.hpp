#ifndef SIKRADIO_RECEIVER_UI_MANAGER_HPP
#define SIKRADIO_RECEIVER_UI_MANAGER_HPP

#include <mutex>
#include <vector>
#include <string>
#include <optional>
#include <poll.h>
#include <netinet/in.h>

#include "structures.hpp"

#ifndef MAX_UI_CLIENT_CONNECTIONS
#define MAX_UI_CLIENT_CONNECTIONS (_POSIX_OPEN_MAX - 10)
#endif

namespace sikradio::receiver {
    using menu_selection_update = sikradio::receiver::structures::menu_selection_update;

    namespace {
        const int listen_backlog = 5;
        const size_t buffer_size = 5;

        const std::string iac_do_linemode = "\255\253\34";
        const std::string iac_sb_linemode_mode_0_iac_se = "\255\250\34\1\0\255\240";
        const std::string iac_will_echo = "\255\251\1";
        const std::string clear_terminal = "\033[2J\033[0;0H";
        const std::string title = "  SIK Radio";
        const std::string hr = "------------------------------------------------------------------------";
        const std::string station_prefix = "    ";
        const std::string active_station_prefix = "  > ";
        const std::string telnet_endl = "\r\n";

        std::string 
        parse_stations(std::vector<std::string> stations, std::vector<std::string>::iterator selected) {
            std::ostringstream ret;
            ret << hr << telnet_endl;
            ret << title << telnet_endl;
            ret << hr << telnet_endl;
            for (auto it = stations.begin(); it != stations.end(); it++) {
                if (it == selected) ret << active_station_prefix;
                else ret << station_prefix;
                ret << (*it);
                ret << telnet_endl;
            }
            ret << hr << telnet_endl;
            return ret.str();
        }

        std::optional<sikradio::receiver::structures::menu_selection_update>
        parse_selection_update(std::string buffer) {
            if (buffer.size() != 3) return std::nullopt;
            if (buffer[0] != '\x1B' || buffer[1] != '\x5B') return std::nullopt;
            if (buffer[2] == 'B') return menu_selection_update::UP; // char[27,91,66]
            else if (buffer[2] == 'A') return menu_selection_update::DOWN; // char[27,91,65]
            else return std::nullopt;
        }
    }

    class ui_manager {
    private:
        std::mutex mut;
        in_port_t ui_port;
        int poll_timeout_in_ms;
        pollfd client_sockets[MAX_UI_CLIENT_CONNECTIONS];
        std::string active_menu;
        char buffer[buffer_size];

        void throw_safely(int sock_to_close) {
            if (sock_to_close >= 0) close(sock_to_close);
            throw sikradio::common::exceptions::socket_exception(strerror(errno));
        }

        void send_string(int sock, const std::string& command) {
            int err = write(sock, command.c_str(), command.size());
            if (err != static_cast<int>(command.size())) throw_safely(sock);
        }

        std::optional<std::string> read_string(int sock) {
            memset(buffer, 0, buffer_size);
            ssize_t read_bytes = read(sock, buffer, buffer_size);
            if (read_bytes < 0) throw_safely(sock);
            if (read_bytes == 0) return std::nullopt;
            return std::string(buffer, buffer + read_bytes);
        }

        void clear_revents() {
            for (auto& cs : client_sockets) {
                cs.revents = 0;
            }
        }

        void check_new_clients() {
            int err = poll(client_sockets, MAX_UI_CLIENT_CONNECTIONS, poll_timeout_in_ms);
            if (err < 0) throw_safely(client_sockets[0].fd);
            // handle new clients
            if (client_sockets[0].revents & POLLIN) {
                int msg_sock = accept(client_sockets[0].fd, NULL, 0);
                if (msg_sock < 0) throw_safely(client_sockets[0].fd);

                // find available space and register new client socket
                for (int i=1; i<MAX_UI_CLIENT_CONNECTIONS; i++) {
                    if (client_sockets[i].fd >= 0) continue;
                    client_sockets[i].fd = msg_sock;
                    try {
                        send_string(msg_sock, iac_do_linemode);
                        send_string(msg_sock, iac_sb_linemode_mode_0_iac_se);
                        send_string(msg_sock, iac_will_echo);
                        send_string(msg_sock, clear_terminal);
                        send_string(msg_sock, active_menu);
                    } catch (sikradio::common::exceptions::socket_exception& e) {
                        // client disconnected
                        client_sockets[i].fd = -1;
                    }
                    break;
                }
            }
        }

    public:
        ui_manager() = delete;
        ui_manager(const ui_manager& other) = delete;
        ui_manager(ui_manager&& other) = delete;
        ui_manager(in_port_t ui_port, int poll_timeout_in_ms) : 
                ui_port{ui_port},
                poll_timeout_in_ms{poll_timeout_in_ms} {
            // initialize stations to empty list with header
            std::vector<std::string> blank;
            active_menu = parse_stations(blank, blank.end());
            // initialize poll table
            for (int i=0; i < MAX_UI_CLIENT_CONNECTIONS; i++) {
                client_sockets[i].fd = -1;
                client_sockets[i].events = POLLIN;
                client_sockets[i].revents = 0;
            }
            // setup ui socket
            int sock = socket(PF_INET, SOCK_STREAM, 0);
            if (sock < 0) throw_safely(sock);
            // reuse address
            int reuse = 1;
            int err = setsockopt(
                sock, 
                SOL_SOCKET, 
                SO_REUSEADDR, 
                &reuse, 
                sizeof(reuse));
            if (err < 0) throw_safely(sock);
            // bind socket to ui_port
            struct sockaddr_in ui_sock{
                .sin_family=AF_INET,
                .sin_port=htons(ui_port)
            };
            ui_sock.sin_addr.s_addr = htonl(INADDR_ANY);
            err = bind(
                sock, 
                reinterpret_cast<struct sockaddr*>(&ui_sock), 
                static_cast<socklen_t>(sizeof(ui_sock)));
            if (err < 0) throw_safely(sock);
            // listen for client requests
            err = listen(sock, listen_backlog);
            if (err < 0) throw_safely(sock);
            // store ui socket at the beginning of poll table
            client_sockets[0].fd = sock;
        }

        void send_menu(
                std::vector<std::string> stations, 
                std::vector<std::string>::iterator selected) 
        {
            clear_revents();
            check_new_clients();
            auto new_menu = parse_stations(stations, selected);
            if (new_menu == active_menu) return;

            for (int i=1; i<MAX_UI_CLIENT_CONNECTIONS; i++) {
                if (client_sockets[i].fd < 0) continue;
                try {
                    send_string(client_sockets[i].fd, new_menu);
                } catch (sikradio::common::exceptions::socket_exception& e) {
                    // client disconnected
                    client_sockets[i].fd = -1;
                }
            }
            active_menu = new_menu;
        }

        std::optional<menu_selection_update> get_update() {
            clear_revents();
            check_new_clients();
            std::optional<std::string> str;
            for (int i=1; i<MAX_UI_CLIENT_CONNECTIONS; i++) {
                if (client_sockets[i].fd < 0) continue;
                try {
                    str = read_string(client_sockets[i].fd);
                } catch (sikradio::common::exceptions::socket_exception& e) {
                    // client disconnected
                    client_sockets[i].fd = -1;
                    continue;
                }
                if (str.has_value()) {
                    return parse_selection_update(str.value());
                }
            }
            return std::nullopt;
        }
    };
}

#endif