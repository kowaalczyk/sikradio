#ifndef SIKRADIO_RECEIVER_REXMIT_MANAGER_HPP
#define SIKRADIO_RECEIVER_REXMIT_MANAGER_HPP

#include <set>
#include <mutex>
#include <chrono>

#include "../common/types.hpp"

namespace sikradio::receiver {
    struct rexmit_id {
        sikradio::common::msg_id_t id;
        std::chrono::system_clock::time_point scheduled_update;

        rexmit_id(sikradio::common::msg_id_t id, std::chrono::system_clock::time_point scheduled_update) : 
            id{id}, 
            scheduled_update{scheduled_update} {}

        bool operator>(const rexmit_id& other) const {return id > other.id;}
        bool operator>=(const rexmit_id& other) const {return id >= other.id;}
        bool operator<=(const rexmit_id& other) const {return id <= other.id;}
        bool operator<(const rexmit_id& other) const {return id < other.id;}
        bool operator==(const rexmit_id& other) const {return id == other.id;}
    };

    class rexmit_manager {
    private:
        std::mutex mut{};
        std::set<rexmit_id> rexmit_ids{};
        std::chrono::system_clock::time_point last_update = std::chrono::system_clock::now();
        std::chrono::system_clock::duration rtime{std::chrono::seconds(5)};

    public:
        rexmit_manager() = default;
        rexmit_manager(const rexmit_manager& other) = delete;
        rexmit_manager(rexmit_manager&& other) = delete;

        void append_ids(sikradio::common::msg_id_t begin, sikradio::common::msg_id_t end) {
            std::scoped_lock{mut};

            auto next_update = std::chrono::system_clock::now() + rtime;
            for (auto id = begin; id < end; id++) {
                rexmit_ids.emplace(id, next_update);
            }
        }

        std::set<sikradio::common::msg_id_t> filter_get_ids(std::set<sikradio::common::msg_id_t> ids_to_forget) {
            std::scoped_lock{mut};

            std::set<sikradio::common::msg_id_t> ids_to_update;
            auto this_update = std::chrono::system_clock::now();
            auto next_update = this_update + rtime;
            for (auto it = rexmit_ids.begin(); it != rexmit_ids.end(); /* update in body */) {
                it = rexmit_ids.erase(it);
                if (ids_to_forget.find(it->id) != ids_to_forget.end()) continue;
                // if id was not to be forget, we need to schedule next rexmit and insert it back
                ids_to_update.emplace(it->id);
                rexmit_ids.emplace_hint(it, it->id, next_update);
            }
            return ids_to_update;
        }

        void reset() {
            std::scoped_lock{mut};

            rexmit_ids.clear();
            last_update = std::chrono::system_clock::now();
        }
    };
}

#endif
