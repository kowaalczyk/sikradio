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

namespace sikradio::receiver {
    using buffer_access_exception = exceptions::buffer_access_exception;
    enum class buffer_state {NO_SESSION, WAITING, READABLE};

    class buffer {
    private:
        // buffer parameters
        std::mutex mut{};
        size_t max_size;
        size_t max_elements;
        // session and buffer state
        buffer_state state{buffer_state::NO_SESSION};
        sikradio::common::msg_id_t max_msg_id{};
        sikradio::common::msg_id_t byte_zero{};
        size_t package_size;  // deduced from first package in session
        // buffer contents
        std::deque<std::optional<sikradio::common::msg_t>> msg_vals{};
        std::deque<sikradio::common::msg_id_t> msg_ids{};

        void save_new_message(const sikradio::common::data_msg &msg) {
            if (msg_ids.empty()) {
                msg_vals.emplace_back(std::make_optional(msg.get_data()));
                msg_ids.emplace_back(msg.get_id());
                return;
            }
            // pop excessive elements and reserve empty space for missed messages
            for (auto missed_id = msg_ids.back() + package_size; 
                    missed_id < msg.get_id(); 
                    missed_id += package_size) {
                if (msg_ids.size() == max_elements) {
                    msg_ids.pop_front();
                    msg_vals.pop_front();
                }
                msg_vals.emplace_back(std::nullopt);
                msg_ids.emplace_back(missed_id);
            }
            // pop excessive element and save the message
            if (msg_ids.size() == max_elements) {
                msg_ids.pop_front();
                msg_vals.pop_front();
            }
            msg_vals.emplace_back(std::make_optional(msg.get_data()));
            msg_ids.emplace_back(msg.get_id());
        }

        void save_missed_message(const sikradio::common::data_msg &msg) {
            // assuming message already has allocated space in the buffer
            size_t msg_pos = (msg.get_id()-msg_ids.front()) / package_size;
            msg_vals[msg_pos] = std::make_optional(msg.get_data());
        }

        std::optional<sikradio::common::msg_t> optional_read() {
            if (state != buffer_state::READABLE)
                return std::nullopt;
            if (msg_vals.size() == 0 || !msg_vals.front().has_value())
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

        std::set<sikradio::common::msg_id_t>
        write_get_missed(const sikradio::common::data_msg &msg) {
            std::scoped_lock{mut};
            // update session and state if necessary
            if (state == buffer_state::NO_SESSION) {
                byte_zero = msg.get_id();
                max_msg_id = msg.get_id();
                package_size = msg.get_data().size();
                max_elements = max_size / package_size;
                state = buffer_state::WAITING;
            }
            if (msg.get_id() >= byte_zero + max_elements*package_size*3/4) {
                state = buffer_state::READABLE;
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
            // extract ids that were missed between last saved message and this one
            std::set<sikradio::common::msg_id_t> missed_ids;
            for (auto id = std::max(max_msg_id + package_size, msg_ids.front()); 
                    id < std::min(msg.get_id(), msg_ids.back()); 
                    id += package_size) {
                if (!msg_vals[id].has_value()) {
                    missed_ids.emplace(id);
                }
            }
            max_msg_id = msg.get_id();
            return missed_ids;
        }

        std::optional<sikradio::common::msg_t> try_read() {
            std::scoped_lock{mut};
            return optional_read();
        }

        bool has_space_for(const sikradio::common::msg_id_t id) {
            std::scoped_lock{mut};
            bool is_in_session = (state != buffer_state::NO_SESSION);
            bool has_space = (msg_ids.front() <= id && id <= msg_ids.back());
            return (is_in_session && has_space);
        }

        void reset() {
            std::scoped_lock{mut};
            state = buffer_state::NO_SESSION;
            msg_ids.clear();
            msg_vals.clear();
        }
    };
}

#endif
