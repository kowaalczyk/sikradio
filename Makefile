sikradio-sender: sender.cpp
	g++-7 -std=c++17 -Wall -Werror $< -lpthread -lboost_program_options -o $@
	# uncomment for release:
	# g++ -std=c++17 -Wall -Werror $< -lpthread -lboost_program_options -o $@

sikradio-receiver: receiver.cpp
	g++-7 -std=c++17 -Wall -Werror $< -lpthread -lboost_program_options -o $@
	# uncomment for release:
	# g++ -std=c++17 -Wall -Werror $< -lpthread -lboost_program_options -o $@

all: sikradio-sender sikradio-receiver

.PHONY: clean

clean:
	rm -f sikradio-sender *.o *.d *~ *.bak

-include $(DEPS)
