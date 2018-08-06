#include "sender/transmitter.hpp"
#include <boost/program_options.hpp>


namespace po = boost::program_options;


int main(int argc, char *argv[]) {
    po::options_description desc("Allowed options");
    desc.add_options()
            (",a", po::value<std::string>()->required(), "MCAST_ADDR")
            (",P", po::value<uint16_t>()->default_value(25830), "DATA_PORT")
            (",C", po::value<uint16_t>()->default_value(35830), "CTRL_PORT")
            (",p", po::value<size_t>()->default_value(512), "PSIZE")
            (",f", po::value<size_t>()->default_value(131072), "FSIZE")
            (",R", po::value<size_t>()->default_value(250), "RTIME")
            (",n", po::value<std::string>()->default_value("Nienazwany_nadajnik"), "NAZWA");

    po::variables_map vm;
    try {
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);
    } catch (po::error &e) {
        std::cerr << e.what() << std::endl;
        exit(1);
    }

    sender::transmitter transmitter(
            vm["-p"].as<size_t>(),
            vm["-f"].as<size_t>(),
            vm["-R"].as<size_t>(),
            vm["-a"].as<std::string>(),
            vm["-P"].as<uint16_t>(),
            vm["-C"].as<uint16_t>(),
            vm["-n"].as<std::string>()
    );

    transmitter.transmit();
    return 0;
}
