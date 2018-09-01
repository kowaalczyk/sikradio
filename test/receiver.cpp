#include "catch.hpp"

#include <set>
#include <thread>

#include "../src/receiver/buffer.hpp"
#include "../src/common/data_msg.hpp"
#include "../src/receiver/data_socket.hpp"
#include "../src/receiver/rexmit_manager.hpp"

namespace {
    const std::string msg_data = "some random message data";

    sikradio::common::data_msg msg(sikradio::common::msg_id_t id) {
        return sikradio::common::data_msg(
            id, 
            42, 
            sikradio::common::msg_t(msg_data.begin(), msg_data.end())
        );
    }
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

TEST_CASE("data socket construction") {
    REQUIRE_NOTHROW(sikradio::receiver::data_socket());
    REQUIRE_NOTHROW(sikradio::receiver::data_socket(9999));
}

TEST_CASE("data socket not connected") {
    SECTION("returns null") {
        sikradio::receiver::data_socket sock{};

        REQUIRE(sock.try_read() == std::nullopt);
    }
}

TEST_CASE("rexmit manager construction") {
    REQUIRE_NOTHROW(sikradio::receiver::rexmit_manager(10));
}

TEST_CASE("rexmit manager access") {
    size_t rtime = 1;  // in seconds
    sikradio::receiver::rexmit_manager mng{rtime};
    std::set<sikradio::common::msg_id_t> empty_set;
    
    SECTION("returns empty set when no ids were inserted") {
        REQUIRE(mng.filter_get_ids(empty_set).empty());
    }

    SECTION("before rtime returns empty set after id is inserted") {
        mng.append_ids(2, 3);

        auto rexmit_set = mng.filter_get_ids(empty_set);
        REQUIRE(rexmit_set.empty());
    }

    SECTION("after rtime returns set with inserted id") {
        mng.append_ids(2, 3);
        std::this_thread::sleep_for(std::chrono::seconds(rtime));
        
        auto rexmit_set = mng.filter_get_ids(empty_set);
        REQUIRE(rexmit_set.find(2) == rexmit_set.begin());
    }

    SECTION("after rtime and reset returns empty set") {
        mng.append_ids(2,3);
        std::set<sikradio::common::msg_id_t> forget_inserted_ids = {2};
        std::this_thread::sleep_for(std::chrono::seconds(rtime));
        mng.reset();

        auto rexmit_set = mng.filter_get_ids(forget_inserted_ids);
        REQUIRE(rexmit_set.empty());
    }

    SECTION("after rtime forgotten inserted id is not returned") {
        mng.append_ids(2,3);
        std::set<sikradio::common::msg_id_t> forget_inserted_ids = {2};

        SECTION("when forgotten immediately") {
            (void)mng.filter_get_ids(forget_inserted_ids);
            std::this_thread::sleep_for(std::chrono::seconds(rtime));

            auto rexmit_set = mng.filter_get_ids(empty_set);
            REQUIRE(rexmit_set.empty());
        }

        SECTION("when forgotten after rtime") {
            std::this_thread::sleep_for(std::chrono::seconds(rtime));

            auto rexmit_set = mng.filter_get_ids(forget_inserted_ids);
            REQUIRE(rexmit_set.empty());
        }
    }
}
