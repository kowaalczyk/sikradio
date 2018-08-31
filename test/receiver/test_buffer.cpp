#include "../catch.hpp"

#include "../../src/receiver/buffer.hpp"
#include "../../src/common/data_msg.hpp"

const std::string msg_data = "some random message data";

sikradio::common::data_msg msg(sikradio::common::msg_id_t id) {
    return sikradio::common::data_msg(
        id, 
        42, 
        sikradio::common::msg_t(msg_data.begin(), msg_data.end())
    );
}

TEST_CASE("buffer construction") {
    REQUIRE_NOTHROW(sikradio::receiver::buffer(10));
}

TEST_CASE("buffer access") {
    sikradio::receiver::buffer buf{10};

    SECTION("empty read returns null") {
        REQUIRE(buf.try_read() == std::nullopt);
    }

    SECTION("after 1 write returns null") {
        auto m = msg(1);
        buf.write(m);

        REQUIRE(buf.try_read() == std::nullopt);
    }

    SECTION("after 3/4*size - 1 writes returns null") {
        for (sikradio::common::msg_id_t id = 0; id < 7; id++) {
            auto m = msg(id);
            buf.write(m);
        }

        REQUIRE(buf.try_read() == std::nullopt);
    }

    SECTION("after 3/4*size writes") {
        size_t buf_size = 10;
        sikradio::receiver::buffer buf{buf_size};
        for (sikradio::common::msg_id_t id = 0; id < buf_size*3/4 + 1; id++) {
            auto m = msg(id);
            buf.write(m);
        }

        SECTION("read possible") {
            REQUIRE(buf.try_read().has_value());   
        }

        SECTION("read possible 3/4*size times, then exception is thrown") {
            for (sikradio::common::msg_id_t id = 0; id < buf_size*3/4 + 1; id++) {
                auto m = buf.try_read();

                REQUIRE(m.has_value());
            }
            REQUIRE_THROWS_AS(buf.try_read(), sikradio::receiver::exceptions::buffer_access_exception);
        }

        SECTION("and reset, read returns null") {
            buf.reset();

            REQUIRE(buf.try_read() == std::nullopt);
        }
    }
}

TEST_CASE("buffer space management") {
    size_t buf_size = 10;
    sikradio::receiver::buffer buf{buf_size};
        
    SECTION("empty has space for no message") {
        REQUIRE_FALSE(buf.has_space_for(0));
        REQUIRE_FALSE(buf.has_space_for(10));
    }

    SECTION("full") {
        auto m = msg(1);
        buf.write(m);
        m = msg(buf_size);
        buf.write(m);

        SECTION("has space for all missed messages") {
            for (sikradio::common::msg_id_t id = 2; id < buf_size; id++) {
                REQUIRE(buf.has_space_for(id));
            }
        }

        SECTION("has space for inserted messages") {
            REQUIRE(buf.has_space_for(1));
            REQUIRE(buf.has_space_for(buf_size));
        }

        SECTION("has no space for older message") {
            REQUIRE_FALSE(buf.has_space_for(0));
        }

        SECTION("has no space for future message") {
            REQUIRE_FALSE(buf.has_space_for(buf_size+1));
        }

        SECTION("after reset has space for no message") {
            buf.reset();
            for (sikradio::common::msg_id_t id = 0; id < buf_size+1; id++) {
                REQUIRE_FALSE(buf.has_space_for(id));
            }
        }
    }
}