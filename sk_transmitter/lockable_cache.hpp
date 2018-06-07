//
// Created by kowal on 01.06.18.
//

#ifndef SIK_NADAJNIK_LOCKABLE_CACHE_HPP
#define SIK_NADAJNIK_LOCKABLE_CACHE_HPP


#include <vector>
#include <mutex>
#include "data_msg.hpp"
#include "exceptions.hpp"
#include "../lib/optional.hpp"


using nonstd::optional;  // TODO: Change in deployment
using nonstd::nullopt;  // TODO: Change in deployment


namespace sk_transmitter {
    class lockable_cache {
    private:
        std::mutex mut{};
        std::vector<sk_transmitter::data_msg> container{};
        size_t container_size;

        size_t internal_id(msg_id_t id) {
            return (id % container_size);
        }

    public:
        explicit lockable_cache(size_t container_size) : container_size(container_size) {
            container.reserve(container_size);
        }

        optional<sk_transmitter::data_msg> atomic_get(sk_transmitter::msg_id_t id) {
            if (id >= container.size()) return nullopt;

            std::lock_guard<std::mutex> lock(mut);
            return optional<sk_transmitter::data_msg>(container[internal_id(id)]);
        }

        void atomic_push(sk_transmitter::data_msg msg) {
            std::lock_guard<std::mutex> lock(mut);

            // just to prevent segfault when copying to uninitialized memory
            if (internal_id(msg.id) == container.size()) container.push_back(msg);
            container[internal_id(msg.id)] = msg;
        }
    };
}


#endif //SIK_NADAJNIK_LOCKABLE_CACHE_HPP
