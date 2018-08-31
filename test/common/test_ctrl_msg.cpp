#include "../catch.hpp"

#include <set>
#include "../../src/common/types.hpp"
#include "../../src/common/ctrl_msg.hpp"

TEST_CASE("control message construction") {
    SECTION("lookup messsage") {
        auto msg = sikradio::common::make_lookup();

        REQUIRE(msg.is_lookup());
        REQUIRE(!msg.is_reply());
        REQUIRE(!msg.is_rexmit());
    }

    SECTION("reply message") {
        auto given_name = "Test radio";
        auto given_addr = "239.10.11.12";
        in_port_t given_port = 9999;
        auto msg = sikradio::common::make_reply(
            given_name,
            given_addr,
            given_port
        );

        SECTION("is of correct type") {
            REQUIRE(msg.is_reply());
            REQUIRE(!msg.is_lookup());
            REQUIRE(!msg.is_rexmit());
        }
        
        SECTION("returns correct attributes") {
            auto [name, addr, port] = msg.get_reply_data();

            REQUIRE(name == given_name);
            REQUIRE(addr == given_addr);
            REQUIRE(port == given_port);
        }
    }

    SECTION("rexmit message") {
        std::set<sikradio::common::msg_id_t> given_ids = {5,10,7,3,11};
        auto msg = sikradio::common::make_rexmit(given_ids);

        SECTION("is of correct type") {
            REQUIRE(msg.is_rexmit());
            REQUIRE(!msg.is_reply());
            REQUIRE(!msg.is_lookup());
        }

        SECTION("returns correct attributes") {
            auto ret_ids = msg.get_rexmit_ids();
            for (auto id : given_ids) {
                auto found = std::find(ret_ids.begin(), ret_ids.end(), id);
                REQUIRE(found != ret_ids.end());
            }
        }
    }
}