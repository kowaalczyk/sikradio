#ifndef SIKRADIO_RECEIVER_EXCEPTIONS_HPP
#define SIKRADIO_RECEIVER_EXCEPTIONS_HPP


#include <string>
#include <exception>


namespace sikradio::receiver::exceptions {
    class buffer_access_exception : public std::exception {
    private:
        const std::string msg_str;

    public:
        explicit buffer_access_exception(std::string msg_str) : msg_str(std::move(msg_str)) {}

        const char *what() const noexcept override {
            return msg_str.c_str();
        }
    };
}


#endif