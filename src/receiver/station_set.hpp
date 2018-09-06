#ifndef SIKRADIO_RECEIVER_STATION_SET_HPP
#define SIKRADIO_RECEIVER_STATION_SET_HPP

#include <set>
#include <tuple>
#include <string>
#include <iterator>
#include <chrono>
#include <mutex>
#include <optional>

#include "structures.hpp"

namespace sikradio::receiver {
    namespace {
        const size_t MAX_INACTIVE_SECONDS = 20;
        const std::string STATION_PREFIX = "    ";
        const std::string SELECTED_STATION_PREFIX = "  > ";
    }

    using station = sikradio::receiver::structures::station;
    using menu_selection_update = sikradio::receiver::structures::menu_selection_update;

    class station_set {
    private:
        std::mutex mut{};
        std::optional<std::string> preferred_station_name{std::nullopt};
        std::set<station> stations{};
        std::set<station>::iterator selected_station{stations.end()};

        void remove_old_stations() {
            auto now = std::chrono::system_clock::now();
            for (auto it = stations.begin(); it != stations.end();/*update in body*/) {
                auto last_update = it->last_reply;
                auto duration_from_last_update = now - last_update;
                if (duration_from_last_update <= std::chrono::seconds(MAX_INACTIVE_SECONDS)) {
                    it++;
                    continue;
                }
                if (*it == *selected_station) {
                    // try to change selected station to an active one
                    if (selected_station != stations.end()) 
                        selected_station++;
                    if (selected_station == stations.end()) 
                        selected_station = stations.begin();
                }
                it = stations.erase(it);
            }
        }

        std::optional<station> get_selected_station() {
            if (stations.empty()) {
                return std::nullopt;
            }
            return std::make_optional(*selected_station);
        }

    public:
        station_set() = default;
        station_set(const station_set& other) = delete;
        station_set(station_set&& other) = delete;

        explicit station_set(std::string preferred_station_name) {
            this->preferred_station_name = std::make_optional(std::move(preferred_station_name));
        }

        explicit station_set(std::optional<std::string> preferred_station_name) : 
                preferred_station_name{preferred_station_name} {}

        std::optional<station> 
        update_get_selected(const station &new_station) {
            std::scoped_lock{mut};
            remove_old_stations();

            std::set<station>::iterator new_it;
            size_t erased = stations.erase(new_station);
            std::tie(new_it, std::ignore) = stations.emplace(new_station);

            if (selected_station == stations.end()) selected_station = new_it;
            // we will only select preferred station when it appears for the 1st time
            if (preferred_station_name.has_value() 
                    && preferred_station_name.value() == new_station.name
                    && erased == 0)
                selected_station = new_it;

            return get_selected_station();
        }

        std::optional<station> 
        select_get_selected(const menu_selection_update msu) {
            std::scoped_lock{mut};
            remove_old_stations();
            
            if (stations.empty()) return get_selected_station();
            if (msu == menu_selection_update::UP) {
                if (selected_station != stations.end()) selected_station++;
                if (selected_station == stations.end()) selected_station = stations.begin();
            } else {
                if (selected_station == stations.begin()) selected_station = stations.end();
                if (selected_station != stations.begin()) selected_station--;
            }
            return get_selected_station();
        }

        std::optional<station> get_selected() {
            std::scoped_lock{mut};
            remove_old_stations();
            return get_selected_station();
        }

        std::vector<std::string> get_station_names() {
            std::scoped_lock{mut};
            remove_old_stations();

            std::vector<std::string> station_names;
            station_names.reserve(stations.size());
            for (auto it = stations.begin(); it != stations.end(); it++) {
                station_names.emplace_back(it->name);
            }
            return station_names;
        }
    };
}

#endif
