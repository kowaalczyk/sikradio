#ifndef SIKRADIO_SENDER_LOCKABLE_QUEUE_HPP
#define SIKRADIO_SENDER_LOCKABLE_QUEUE_HPP

#include <mutex>
#include <queue>
#include <atomic>
#include <set>
#include "../common/data_msg.hpp"
#include <optional>

namespace sikradio::sender {
    using std::optional;
    using std::nullopt;

    class lockable_queue {
    private:
        std::mutex mut{};
        std::queue<sikradio::common::data_msg> q{};

    public:
        lockable_queue() = default;

        optional<sikradio::common::data_msg> atomic_get_and_pop() {
            std::scoped_lock{mut};

            if (q.empty()) return nullopt;
            auto ret = q.front();
            q.pop();
            return optional<sikradio::common::data_msg>(ret);
        }

        std::set<sikradio::common::data_msg> atomic_get_unique() {
            std::scoped_lock{mut};
            
            std::set<sikradio::common::data_msg> ret;
            while (!q.empty()) {
                ret.insert(q.front());
                q.pop();
            }
            return ret;
        }

        void atomic_push(sikradio::common::data_msg msg) {
            std::scoped_lock{mut};
            q.push(std::move(msg));
        }
    };
}

#endif //SIKRADIO_SENDER_LOCKABLE_QUEUE_HPP
