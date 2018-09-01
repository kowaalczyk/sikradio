#ifndef SIKRADIO_RECEIVER_STATE_MANAGER_HPP
#define SIKRADIO_RECEIVER_STATE_MANAGER_HPP

#include <string>
#include <optional>
#include <mutex>
#include <atomic>
#include <netinet/in.h>

#include "../common/types.hpp"

namespace sikradio::receiver {
    class state_manager {
    private:
        std::mutex mut{};
        std::optional<std::string> multicast_address{std::nullopt};
        std::atomic_bool dirty{false};
        sikradio::common::msg_id_t session_id{0};

    public:
        state_manager() = default;
        state_manager(const state_manager& other) = delete;
        state_manager(state_manager&& other) = delete;

        std::tuple<std::optional<std::string>, in_port_t> check_state() {
            std::scoped_lock{mut};

            bool was_dirty = dirty;
            dirty = false;
            return std::make_tuple(multicast_address, was_dirty);
        }

        void register_address(std::string new_multicast_address) {
            std::scoped_lock{mut};

            if (!multicast_address.has_value() || multicast_address.value() != new_multicast_address) {
                multicast_address = std::make_optional(new_multicast_address);
                dirty = true;
            }
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
            dirty = true;
        }
    };
}

#endif