#include "../catch.hpp"

#include "../../src/common/ctrl_socket.hpp"

TEST_CASE("control socket construction") {
    REQUIRE_NOTHROW(sikradio::common::ctrl_socket(9999));
}
