TARGET ?= sikradio-sender

sikradio-sender: main.o

main.o: main.cpp
	g++ -std=c++17 -lpthread -L/usr/lib/x86_64-linux-gnu/ -static -lboost_program_options $< -o $@

.PHONY: clean
clean:
	rm -f $(TARGETS) *.o *.d *~ *.bak

-include $(DEPS)
