# development:
COMPILER = g++-7
PRE_FLAGS = -std=c++17 -Wall -Werror
POST_FLAGS = -lpthread -lboost_program_options
COMMON_TESTS = test/common/test_ctrl_msg.cpp test/common/test_data_msg.cpp
SENDER_TESTS = test/sender/test_lockable_cache.cpp test/sender/test_lockable_queue.cpp
RECEIVER_TESTS = 
CATCH_TEST_FLAGS = -r compact
# production:
# COMPILER = g++
# PRE_FLAGS = -std=c++17 -Wall -Werror
# POST_FLAGS = -lpthread -lboost_program_options

sikradio-sender: src/sender.cpp
	$(COMPILER) $(PRE_FLAGS) $< $(POST_FLAGS) -o $@

sikradio-receiver: src/receiver.cpp
	$(COMPILER) $(PRE_FLAGS) $< $(POST_FLAGS) -o $@

all: sikradio-sender sikradio-receiver

test/build/test_main.o: test/test_main.cpp
	mkdir test/build > /dev/null 2>&1
	$(COMPILER) $(PRE_FLAGS) $< -c -o $@

test-common: test/build/test_main.o
	$(COMPILER) $(PRE_FLAGS) $(COMMON_TESTS) $< -o $@
	./$@ $(CATCH_TEST_FLAGS)
	rm -f $@

test-sender: test/build/test_main.o
	$(COMPILER) $(PRE_FLAGS) $(SENDER_TESTS) $< -o $@
	./$@ $(CATCH_TEST_FLAGS)
	rm -f $@

test-receiver: test/build/test_main.o
	$(COMPILER) $(PRE_FLAGS) $(RECEIVER_TESTS) $< -o $@
	./$@ $(CATCH_TEST_FLAGS)
	rm -f $@

test-all: test/build/test_main.o
	$(COMPILER) $(PRE_FLAGS) $(COMMON_TESTS) $(SENDER_TESTS) $(RECEIVER_TESTS) $< -o $@
	./$@ $(CATCH_TEST_FLAGS)
	rm -f $@

.PHONY: clean

clean:
	rm -f sikradio-sender sikradio-receiver test-* *.o *.d *~ *.bak

clean-test-main:
	# this is not performed during clean, to speed up repeated test compilation
	rm -rf test/build

-include $(DEPS)
