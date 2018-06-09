//
// Created by kowal on 01.06.18.
//

#ifndef SIK_NADAJNIK_LOCKABLE_QUEUE_HPP
#define SIK_NADAJNIK_LOCKABLE_QUEUE_HPP


#include <mutex>
#include <queue>
#include <atomic>
#include <set>
#include "data_msg.hpp"
#include "../lib/optional.hpp"


using nonstd::optional;  // TODO: Change in deployment
using nonstd::nullopt;  // TODO: Change in deployment


namespace sk_transmitter {
    class lockable_queue {
    private:
        std::mutex mut{};
        std::queue<sk_transmitter::data_msg> q{};

    public:
        lockable_queue() = default;

        optional<sk_transmitter::data_msg> atomic_get_and_pop() {
            if (q.empty()) return nullopt;

            std::lock_guard<std::mutex> lock(mut);

            auto ret = q.front();
            q.pop();
            return optional<sk_transmitter::data_msg>(ret);
        }

        std::set<sk_transmitter::data_msg> atomic_get_unique() {
            std::set<sk_transmitter::data_msg> ret;

            std::lock_guard<std::mutex> lock(mut);

            while(!q.empty()) {
                ret.insert(q.front());
                q.pop();
            }
            return ret;
        }

        void atomic_push(sk_transmitter::data_msg msg) {
            std::lock_guard<std::mutex> lock(mut);
            q.push(std::move(msg));
        }
    };
}


#endif //SIK_NADAJNIK_LOCKABLE_QUEUE_HPP
