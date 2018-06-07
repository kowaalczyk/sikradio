//
// Created by kowal on 02.06.18.
//

#ifndef SIK_NADAJNIK_CTRL_MSG_HPP
#define SIK_NADAJNIK_CTRL_MSG_HPP


#include <sys/socket.h>
#include <cstring>
#include <utility>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <zconf.h>
#include <vector>
#include "exceptions.hpp"
#include "../lib/optional.hpp"
#include "ctrl_msg.hpp"
#include "types.hpp"


using nonstd::optional;
using nonstd::nullopt;


namespace sk_transmitter {
    const std::string lookup_msg_key = "ZERO_SEVEN_COME_IN";
    const std::string rexmit_msg_key = "LOUDER_PLEASE ";

    class ctrl_msg {
    private:
        std::string msg_data{};
        struct sockaddr_in sender_address{};

    public:
        ctrl_msg(const char *msg_data, const sockaddr_in &sender_address) : msg_data(msg_data),
                                                                                   sender_address(sender_address) {}

        bool is_lookup() {
            return (msg_data.find(lookup_msg_key) == 0);
        }

        bool is_rexmit() {
            return (msg_data.find(rexmit_msg_key) == 0);
        }

        optional<std::vector<msg_id_t>> get_rexmit_ids() {
            if (!is_rexmit()) return nullopt;

            // TODO: Finish this and test retransmission, address TODOs related to listener in main
            return optional<std::vector<msg_id_t>>(std::vector<msg_id_t>());
        }

        const sockaddr_in &get_sender_address() const {
            return sender_address;
        }
    };
}


#endif //SIK_NADAJNIK_CTRL_MSG_HPP
