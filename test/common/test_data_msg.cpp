#include "../catch.hpp"

#include "../../src/common/exceptions.hpp"
#include "../../src/common/data_msg.hpp"
#include "../../src/common/types.hpp"

TEST_CASE("data message construction") {
    sikradio::common::msg_id_t id = 8;
    sikradio::common::msg_id_t session_id = 7;
    sikradio::common::msg_t data = {11,12,13,14,15,16};

    SECTION("constructor #1") {
        auto msg = sikradio::common::data_msg(id);
        
        REQUIRE(msg.get_id() == id);
        REQUIRE_THROWS_AS(msg.sendable(), sikradio::common::exceptions::data_msg_exception);
    }
    SECTION("constructor #2") {
        auto msg = sikradio::common::data_msg(id, data);
    
        REQUIRE(msg.get_id() == id);
        REQUIRE(msg.get_data() == data);
        REQUIRE_THROWS_AS(msg.sendable(), sikradio::common::exceptions::data_msg_exception);
    }
    SECTION("constructor #3") {
        auto msg = sikradio::common::data_msg(id, session_id, data);
    
        REQUIRE(msg.get_id() == id);
        REQUIRE(msg.get_session_id() == session_id);
        REQUIRE(msg.get_data() == data);
        REQUIRE_NOTHROW(msg.sendable());
    }
    SECTION("from raw receiverd message") {
        auto test_msg = sikradio::common::data_msg(id, session_id, data);
        auto rcvd = test_msg.sendable().data();
        auto msg = sikradio::common::data_msg{rcvd};

        REQUIRE(msg.get_id() == id);
        REQUIRE(msg.get_session_id() == session_id);
        REQUIRE(msg.get_data() == data);
        REQUIRE_NOTHROW(msg.sendable());
    }
}

TEST_CASE("comparison of messages") {
    sikradio::common::msg_id_t smaller_id = 8;
    sikradio::common::msg_id_t bigger_id = 10;

    SECTION("comparison") {
        auto msg1 = sikradio::common::data_msg{smaller_id};
        auto msg2 = sikradio::common::data_msg{bigger_id};

        REQUIRE(msg1 < msg2);
    }
    // TODO: If equality is added, make sure to test this:
    // SECTION("equality") {
    //     auto msg1 = sikradio::common::data_msg{smaller_id};
    //     auto msg2 = sikradio::common::data_msg{smaller_id};

    //     REQUIRE(msg1 == msg2);
    // }
}
