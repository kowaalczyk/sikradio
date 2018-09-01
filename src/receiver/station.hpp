#ifndef SIKRADIO_RECEIVER_STATION_HPP
#define SIKRADIO_RECEIVER_STATION_HPP

#include <string>
#include <chrono>
#include <cstdint>
#include <netinet/in.h>

#include "../common/ctrl_msg.hpp"

namespace sikradio::receiver {
    struct station {
        std::string name;
        std::string ctrl_address;
        in_port_t ctrl_port;
        std::string data_address;
        std::chrono::system_clock::time_point last_reply;
        
        bool operator>(const station& other) const {return name > other.name;}
        bool operator>=(const station& other) const {return name >= other.name;}
        bool operator<=(const station& other) const {return name <= other.name;}
        bool operator<(const station& other) const {return name < other.name;}
        bool operator==(const station& other) const {
            bool names_equal = (name == other.name);
            bool ca_equal = (ctrl_address == other.ctrl_address);
            bool da_equal = (data_address == other.data_address);
            bool port_equal = (ctrl_port == other.ctrl_port);
            return (names_equal && da_equal && ca_equal && port_equal);
        }
    };
    typedef struct station station;

    station as_station(sikradio::common::ctrl_msg, struct sockaddr_in sender_address) {
        return station{};  // TODO
    }
}

#endif
