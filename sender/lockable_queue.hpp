#ifndef SIKRADIO_SENDER_LOCKABLE_QUEUE_HPP
#define SIKRADIO_SENDER_LOCKABLE_QUEUE_HPP


#include <mutex>
#include <queue>
#include <atomic>
#include <set>
#include "data_msg.hpp"
#include <optional>


using std::optional;
using std::nullopt;


namespace sender {
    class lockable_queue {
    private:
        std::mutex mut{};
        std::queue<sender::data_msg> q{};

    public:
        lockable_queue() = default;

        optional<sender::data_msg> atomic_get_and_pop() {
            if (q.empty()) return nullopt;

            std::lock_guard<std::mutex> lock(mut);

            auto ret = q.front();
            q.pop();
            return optional<sender::data_msg>(ret);
        }

        std::set<sender::data_msg> atomic_get_unique() {
            std::set<sender::data_msg> ret;

            std::lock_guard<std::mutex> lock(mut);

            while(!q.empty()) {
                ret.insert(q.front());
                q.pop();
            }
            return ret;
        }

        void atomic_push(sender::data_msg msg) {
            std::lock_guard<std::mutex> lock(mut);
            q.push(std::move(msg));
        }
    };
}


#endif //SIKRADIO_SENDER_LOCKABLE_QUEUE_HPP
