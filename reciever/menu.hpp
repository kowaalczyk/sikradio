#ifndef SIKRADIO_RECEIVER_MENU_HPP
#define SIKRADIO_RECEIVER_MENU_HPP


#include <map>
#include <string>
#include <chrono>
#include <mutex>


#define MAX_INACTIVE_SECONDS 20


namespace sikradio::receiver {
    enum class menu_selection_update {UP, DOWN};

    class menu {
    private:
        std::mutex mut;
        std::map<std::string, std::chrono::time_point> channels;
        size_t selected_channel_id;

        void remove_old_items() {
            std::chrono::time_point now = std::chrono::system_clock::now();
            for (auto it = channels.begin(); it != channels.end();;) {
                auto last_update = it->second;
                auto duration_from_last_update = now - last_update;
                if (duration_from_last_update > std::chrono::seconds(MAX_INACTIVE_SECONDS)) {
                    it = channels.erase(it);
                } else {
                    it++;
                }
            }
        }

        void notify_clients() {
            // TODO: Send TELNET request to all connected clients
            // This should be done via async call to method in a separate class (ClientManager)
        }

    public:
        menu() = default;

        std::vector<std::string> get_items() const {
            std::vector<std::string> items;
            std::lock_guard<std::mutex> lock(mut);

            for (auto it = channels.begin(); it != channels.end(); it++) {
                items.emplace_back(it);
            }
            return items;
        }

        void refresh_items(const std::vector<std::string> &new_items) {
            std::lock_guard<std::mutex> lock(mut);
            std::chrono::time_point now = std::chrono::system_clock::now();
            
            for (auto it = new_items.begin(); it != new_items.end(); it++) {
                std::string item = (*it);
                channels[item] = now;  // update time or create
            }
            remove_old_items();
            notify_clients();
        }

        void update_selection(const sikradio::receiver::menu_selection_update msu) {
            std::lock_guard<std::mutex> lock(mut);

            if (msu == menu_selection_update::UP) {
                selected_channel_id = (selected_channel_id+1) % channels.size();
            } else {
                selected_channel_id = (selected_channel_id > 0 ? selected_channel_id-1 : channels.size()-1);
            }
            remove_old_items();
            notify_clients();
        }
}


#endif