#include <boost/program_options.hpp>
#include "acceptorclient.h"
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <iostream>
#include <stdio.h>

int main(int argc, char* argv[])
{
    namespace po = boost::program_options;
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "produce help message")
        ("name,n", po::value<std::string>(), "name of the UNIX socket to connect to")
        ("message,m", po::value<std::string>()->default_value("hello"), "message to send to clients")
    ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << std::endl;
        return 1;
    }

    while(1) {
        int acceptor = acceptor_connect(vm["name"].as<std::string>().c_str());
        if(acceptor < 0) {
            perror(NULL);
            sleep(1);
            continue;
        }
        std::cout << "connected" << std::endl;

        std::string message = vm["message"].as<std::string>();

        int clientSock = -1;
        while((clientSock = acceptor_accept(acceptor)) >= 0) {
            std::cout << "got socket: " << clientSock << std::endl;
            write(clientSock, message.c_str(), message.length());
            close(clientSock);
        }
    }

    perror(NULL);

    return 0;
}
