# sikradio  
A final project for computer networking course, MIM UW, spring semester 2017-19.  

The aim of this project was to create a high performance internet radio for music streamin, consisting of receiver and sender. As a requirement, I had to stick to a network protocol described below and work on UNIX socket interface directly, without 3rd party libraries. In order for the project to run smoothly and meet performance requirements based on which it was graded, it is highly concurrent.  

## Sender  
Sender reads data from standard input and broadcasts it to a multicast address provided with `-a` command line argument. Read data is stored in a queue, which allows for retransmission of recent messages after a client request to do so. Sender starts broadcasting messages only after queue becomes full enough for continuous streaming.  
Other possible arguments (all optional) are:  
* `-P` - port for data communication  
* `-C` - port for control communication  
* `-p` - size of audio data in data packet in bytes  
* `-f` - size of queue for data messages in bytes  
* `-R` - time between retransmissions in milliseconds  
* `-n` - name of radio station streamed by the sender  

## Receiver  
Receiver subscribes to one of senders' multicast addresses and writes received data to standard output. It uses control communication to get the list of active senders and their adresses, and accepts connections on ui port that can change audio source to other sender. Received audio data is stored in a buffer, and streamed only when buffer is full enough to result in fluent transmission. This allows to collect responses to retransmission requests that are sent in case when a message is missed.  
Optional arguments for the receivers include:  
* `-d` - discovery address for sending lookup control requests  
* `-C` - port for other control communication  
* `-U` - port for ui communication  
* `-b` - size of buffer  
* `-R` - time between retransmission requests for messages  
* `-n` - name of the station, if specified sender will switch to it as soon as it is detected  

## Protocols  
All communication is conducted via IPv4.  

### Control  
Data streaming is conducted through UDP datagrams containing text data. Each message is ended with an UNIX endline character, all other characters can only contain ASCII values from 32 to 127. Data fields in messages is separated with spaces, last field can contain values with spaces (for example, name of the station).  

Receiver broadcasts **lookup messages** to identify active stations. Senders responds to lookup requests with **reply messages**, to the same address from which lookup request was sent. Receivers sends **rexmit messages** if data packets they received were out of order and therefore missed.  

**lookup message schema**  
`ZERO_SEVEN_COME_IN`  

**reply message schema**  
`BOREWICZ_HERE [sender multicast ip address] [sedner data port] [station name]`  

**rexmit message schema**  
`LOUDER_PLEASE [list of first byte numbers of packets that were not received, comma-separated]`  

### Data streaming protocol  
Data streaming is conducted through UDP datagrams containing binary data.  
Each datagram consists of:  
* `uint64_t session_id`, big-endian byte order   
* `uint64_t first_byte_num`, big-endian byte order  
* `byte[] audio_data`  
Session id remains constant throughout execution of a single sender. Receiver should remember last received session id and ignore messages with lower session ids. Whenever session ids is changed to higher id, receiver re-starts audio playback.  
Sender indexes the bytes within each session, receiver ignores bytes that are too old to be inserted into its buffer. If receiver does not receive consecutive messages, it asks for retransmission as specified in the control protocol.  

### UI  
For communication with UI clients, receiver uses [Telnet protocol](https://tools.ietf.org/html/rfc854). After connecting to receivers' ui port, telnet client receives list of station (with updates) and can change station by pressing arrow keys.  

## Architecture  
Both sender and receiver are split into multiple threads in order to maximize streaming speed - it was necessary to minimize the number of locking on each of the threads. Each thread performs maximum of 1 blocking i/o operation and others are timed out - thanks to that, there is no risk of deadlocking the entire system and resources shared between the threads are accessed only when it is absolutely necessary.  
Both programs share many things in common, in particular the control protocol specification and data types - these things are stored in `src/common` folder and included in both the receiver and the sender.  

## Requirements  
Project requires support of C++17 features (at least these supported in g++ version 7.3.0).  
The only third party library that needs to be installed is boost_program_options, from boost version 1_65_1. To check boost version, use `$ cat /usr/include/boost/version.hpp | grep "#define BOOST_LIB_VERSION"`.  
Project was never tested on platforms other than Linux, but should be compatible within entire UNIX family of systems.  
I can go on explaining the architecture, but I believe looking at the code is a much better way to explore the project. I will add that the all-header design is due to me not wanting to debug linkng order while we were prohibitted from using CMake.  

## Building and running  
Project can be built using provided Makefile.  
Default target `$ make` creates executables for sender and receiver: `sikradio-sender` and `sikradio-receiver`. These files can be cleaned using `$ make clean` if necessary.  

### Streaming a wav file  
**sender**  
`$ sox -S "some-music.mp3" -r 44100 -b 16 -e signed-integer -c 2 -t raw - | pv -q -L $((44100*4)) | ./sikradio-sender -a 239.10.11.12 -n "My awesome radio"`  

**receiver**  
`./sikradio-receiver | play -t raw -c 2 -r 44100 -b 16 -e signed-integer --buffer 32768 -`  

### Tests  
There are also targets for tests: `$ make test-receiver`, `$ make test-sender`, `$ make test-common` build and execute unit tests for various component of the project. To speed up testing build time, `$ make clean` will not clean `catch_test_main`, which needs to be built only once.  

## Third party libraries  
Boost library is not included in the project and can be downloaded from [its own webpage](https://www.boost.org/).  
For testing, [Catch 2 single-header test framework]() is included in `test/` directory. It is shared under BSL 1.0 license included in LICENSE file.  
