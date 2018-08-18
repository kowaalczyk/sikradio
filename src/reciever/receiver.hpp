#ifndef SIKRADIO_RECEIVER_RECEIVER_HPP
#define SIKRADIO_RECEIVER_RECEIVER_HPP


namespace sikradio::receiver {
    class receiver {
    private:
        void run_ctrl_reciever(station_list*, ctrl_socket*) {
            // TODO:
            // 0. try recieve msg from ctrl_socket, if recieved:
            // 1. make sure message was "reply"
            // 2. update value in menu with current timestamp and data (sender address, mcast address, data port)
        }

        void run_lookup_sender(ctrl_socket*) {
            // TODO:
            // sleep 5 secs
            // send lookup to DISCOVER_ADDR via ctrl_socket
        }

        void run_data_reciever(data_socket*, buffer*, station_list*) {
            // TODO:
            // 1. recieve data_msg from socket
            // if msg recieved:
            //   2. remember last recieved id and session
            //   3. write message to buffer
            //   4. if some id were missed, spawn rexmit senders ctrl_socket
        }

        void run_rexmit_sender(buffer*, station_list*, station, list_of_msg_ids) {
            // TODO:
            // while msg list is not empty:
            //   1. Check if station is still in menu and get its ctrl_addr from there
            //   2. Remove messages that cannot be saved in buffer (too old, etc.)
            //   3. If there are still some msgs:
                // 4. Send rexmit request to ctrl_addr (sendto?)
            //     5. Sleep RTIME
        }

        void run_data_streamer(buffer*) {
            // TODO:
            // 1. read from buffer
            // 2. if read successful, write it to stdout
            // NOTE: this can probably be executed in the main thread
        }
    public:
        receiver() = delete;

    };
}


#endif
