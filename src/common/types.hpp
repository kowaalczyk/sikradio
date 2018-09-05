#ifndef SIKRADIO_COMMON_TYPES_HPP
#define SIKRADIO_COMMON_TYPES_HPP


#include <cstdio>
#include <vector>
#include <cstdint>


namespace sikradio::common {
    typedef uint64_t msg_id_t;
    typedef char byte_t;
    typedef std::vector<byte_t> msg_t;
}


#endif //SIKRADIO_COMMON_TYPES_HPP
