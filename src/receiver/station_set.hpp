#ifndef SIKRADIO_RECEIVER_STATION_SET_HPP
#define SIKRADIO_RECEIVER_STATION_SET_HPP


#include <set>
#include <tuple>
#include <string>
#include <iterator>
#include <chrono>
#include <mutex>
#include <optional>

#include "station.hpp"

namespace sikradio::receiver {
    namespace {
        const size_t MAX_INACTIVE_SECONDS = 20;
        const std::string STATION_PREFIX = "    ";
        const std::string SELECTED_STATION_PREFIX = "  > ";
    }

    enum class menu_selection_update {UP, DOWN};

    class station_set {
    private:
        std::mutex mut{};
        std::optional<std::string> preferred_station_name{std::nullopt};
        std::set<sikradio::receiver::station> stations{};
        std::set<sikradio::receiver::station>::iterator selected_station = stations.end();

        /**
         * Removes stations older than MAX_INACTIVE_SECONDS from the internal collection.
         * If selected station is removed, selects: 
         * - Next station if the station has one
         * - First station if it exists
         * If first station does not exist, internal collection is empty and no station is selected.
         */
        void remove_old_stations() {
            std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
            for (auto it = stations.begin(); it != stations.end();/*update in body*/) {
                auto last_update = it->last_reply;
                auto duration_from_last_update = now - last_update;
                if (duration_from_last_update > std::chrono::seconds(MAX_INACTIVE_SECONDS)) {
                    if (*it == *selected_station) {
                        if (selected_station != stations.end()) selected_station++;
                        if (selected_station == stations.end()) selected_station = stations.begin();
                    }
                    it = stations.erase(it);
                } else {
                    it++;
                }
            }
        }

        std::optional<sikradio::receiver::station> get_selected_station() {
            if (stations.empty()) {
                assert(selected_station == stations.begin());
                assert(selected_station == stations.end());
                return std::nullopt;
            }
            return std::make_optional(*selected_station);
        }

    public:
        station_set() = default;

        explicit station_set(std::string preferred_station_name) {
            this->preferred_station_name = std::make_optional(std::move(preferred_station_name));
        }

        explicit station_set(std::optional<std::string> preferred_station_name) : preferred_station_name{preferred_station_name} {}

        std::optional<sikradio::receiver::station> update_get_selected(const sikradio::receiver::station &new_station) {
            std::scoped_lock{mut};

            std::set<sikradio::receiver::station>::iterator new_it;
            stations.erase(new_station);
            std::tie(new_it, std::ignore) = stations.emplace(new_station);

            if (selected_station == stations.end()) selected_station = new_it;
            if (preferred_station_name.has_value() && preferred_station_name.value() == new_station.name)
                selected_station = new_it;
            remove_old_stations();
            return get_selected_station();
        }

        std::optional<sikradio::receiver::station> select_get_selected(const sikradio::receiver::menu_selection_update msu) {
            std::scoped_lock{mut};
            
            if (stations.empty()) return get_selected_station();
            if (msu == menu_selection_update::UP) {
                if (std::next(selected_station) != stations.end()) selected_station++;
            } else {
                if (selected_station != stations.begin()) selected_station--;
            }
            remove_old_stations();
            return get_selected_station();
        }

        std::optional<sikradio::receiver::station> get_selected() {
            std::scoped_lock{mut};
            return get_selected_station();
        }

        std::vector<std::string> get_printable_station_names() {
            std::scoped_lock{mut};

            std::vector<std::string> station_names;
            station_names.reserve(stations.size());
            for (auto it = stations.begin(); it != stations.end(); it++) {
                std::string station_name = "";
                if (*it == *selected_station) station_name += SELECTED_STATION_PREFIX;
                else station_name += STATION_PREFIX;
                station_name += it->name;
                station_names.emplace_back(station_name);
            }
            return station_names;
        }
    };
}


#endif