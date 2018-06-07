//
// Created by kowal on 31.05.18.
//

#ifndef SK_TRANSMITTER_EXCEPTIONS_HPP
#define SK_TRANSMITTER_EXCEPTIONS_HPP


#include <string>

namespace sk_transmitter {
    namespace exceptions {
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
}


#endif //SK_TRANSMITTER_EXCEPTIONS_HPP
