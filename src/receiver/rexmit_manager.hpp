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

        explicit rexmit_id(sikradio::common::msg_id_t id) : 
                id{id}, scheduled_update{std::chrono::system_clock::now()} {}

        rexmit_id(sikradio::common::msg_id_t id, std::chrono::system_clock::time_point scheduled_update) : 
                id{id}, scheduled_update{scheduled_update} {}

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
        std::chrono::system_clock::duration rtime;

    public:
        rexmit_manager() = delete;
        rexmit_manager(const rexmit_manager& other) = delete;
        rexmit_manager(rexmit_manager&& other) = delete;
        explicit rexmit_manager(size_t rtime_seconds) : rtime{std::chrono::seconds(rtime_seconds)} {}

        void append_ids(const std::set<sikradio::common::msg_id_t>& new_ids) {
            std::scoped_lock{mut};

            auto next_update = std::chrono::system_clock::now() + rtime;
            for (auto it = new_ids.begin(); it != new_ids.end(); it++) {
                rexmit_ids.emplace(*it, next_update);
            }
        }

        std::set<sikradio::common::msg_id_t> filter_get_ids(std::set<sikradio::common::msg_id_t> ids_to_forget) {
            std::scoped_lock{mut};
            // remove ids to forget from rexmit ids
            for (auto it = ids_to_forget.begin(); it != ids_to_forget.end(); it++) {
                rexmit_ids.erase(rexmit_id(*it));
            }
            // get ids scheduled to update between previous call and this one
            std::set<sikradio::common::msg_id_t> ids_to_update;
            auto this_update = std::chrono::system_clock::now();
            auto next_update = this_update + rtime;
            for (auto it = rexmit_ids.begin(); it != rexmit_ids.end(); /* update in body */) {
                if (it->scheduled_update > this_update) {
                    it++;
                    continue;
                }
                // id will be returned, next update has to be scheduled now
                auto id_to_rexmit = it->id;
                it = rexmit_ids.erase(it);
                ids_to_update.emplace(id_to_rexmit);
                rexmit_ids.emplace_hint(it, id_to_rexmit, next_update);
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
