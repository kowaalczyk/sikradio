#ifndef SIKRADIO_RECEIVER_STATE_MANAGER_HPP
#define SIKRADIO_RECEIVER_STATE_MANAGER_HPP

#include <string>
#include <optional>
#include <mutex>
#include <atomic>
#include <netinet/in.h>

#include "../common/types.hpp"
#include "structures.hpp"

namespace sikradio::receiver {
    using station = sikradio::receiver::structures::station;

    class state_manager {
    private:
        std::mutex mut{};
        std::optional<station> active_station{std::nullopt};
        std::atomic_bool dirty{false};
        sikradio::common::msg_id_t session_id{0};

    public:
        state_manager() = default;
        state_manager(const state_manager& other) = delete;
        state_manager(state_manager&& other) = delete;

        std::tuple<std::optional<station>, in_port_t> check_state() {
            std::scoped_lock{mut};

            bool was_dirty = dirty;
            dirty = false;
            return std::make_tuple(active_station, was_dirty);
        }

        bool register_address_check_change(station new_station) {
            std::scoped_lock{mut};

            if (!active_station.has_value() || active_station.value() != new_station) {
                active_station = std::make_optional(new_station);
                dirty = true;
            }
            return dirty;
        }

        bool register_session_check_ignore(sikradio::common::msg_id_t new_session_id) {
            std::scoped_lock{mut};

            if (new_session_id < session_id) return true;
            if (new_session_id == session_id) return false;
            session_id = new_session_id;
            dirty = true;
            return true;
        }

        void mark_dirty() {
            std::scoped_lock{mut};

            active_station = std::nullopt;
            dirty = true;
        }
    };
}

#endif