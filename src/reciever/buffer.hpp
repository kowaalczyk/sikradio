#ifndef SIKRADIO_RECEIVER_BUFFER_HPP
#define SIKRADIO_RECEIVER_BUFFER_HPP


#include <mutex>
#include <deque>
#include <optional>
#include <cassert>

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
        size_t available_elements{};

        sikradio::common::msg_id_t session_id{};
        sikradio::common::msg_id_t byte_zero{};
        sikradio::receiver::buffer_state state{sikradio::receiver::buffer_state::NO_SESSION};
        std::deque<std::optional<sikradio::common::msg_t>> msg_vals{};
        std::deque<sikradio::common::msg_id_t> msg_ids{};

        void save_new_message(const sikradio::common::data_msg &msg) {
            assert(msg.get_session_id() == session_id);
            assert(msg.get_id() > msg_ids.back());

            for (auto missed_id = msg_ids.back(); missed_id < msg.get_id(); missed_id++) {
                if (msg_ids.size() >= max_elements) {
                    msg_ids.pop_front();
                    msg_vals.pop_front();
                }
                msg_vals.emplace_back(std::nullopt);
                msg_ids.emplace_back(missed_id);
            }
            msg_vals.emplace_back(std::optional<sikradio::common::msg_t>(msg.get_data()));
            msg_ids.emplace_back(msg.get_id());
        }

        void save_missed_message(const sikradio::common::data_msg &msg) {
            assert(msg.get_session_id() == session_id);
            assert(msg_ids.front() <= msg.get_id() <= msg_ids.back());

            size_t msg_pos = msg.get_id() - msg_ids.front();
            msg_vals[msg_pos] = std::optional<sikradio::common::msg_t>(msg.get_data());
        }

    public:
        buffer() = delete;
        explicit buffer(size_t max_elements) : max_elements(max_elements) {}

        void write(const sikradio::common::data_msg &msg) {
            std::lock_guard<std::mutex> lock(mut);
            // update session and state if necessary
            if (msg.get_session_id() > this->session_id) {
                reset_session();
            }
            if (state == sikradio::receiver::buffer_state::NO_SESSION) {
                session_id = msg.get_session_id();
                byte_zero = msg.get_id();
                state = sikradio::receiver::buffer_state::WAITING;
            }
            if (msg.get_id() > byte_zero + max_elements*3/4) {
                state = sikradio::receiver::buffer_state::READABLE;
            }
            // save message to buffer
            if (msg_ids.empty() || msg.get_id() > msg_ids.back()) {
                save_new_message(msg);
            } else if (msg_ids.front() <= msg.get_id() <= msg_ids.back()) {
                save_missed_message(msg);
            } else {
                // received message is so old that it will be ignored
            }
        }

        sikradio::common::msg_t read() {
            std::lock_guard<std::mutex> lock(mut);
            if (!msg_vals.front().has_value()) throw buffer_access_exception(
                "Invalid read: missing front value in buffer queue"
            );
            auto ret = std::move(msg_vals.front().value());
            msg_vals.pop_front();
            msg_ids.pop_back();
            return ret;
        }

        void reset_session() {  // TODO: Move content to #write unless it is used elsewhere
            msg_ids = std::deque<sikradio::common::msg_id_t>();
            msg_vals = std::deque<std::optional<sikradio::common::msg_t>>();
            // TODO: Might need to change group address
            state = sikradio::receiver::buffer_state::NO_SESSION;
        }

        bool is_readable() {
            std::lock_guard<std::mutex> lock(mut);
            return (state == sikradio::receiver::buffer_state::READABLE);
        }

        bool exists_space(const sikradio::common::msg_id_t id) const {
            return (state == sikradio::receiver::buffer_state::READABLE && msg_ids.front() <= id <= msg_ids.back());
        }
    };
}


#endif