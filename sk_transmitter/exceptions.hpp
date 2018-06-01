//
// Created by kowal on 31.05.18.
//

#ifndef SK_TRANSMITTER_EXCEPTIONS_HPP
#define SK_TRANSMITTER_EXCEPTIONS_HPP


#include <string>

namespace sk_transmitter {
    class lockable_cache_insert_exception : public std::exception {
    private:
        const std::string msg_str;

    public:
        explicit lockable_cache_insert_exception(std::string msg_str) : msg_str(std::move(msg_str)) {}

        const char *what() const noexcept override {
            return msg_str.c_str();
        }
    };

    class sk_socket_send_exception : public std::exception {
    private:
        const std::string msg_str;

    public:
        explicit sk_socket_send_exception(std::string msg_str) : msg_str(std::move(msg_str)) {}

        const char *what() const noexcept override {
            return msg_str.c_str();
        }
    };
}


#endif //SK_TRANSMITTER_EXCEPTIONS_HPP
