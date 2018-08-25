#include <boost/program_options.hpp>
#include <iostream>


namespace po = boost::program_options;


int main(int argc, char *argv[]) {
    po::options_description desc("Allowed options");
    desc.add_options()
            (",d", po::value<std::string>()->default_value("255.255.255.255"), "DISCOVER_ADDR")
            (",C", po::value<uint16_t>()->default_value(35830), "CTRL_PORT")
            (",U", po::value<uint16_t>()->default_value(15830), "UI_PORT")
            (",b", po::value<size_t>()->default_value(65536), "BSIZE")
            (",R", po::value<size_t>()->default_value(250), "RTIME")
            (",n", po::value<std::string>()->default_value(""), "PREFERRED_STATION");
    
    po::variables_map vm;
    try {
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);
    } catch (po::error &e) {
        std::cerr << e.what() << std::endl;
        exit(1);
    }
    // TODO: Reciever 
}