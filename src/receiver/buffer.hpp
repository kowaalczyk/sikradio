#ifndef SIKRADIO_RECEIVER_BUFFER_HPP
#define SIKRADIO_RECEIVER_BUFFER_HPP

#include <mutex>
#include <deque>
#include <optional>

#include "../common/data_msg.hpp"
#include "../common/types.hpp"
#include "exceptions.hpp"

using buffer_access_exception = sikradio::receiver::exceptions::buffer_access_exception;

namespace sikradio::receiver {
    enum class buffer_state {NO_SESSION, WAITING, READABLE};

    class buffer {
    private:
        std::mutex mut{};

        size_t max_elements;

        sikradio::common::msg_id_t byte_zero{};
        sikradio::receiver::buffer_state state{sikradio::receiver::buffer_state::NO_SESSION};
        std::deque<std::optional<sikradio::common::msg_t>> msg_vals{};
        std::deque<sikradio::common::msg_id_t> msg_ids{};

        void save_new_message(const sikradio::common::data_msg &msg) {
            if (msg_ids.empty()) {
                msg_vals.emplace_back(std::make_optional(msg.get_data()));
                msg_ids.emplace_back(msg.get_id());
                return;
            }
            // reserve empty space for missed messages and pop excessive elements
            for (auto missed_id = msg_ids.back() + 1; missed_id < msg.get_id(); missed_id++) {
                if (msg_ids.size() == max_elements) {
                    msg_ids.pop_front();
                    msg_vals.pop_front();
                }
                msg_vals.emplace_back(std::nullopt);
                msg_ids.emplace_back(missed_id);
            }
            if (msg_ids.size() == max_elements) {
                msg_ids.pop_front();
                msg_vals.pop_front();
            }
            msg_vals.emplace_back(std::make_optional(msg.get_data()));
            msg_ids.emplace_back(msg.get_id());
        }

        void save_missed_message(const sikradio::common::data_msg &msg) {
            // message already has allocated space in the buffer - no need to pop elements
            size_t msg_pos = msg.get_id() - msg_ids.front();
            msg_vals[msg_pos] = std::make_optional(msg.get_data());
        }

    public:
        buffer() = delete;
        explicit buffer(size_t max_elements) : max_elements(max_elements) {}

        void write(const sikradio::common::data_msg &msg) {
            std::scoped_lock{mut};
            // update session and state if necessary
            if (state == sikradio::receiver::buffer_state::NO_SESSION) {
                byte_zero = msg.get_id();
                state = sikradio::receiver::buffer_state::WAITING;
            }
            if (msg.get_id() >= byte_zero + max_elements*3/4) {
                state = sikradio::receiver::buffer_state::READABLE;
            }
            // save message to buffer
            if (msg_ids.empty() || msg.get_id() > msg_ids.back()) {
                save_new_message(msg);
            } else if (msg_ids.front() <= msg.get_id() && msg.get_id() <= msg_ids.back()) {
                save_missed_message(msg);
            } else {
                // received message is so old that it will be ignored
            }
        }

        std::optional<sikradio::common::msg_t> try_read() {
            std::scoped_lock{mut};

            if (state != sikradio::receiver::buffer_state::READABLE)
                return std::nullopt;
            if (!msg_vals.front().has_value())
                throw buffer_access_exception("Buffer is not readable during active session!");
            auto ret = std::move(msg_vals.front());
            msg_vals.pop_front();
            msg_ids.pop_front();
            return ret;
        }

        bool has_space_for(const sikradio::common::msg_id_t id) {
            std::scoped_lock{mut};

            bool is_in_session = (state != sikradio::receiver::buffer_state::NO_SESSION);
            bool has_space = (msg_ids.front() <= id && id <= msg_ids.back());
            return (is_in_session && has_space);
        }

        void reset() {
            std::scoped_lock{mut};

            msg_ids = std::deque<sikradio::common::msg_id_t>();
            msg_vals = std::deque<std::optional<sikradio::common::msg_t>>();
            state = sikradio::receiver::buffer_state::NO_SESSION;
        }
    };
}

#endif
