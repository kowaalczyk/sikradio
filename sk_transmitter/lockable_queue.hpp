//
// Created by kowal on 01.06.18.
//

#ifndef SIK_NADAJNIK_LOCKABLE_QUEUE_HPP
#define SIK_NADAJNIK_LOCKABLE_QUEUE_HPP


#include <mutex>
#include <queue>
#include <atomic>
#include "internal_msg.hpp"


namespace sk_transmitter {
    class lockable_queue {
    private:
        std::mutex mut{};
        std::queue<sk_transmitter::internal_msg> q{};

    public:
        lockable_queue() = default;

        sk_transmitter::internal_msg atomic_get_and_pop() {  // TODO: If queue is empty, sleep and retry
            while(q.empty())
                ;

            std::lock_guard<std::mutex> lock(mut);

            auto ret = q.front();
            q.pop();
            return ret;
        }

        void atomic_push(sk_transmitter::internal_msg &msg) {
            std::lock_guard<std::mutex> lock(mut);
            q.push(msg);
        }
    };
}


#endif //SIK_NADAJNIK_LOCKABLE_QUEUE_HPP
