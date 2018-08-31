#include "../catch.hpp"

#include "../../src/common/exceptions.hpp"
#include "../../src/common/types.hpp"
#include "../../src/common/data_msg.hpp"

#ifndef UDP_DATAGRAM_DATA_LEN_MAX
#define UDP_DATAGRAM_DATA_LEN_MAX 65535
#endif

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
        // simulate message receive:
        auto sndbl = test_msg.sendable();
        char rcv_buff[UDP_DATAGRAM_DATA_LEN_MAX];
        memcpy(rcv_buff, sndbl.data(), sndbl.size());
        sikradio::common::msg_t raw_msg;
        raw_msg.assign(rcv_buff, rcv_buff+sizeof(rcv_buff));
        // actual construction from received message:
        auto msg = sikradio::common::data_msg(raw_msg);
        sikradio::common::msg_t rcvd_data(msg.get_data().begin(), msg.get_data().begin() + data.size());

        REQUIRE(msg.get_id() == id);
        REQUIRE(msg.get_session_id() == session_id);
        REQUIRE(rcvd_data == data);
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
