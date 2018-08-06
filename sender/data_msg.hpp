#ifndef SIKRADIO_SENDER_INTERNAL_MSG_HPP
#define SIKRADIO_SENDER_INTERNAL_MSG_HPP


#include <utility>
#include <netinet/in.h>
#include <cstring>
#include "types.hpp"


namespace sender {
    class data_msg {
    private:
        uint64_t htonll(uint64_t value) {
            static const int num = 2137;

            if (*reinterpret_cast<const byte_t *>(&num) == num) {
                const uint32_t high_part = htonl(static_cast<uint32_t>(value >> 32));
                const uint32_t low_part = htonl(static_cast<uint32_t>(value & 0xFFFFFFFFLL));

                return (static_cast<uint64_t>(low_part) << 32) | high_part;
            } else {
                return value;
            }
        }

    public:
        sender::msg_id_t id{};
        sender::msg_t data{};

        data_msg(msg_id_t id, msg_t data) : id{id},
                                                data{std::move(data)} {}

        msg_t sendable_with_session_id(msg_id_t session_id) {
            byte_t msg[data.size() + 2 * sizeof(uint64_t)];

            auto net_session_id = htonll(session_id);
            auto net_id = htonll(id);

            memcpy(msg, reinterpret_cast<const void *>(&net_session_id), sizeof(uint64_t));
            memcpy(msg + sizeof(uint64_t), reinterpret_cast<const void *>(&net_id), sizeof(uint64_t));
            memcpy(msg + 2 * sizeof(uint64_t), data.data(), data.size());

            msg_t ret;
            ret.assign(msg, msg + sizeof(msg));
            return ret;
        }

        bool operator<(const data_msg &rhs) const {
            return id < rhs.id;
        }

        bool operator>(const data_msg &rhs) const {
            return rhs < *this;
        }

        bool operator<=(const data_msg &rhs) const {
            return !(rhs < *this);
        }

        bool operator>=(const data_msg &rhs) const {
            return !(*this < rhs);
        }
    };
}


#endif //SIKRADIO_SENDER_INTERNAL_MSG_HPP
