#ifndef SIKRADIO_COMMON_CTRL_MSG_HPP
#define SIKRADIO_COMMON_CTRL_MSG_HPP

#include <vector>
#include <set>
#include <regex>
#include <utility>
#include <cstring>
#include <string>
#include <netinet/in.h>

#include "exceptions.hpp"
#include "types.hpp"

namespace sikradio::common {
    namespace {
        const std::string lookup_msg_key = "ZERO_SEVEN_COME_IN";
        const std::string reply_msg_key = "BOREWICZ_HERE ";
        const std::string rexmit_msg_key = "LOUDER_PLEASE ";
    }

    using ctrl_msg_exception = exceptions::ctrl_msg_exception;

    class ctrl_msg {
    private:
        std::string msg_data{};

    public:
        ctrl_msg() = default;
        ctrl_msg(const ctrl_msg& other) = default;

        explicit ctrl_msg(std::string msg_data) : msg_data{std::move(msg_data)} {}

        bool is_lookup() const {
            return (msg_data.find(lookup_msg_key) == 0);
        }

        bool is_reply() const {
            return (msg_data.find(reply_msg_key) == 0);
        }

        bool is_rexmit() const {
            return (msg_data.find(rexmit_msg_key) == 0);
        }

        std::vector<sikradio::common::msg_id_t> get_rexmit_ids() const {
            if (!is_rexmit()) 
                throw ctrl_msg_exception("Trying to read rexmit ids from non-rexmit message");

            std::string searchable(msg_data.begin()+(rexmit_msg_key.length()-1), msg_data.end());
            searchable[0] = ',';
            // now all valid numbers will be preceeded by a comma
            std::regex r(",[0-9]+");
            std::vector<sikradio::common::msg_id_t> ids;
            for (auto i = std::sregex_iterator(searchable.begin(), searchable.end(), r);
                    i != std::sregex_iterator();
                    i++) {
                std::string m_str = (*i).str();
                std::string num_str(m_str.begin()+1, m_str.end());  // remove the comma
                ids.push_back(std::stoull(num_str));
            }
            return ids;
        }

        std::tuple<std::string, std::string, in_port_t> get_reply_data() const {
            if (!is_reply()) 
                throw ctrl_msg_exception("Trying to read reply data from a non-reply message");
            
            std::string addr;
            std::string name;
            in_port_t port;
            // putting space in the non-capturing group to separate spaces from content
            std::regex r("(?: )(\\S+)");
            auto i = std::sregex_iterator(msg_data.begin(), msg_data.end(), r);
            addr = (*i)[1];  // 1st capturing group in regex
            i++;
            port = static_cast<in_port_t>(std::stoul((*i)[1]));
            i++;
            name += (*i)[1];  // 1st word in the name
            i++;
            for (; i != std::sregex_iterator(); i++) {
                name += " ";
                name += (*i)[1];  // following words in the name
            }
            return std::make_tuple(name, addr, port);
        }

        std::string sendable() const {
            return msg_data;
        }
    };

    ctrl_msg make_lookup() {
        return ctrl_msg(lookup_msg_key + "\n");
    }

    ctrl_msg make_reply(
            const std::string &name, 
            const std::string &mcast_addr, 
            in_port_t data_port) {
        std::ostringstream ss;
        ss << reply_msg_key;
        ss << mcast_addr;
        ss << " ";
        ss << std::to_string(data_port);
        ss << " ";
        ss << name;
        ss << "\n";
        return ctrl_msg(ss.str());
    }

    ctrl_msg make_rexmit(std::set<sikradio::common::msg_id_t> ids) {
        std::ostringstream ss;
        for (auto it = ids.begin(); it != ids.end(); it++) {
            if (it != ids.begin()) ss << ",";
            ss << std::to_string(*it);
        }
        return ctrl_msg(rexmit_msg_key + ss.str() + "\n");
    }
}

#endif //SIKRADIO_COMMON_CTRL_MSG_HPP
