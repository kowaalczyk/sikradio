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
#include <optional>
#include "ctrl_msg.hpp"
#include "types.hpp"
#include <regex>


using std::optional;
using std::nullopt;


namespace sender {
    const std::string lookup_msg_key = "ZERO_SEVEN_COME_IN";
    const std::string rexmit_msg_key = "LOUDER_PLEASE ";

    class ctrl_msg {
    private:
        std::string msg_data{};
        struct sockaddr_in sender_address{};

    public:
        ctrl_msg(std::string msg_data, const sockaddr_in &sender_address) : msg_data(std::move(msg_data)),
                                                                                   sender_address(sender_address) {}

        bool is_lookup() {
            return (msg_data.find(lookup_msg_key) == 0);
        }

        bool is_rexmit() {
            return (msg_data.find(rexmit_msg_key) == 0);
        }

        optional<std::vector<msg_id_t>> get_rexmit_ids() {
            if (!is_rexmit()) return nullopt;

            std::string searchable(msg_data.begin()+(rexmit_msg_key.length()-1), msg_data.end());
            searchable[0] = ',';  // now all valid numbers will be preceeded by a comma

            std::regex r(",[0-9]+");
            std::vector<msg_id_t> ids;

            for (std::sregex_iterator i = std::sregex_iterator(searchable.begin(), searchable.end(), r);
                    i != std::sregex_iterator();
                    i++) {
                std::string m_str = (*i).str();
                std::string num_str(m_str.begin()+1, m_str.end());  // remove comma

                ids.push_back(std::stoull(num_str));
            }

            return optional<std::vector<msg_id_t>>(ids);
        }

        sockaddr_in get_sender_address() const {
            return sender_address;
        }
    };
}


#endif //SIK_NADAJNIK_CTRL_MSG_HPP
