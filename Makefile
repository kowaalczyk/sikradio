sikradio-sender: main.cpp
	g++-7 -std=c++17 -Wall -Werror $< -lpthread -lboost_program_options -o $@
	# uncomment for release:
	# g++-7 -std=c++17 -Wall -Werror $< -lpthread -lboost_program_options -o $@

.PHONY: clean
clean:
	rm -f sikradio-sender *.o *.d *~ *.bak

-include $(DEPS)
