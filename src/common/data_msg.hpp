#ifndef SIKRADIO_COMMON_INTERNAL_MSG_HPP
#define SIKRADIO_COMMON_INTERNAL_MSG_HPP


#include <optional>
#include <utility>
#include <netinet/in.h>
#include <cstring>
#include <cassert>

#include "types.hpp"

namespace {
    uint64_t htonll(uint64_t value) {
        static const int num = 2137;

        if (*reinterpret_cast<const sikradio::common::msg_id_t *>(&num) == num) {
            const uint32_t high_part = htonl(static_cast<uint32_t>(value >> 32));
            const uint32_t low_part = htonl(static_cast<uint32_t>(value & 0xFFFFFFFFLL));

            return (static_cast<uint64_t>(low_part) << 32) | high_part;
        } else {
            return value;
        }
    }

    uint64_t ntohll(uint64_t value) {
        // TODO
        return 0;
    }
}

namespace sikradio::common {
    class data_msg {
    private:
        sikradio::common::msg_id_t id;
        std::optional<sikradio::common::msg_id_t> session_id;
        std::optional<sikradio::common::msg_t> data;

    public:
        data_msg() = delete;
        data_msg(const data_msg& other) = default;
        
        explicit data_msg(msg_id_t id) : id{id}, 
                                         session_id{std::nullopt}, 
                                         data{std::nullopt} {}

        data_msg(msg_id_t id, msg_t data) : id{id},
                                            session_id{std::nullopt},
                                            data{std::optional<msg_t>(std::move(data))} {}
        
        data_msg(msg_id_t id, msg_id_t session_id, msg_t data) : id{id},
                                                                 session_id{std::optional<msg_id_t>(session_id)}, 
                                                                 data{std::optional<msg_t>(std::move(data))} {}
        
        data_msg(const sikradio::common::byte_t *raw_msg) {
            msg_id_t id;
            msg_id_t session_id;
            msg_t data;

            memcpy(&id, raw_msg, sizeof(msg_id_t));
            id = ntohll(id);
            memcpy(&session_id, raw_msg+sizeof(msg_id_t), sizeof(msg_id_t));
            session_id = ntohll(session_id);
            data.assign(raw_msg + 2*sizeof(msg_id_t), raw_msg + sizeof(raw_msg) - 2*sizeof(msg_id_t) - 1);
            
            this->id = id;
            this->session_id = std::optional<msg_id_t>(session_id);
            this->data = std::optional<msg_t>(data);
        }

        void set_data(msg_t data) {
            this->data = std::optional<msg_t>(data);
        }

        void set_session_id(msg_id_t session_id) {
            this->session_id = std::optional<msg_id_t>(session_id);
        }

        msg_id_t get_id() const {
            return id;
        }

        msg_id_t get_session_id() const {
            return session_id.value();
        }

        const msg_t& get_data() const {
            return data.value();
        }

        msg_t sendable() const {
            assert(session_id.has_value() && data.has_value());

            auto net_session_id = htonll(session_id.value());
            auto net_id = htonll(id);

            sikradio::common::byte_t msg[data.value().size() + 2*sizeof(msg_id_t)];
            memcpy(msg, reinterpret_cast<const void *>(&net_session_id), sizeof(msg_id_t));
            memcpy(msg + sizeof(msg_id_t), reinterpret_cast<const void *>(&net_id), sizeof(msg_id_t));
            memcpy(msg + 2*sizeof(msg_id_t), data.value().data(), data.value().size());

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


#endif //SIKRADIO_COMMON_INTERNAL_MSG_HPP
