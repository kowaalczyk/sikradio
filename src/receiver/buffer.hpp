#ifndef SIKRADIO_RECEIVER_BUFFER_HPP
#define SIKRADIO_RECEIVER_BUFFER_HPP

#include <mutex>
#include <deque>
#include <set>
#include <utility>
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

        size_t max_size;
        size_t max_elements;

        sikradio::common::msg_id_t max_msg_id{};
        sikradio::common::msg_id_t byte_zero{};
        size_t package_size;  // size of package deduced from first package in session
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
            for (auto missed_id = msg_ids.back() + package_size; missed_id < msg.get_id(); missed_id += package_size) {
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
            size_t msg_pos = (msg.get_id()-msg_ids.front()) / package_size;
            msg_vals[msg_pos] = std::make_optional(msg.get_data());
        }

        std::optional<sikradio::common::msg_t> optional_read() {
            if (state != sikradio::receiver::buffer_state::READABLE)
                return std::nullopt;
            if (!msg_vals.front().has_value())
                throw buffer_access_exception("Buffer is not readable during active session!");
            auto ret = msg_vals.front();
            msg_vals.pop_front();
            msg_ids.pop_front();
            return ret;
        }

    public:
        buffer() = delete;
        buffer(const buffer& other) = delete;
        buffer(buffer&& other) = delete;
        explicit buffer(size_t max_size) : max_size{max_size} {}

        std::pair<std::optional<sikradio::common::msg_t>, std::set<sikradio::common::msg_id_t>>
        write_try_read_get_rexmit(const sikradio::common::data_msg &msg) {
            std::scoped_lock{mut};

            // update session and state if necessary
            if (state == sikradio::receiver::buffer_state::NO_SESSION) {
                byte_zero = msg.get_id();
                max_msg_id = msg.get_id();
                package_size = msg.get_data().size();
                max_elements = max_size / package_size;
                state = sikradio::receiver::buffer_state::WAITING;
            }
            if (msg.get_id() >= byte_zero + max_elements*package_size*3/4) {
                state = sikradio::receiver::buffer_state::READABLE;
            }
            // save message to buffer
            if (msg.get_id() % package_size != 0) 
                throw buffer_access_exception("Id=" + std::to_string(msg.get_id()) + "must be divisible by package size=" + std::to_string(package_size));
            if (msg_ids.empty() || msg.get_id() > msg_ids.back()) {
                save_new_message(msg);
            } else if (msg_ids.front() <= msg.get_id() && msg.get_id() <= msg_ids.back()) {
                save_missed_message(msg);
            } else {
                // received message is so old that it will be ignored
            }
            // count missed ids
            std::set<sikradio::common::msg_id_t> missed_ids;
            for (auto id = std::max(max_msg_id + package_size, msg_ids.front()); 
                    id < std::min(msg.get_id(), msg_ids.back()); 
                    id += package_size) {
                if (!msg_vals[id].has_value()) {
                    missed_ids.emplace(id);
                }
            }
            max_msg_id = msg.get_id();
            // try read
            return std::make_pair(optional_read(), missed_ids);
        }

        std::optional<sikradio::common::msg_t> try_read() {
            std::scoped_lock{mut};
            return optional_read();
        }

        bool has_space_for(const sikradio::common::msg_id_t id) {
            std::scoped_lock{mut};

            bool is_in_session = (state != sikradio::receiver::buffer_state::NO_SESSION);
            bool has_space = (msg_ids.front() <= id && id <= msg_ids.back());
            return (is_in_session && has_space);
        }

        void reset() {
            std::scoped_lock{mut};

            state = sikradio::receiver::buffer_state::NO_SESSION;
            msg_ids.clear();
            msg_vals.clear();
        }
    };
}

#endif
