#include "../catch.hpp"

#include "../../src/receiver/data_socket.hpp"

TEST_CASE("receiver data socket construction") {
    REQUIRE_NOTHROW(sikradio::receiver::data_socket());
    REQUIRE_NOTHROW(sikradio::receiver::data_socket(9999));
}

TEST_CASE("receiver data socket not connected") {
    SECTION("returns null") {
        sikradio::receiver::data_socket sock{};

        REQUIRE(sock.try_read() == std::nullopt);
    }
}