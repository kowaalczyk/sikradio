debug = true
ifeq ($(debug), true)
	COMPILER = g++-7
	PRE_FLAGS = -std=c++17 -Wall -Werror -g
	POST_FLAGS = -lpthread -lboost_program_options
	CATCH_TEST_FLAGS = -r compact
else
	COMPILER = g++
	PRE_FLAGS = -std=c++17 -Wall -Werror -DNDEBUG
	POST_FLAGS = -lpthread -lboost_program_options
endif

all: sikradio-sender sikradio-receiver

sikradio-sender: src/sender.cpp
	$(COMPILER) $(PRE_FLAGS) $< $(POST_FLAGS) -o $@

sikradio-receiver: src/receiver.cpp
	$(COMPILER) $(PRE_FLAGS) $< $(POST_FLAGS) -o $@

test/build/test_main.o: test/test_main.cpp
	-mkdir test/build > /dev/null 2>&1
	$(COMPILER) $(PRE_FLAGS) $< -c -o $@

test-common: test/build/test_main.o clean
	$(COMPILER) $(PRE_FLAGS) test/common.cpp $< -o $@
	- ./$@ $(CATCH_TEST_FLAGS)

test-sender: test/build/test_main.o clean
	$(COMPILER) $(PRE_FLAGS) test/sender.cpp $< -o $@
	- ./$@ $(CATCH_TEST_FLAGS)

test-receiver: test/build/test_main.o clean
	$(COMPILER) $(PRE_FLAGS) $< test/receiver.cpp -o $@
	- ./$@ $(CATCH_TEST_FLAGS)

.PHONY: clean

clean:
	rm -f sikradio-sender sikradio-receiver test-* *.o *.d *~ *.bak

clean-test-main:
	# this is not performed during clean, to speed up repeated test compilation
	rm -rf test/build

-include $(DEPS)
