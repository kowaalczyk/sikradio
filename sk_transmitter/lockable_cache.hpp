//
// Created by kowal on 01.06.18.
//

#ifndef SIK_NADAJNIK_LOCKABLE_CACHE_HPP
#define SIK_NADAJNIK_LOCKABLE_CACHE_HPP


#include <vector>
#include <mutex>
#include "internal_msg.hpp"
#include "exceptions.hpp"
#include "../lib/optional.hpp"


using nonstd::optional;  // TODO: Change in deployment
using nonstd::nullopt;  // TODO: Change in deployment


namespace sk_transmitter {
    class lockable_cache {
    private:
        std::mutex mut{};
        std::vector<sk_transmitter::internal_msg> container{};
        size_t container_size;

        size_t internal_id(msg_id_t id) {
            return (id % container_size);
        }

    public:
        explicit lockable_cache(size_t container_size) : container_size(container_size) {
            container.reserve(container_size);
        }

        optional<sk_transmitter::internal_msg> atomic_get(sk_transmitter::msg_id_t id) {
            if (id >= container.size()) return nullopt;

            std::lock_guard<std::mutex> lock(mut);

            auto ret = container[internal_id(id)];
            ret.initial = false;  // never accept this message again
            return optional<sk_transmitter::internal_msg>(ret);
        }

        void atomic_push(sk_transmitter::internal_msg msg) {
            if (!msg.initial) {
                throw lockable_cache_insert_exception("Cannot atomic_push a non-initial message into cache");
            }

            std::lock_guard<std::mutex> lock(mut);

            container[internal_id(msg.id)] = std::move(msg);
        }
    };
}


#endif //SIK_NADAJNIK_LOCKABLE_CACHE_HPP
