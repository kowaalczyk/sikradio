#ifndef SIKRADIO_RECEIVER_EXCEPTIONS_HPP
#define SIKRADIO_RECEIVER_EXCEPTIONS_HPP

#include <string>
#include <exception>

#include "../common/exceptions.hpp"

namespace sikradio::receiver::exceptions {
    using base_exception = sikradio::common::exceptions::base_exception;

    class buffer_access_exception : public base_exception {
    public:
        explicit buffer_access_exception(std::string msg_str) : base_exception(std::move(msg_str)) {}
    };
}

#endif
