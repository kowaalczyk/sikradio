#ifndef SIKRADIO_RECEIVER_EXCEPTIONS_HPP
#define SIKRADIO_RECEIVER_EXCEPTIONS_HPP


#include <string>
#include <exception>

#include "../common/exceptions.hpp"

using base_exception = sikradio::common::exceptions::base_exception;

namespace sikradio::receiver::exceptions {
    class buffer_access_exception : public base_exception {
    public:
        explicit buffer_access_exception(std::string msg_str) : base_exception(std::move(msg_str)) {}
    };
}


#endif