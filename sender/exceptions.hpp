#ifndef SIKRADIO_SENDER_EXCEPTIONS_HPP
#define SIKRADIO_SENDER_EXCEPTIONS_HPP


#include <string>


namespace sender {
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


#endif //SIKRADIO_SENDER_EXCEPTIONS_HPP
