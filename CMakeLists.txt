cmake_minimum_required(VERSION 3.10)
project(sik_nadajnik)

set(CMAKE_CXX_STANDARD 17)


add_executable(sk_transmitter main.cpp sk_transmitter/exceptions.hpp sk_transmitter/types.hpp sk_transmitter/internal_msg.hpp sk_transmitter/lockable_cache.hpp sk_transmitter/lockable_queue.hpp sk_transmitter/sk_socket.hpp)

target_link_libraries(sk_transmitter pthread)