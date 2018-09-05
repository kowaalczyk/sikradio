#ifndef SIKRADIO_COMMON_EXCEPTIONS_HPP
#define SIKRADIO_COMMON_EXCEPTIONS_HPP

#include <string>
#include <exception>

namespace sikradio::common::exceptions {
    class base_exception : public std::exception {
    private:
        std::string msg_str;

    public:
        explicit base_exception(std::string&& msg_str) : msg_str(std::move(msg_str)) {}

        const char *what() const noexcept override {
            return msg_str.c_str();
        }
    };

    class socket_exception : public base_exception {
    public:
        explicit socket_exception(std::string msg_str) : base_exception(std::move(msg_str)) {}
    };

    class address_exception : public base_exception {
    public:
        explicit address_exception(std::string msg_str) : base_exception(std::move(msg_str)) {}
    };

    class ctrl_msg_exception : public base_exception {
    public:
        explicit ctrl_msg_exception(std::string msg_str) : base_exception(std::move(msg_str)) {}
    };

    class data_msg_exception : public base_exception {
    public:
        explicit data_msg_exception(std::string msg_str) : base_exception(std::move(msg_str)) {}
    };
}

#endif
