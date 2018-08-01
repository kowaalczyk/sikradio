#ifndef SIKRADIO_SENDER_LOCKABLE_CACHE_HPP
#define SIKRADIO_SENDER_LOCKABLE_CACHE_HPP


#include <vector>
#include <mutex>
#include "data_msg.hpp"
#include "exceptions.hpp"
#include <optional>


using std::optional;
using std::nullopt;


namespace sender {
    class lockable_cache {
    private:
        std::mutex mut{};
        std::vector<sender::data_msg> container{};
        size_t container_size;

        size_t internal_id(msg_id_t id) {
            return (id % container_size);
        }

    public:
        explicit lockable_cache(size_t container_size) : container_size(container_size) {
            container.reserve(container_size);
        }

        optional<sender::data_msg> atomic_get(sender::msg_id_t id) {
            if (id >= container.size()) return nullopt;

            std::lock_guard<std::mutex> lock(mut);
            return optional<sender::data_msg>(container[internal_id(id)]);
        }

        void atomic_push(sender::data_msg msg) {
            std::lock_guard<std::mutex> lock(mut);

            // just to prevent segfault when copying to uninitialized memory
            if (internal_id(msg.id) == container.size()) container.push_back(msg);
            container[internal_id(msg.id)] = msg;
        }
    };
}


#endif //SIKRADIO_SENDER_LOCKABLE_CACHE_HPP
