#ifndef SIKRADIO_SENDER_LOCKABLE_CACHE_HPP
#define SIKRADIO_SENDER_LOCKABLE_CACHE_HPP


#include <vector>
#include <mutex>
#include "../common/data_msg.hpp"
#include <optional>


using std::optional;
using std::nullopt;


namespace sikradio::sender {
    class lockable_cache {
    private:
        std::mutex mut{};
        std::vector<sikradio::common::data_msg> container{};
        size_t container_size;

        size_t internal_id(sikradio::common::msg_id_t id) {
            return (id % container_size);
        }

    public:
        explicit lockable_cache(size_t container_size) : container_size(container_size) {
            container.reserve(container_size);
        }

        optional<sikradio::common::data_msg> atomic_get(sikradio::common::msg_id_t id) {
            if (id >= container.size()) return nullopt;

            std::scoped_lock{mut};
            return optional<sikradio::common::data_msg>(container[internal_id(id)]);
        }

        void atomic_push(sikradio::common::data_msg msg) {
            std::scoped_lock{mut};

            // just to prevent segfault when copying to uninitialized memory
            if (internal_id(msg.get_id()) == container.size()) container.push_back(msg);
            container[internal_id(msg.get_id())] = msg;
        }
    };
}


#endif //SIKRADIO_SENDER_LOCKABLE_CACHE_HPP
