#ifndef SIKRADIO_COMMON_CTRL_MSG_HPP
#define SIKRADIO_COMMON_CTRL_MSG_HPP


#include <utility>
#include <regex>
#include <netinet/in.h>
#include <cstring>
#include <cassert>

#include "types.hpp"


using std::optional;
using std::nullopt;


namespace sikradio::common {
    const std::string lookup_msg_key = "ZERO_SEVEN_COME_IN";
    const std::string rexmit_msg_key = "LOUDER_PLEASE ";

    class ctrl_msg {
    private:
        std::string msg_data{};
        struct sockaddr_in sender_address{};

    public:
        ctrl_msg(
            std::string msg_data, 
            const sockaddr_in &sender_address) : msg_data(std::move(msg_data)),
                                                 sender_address(sender_address) {}

        bool is_lookup() const {
            return (msg_data.find(lookup_msg_key) == 0);
        }

        bool is_rexmit() const {
            return (msg_data.find(rexmit_msg_key) == 0);
        }

        std::vector<sikradio::common::msg_id_t> get_rexmit_ids() const {
            assert(is_rexmit());

            std::string searchable(msg_data.begin()+(rexmit_msg_key.length()-1), msg_data.end());
            searchable[0] = ',';  // now all valid numbers will be preceeded by a comma

            std::regex r(",[0-9]+");
            std::vector<sikradio::common::msg_id_t> ids;

            for (std::sregex_iterator i = std::sregex_iterator(
                        searchable.begin(), 
                        searchable.end(), 
                        r);
                    i != std::sregex_iterator();
                    i++) {
                std::string m_str = (*i).str();
                std::string num_str(m_str.begin()+1, m_str.end());  // remove comma

                ids.push_back(std::stoull(num_str));
            }

            return ids;
        }

        sockaddr_in get_sender_address() const {
            return sender_address;
        }
    };
}


#endif //SIKRADIO_COMMON_CTRL_MSG_HPP
