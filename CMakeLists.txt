cmake_minimum_required(VERSION 3.10)
project(sik_nadajnik)

set(CMAKE_CXX_STANDARD 17)

find_package(Threads REQUIRED)
find_package(Boost COMPONENTS program_options iostreams REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

add_executable(sk_transmitter main.cpp sk_transmitter/exceptions.hpp sk_transmitter/types.hpp sk_transmitter/data_msg.hpp sk_transmitter/lockable_cache.hpp sk_transmitter/lockable_queue.hpp sk_transmitter/data_socket.hpp lib/optional.hpp sk_transmitter/ctrl_socket.hpp sk_transmitter/ctrl_msg.hpp transmitter.hpp sk_transmitter/workers/sender.hpp)

#target_link_libraries(sk_transmitter boost_program_options)
target_link_libraries(sk_transmitter pthread ${Boost_LIBRARIES})
