sikradio-sender: main.cpp
        g++ -std=c++17 -Wall -Werror -lpthread -lboost_program_options $< -o $@

.PHONY: clean
clean:
      	rm -f sikradio-sender *.o *.d *~ *.bak

-include $(DEPS)
