//
// Created by kowal on 31.05.18.
//

#ifndef SK_TRANSMITTER_INTERNAL_MSG_HPP
#define SK_TRANSMITTER_INTERNAL_MSG_HPP


#include <utility>
#include "types.hpp"

namespace sk_transmitter {
    class internal_msg {
    public:
        bool initial{false};
        sk_transmitter::msg_id_t id{};
        sk_transmitter::msg_t data{};

        internal_msg(msg_id_t id, msg_t data) : id{id},
                                                data{std::move(data)} {}

        internal_msg(bool initial, msg_t data) : initial{initial},
                                                 data{std::move(data)} {}

        msg_t sendable_with_session_id(msg_id_t session_id) {
            return sk_transmitter::msg_t();  // TODO: Return as byte array that can be written to socket
        }
    };
}


#endif //SK_TRANSMITTER_INTERNAL_MSG_HPP
