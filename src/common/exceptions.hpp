#ifndef SIKRADIO_COMMON_EXCEPTIONS_HPP
#define SIKRADIO_COMMON_EXCEPTIONS_HPP

#include <string>
#include <exception>

namespace sikradio::common::exceptions {
    class socket_exception : public std::exception {
    private:
        const std::string msg_str;

    public:
        explicit socket_exception(std::string msg_str) : msg_str(std::move(msg_str)) {}

        const char *what() const noexcept override {
            return msg_str.c_str();
        }
    };
}

#endif
