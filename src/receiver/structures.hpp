#ifndef SIKRADIO_RECEIVER_STRUCTURES_HPP
#define SIKRADIO_RECEIVER_STRUCTURES_HPP

#include <string>
#include <tuple>
#include <chrono>
#include <cstdint>
#include <netinet/in.h>

#include "../common/ctrl_msg.hpp"

namespace sikradio::receiver::structures {
    enum class menu_selection_update {UP, DOWN};

    struct station {
        std::string name;
        std::string ctrl_address;
        in_port_t ctrl_port;
        std::string data_address;
        in_port_t data_port;
        std::chrono::system_clock::time_point last_reply;
        
        bool operator>(const station& other) const {return name > other.name;}
        bool operator>=(const station& other) const {return name >= other.name;}
        bool operator<=(const station& other) const {return name <= other.name;}
        bool operator<(const station& other) const {return name < other.name;}
        bool operator==(const station& other) const {
            bool names_equal = (name == other.name);
            bool ca_equal = (ctrl_address == other.ctrl_address);
            bool da_equal = (data_address == other.data_address);
            bool cp_equal = (ctrl_port == other.ctrl_port);
            bool dp_equal = (data_port == other.data_port);
            return (names_equal && da_equal && ca_equal && cp_equal && dp_equal);
        }
        bool operator!=(const station& other) const {return !((*this) == other);}
    };
    typedef struct station station;

    station as_station(
            sikradio::common::ctrl_msg msg, 
            struct sockaddr_in sender_address) {
        station s;
        std::string name;
        std::string data_addr;
        in_port_t data_port;
        std::string ctrl_addr;

        ctrl_addr = inet_ntoa(sender_address.sin_addr);
        std::tie(name, data_addr, data_port) = msg.get_reply_data();
        
        s.name = name;
        s.data_address = data_addr;
        s.ctrl_address = ctrl_addr;
        s.ctrl_port = ntohs(sender_address.sin_port);
        s.data_port = data_port;
        s.last_reply = std::chrono::system_clock::now();
        return s;
    }
}

#endif
