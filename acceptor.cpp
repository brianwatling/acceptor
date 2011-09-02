#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <iostream>
#include <string>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/un.h>
#include <poll.h>

#define QUICK_ASSERT(check) \
    do { \
        if(!(check)) { \
            throw std::runtime_error(boost::str(boost::format("fatal error at %1%:%2% message: %3%") % __FILE__ % __LINE__ % #check)); \
        } \
    } while(0)

#define ERRNO_ASSERT(check) \
    do { \
        if(!(check)) { \
            char errorDesc[256]; \
            strerror_r(errno, errorDesc, sizeof(errorDesc)); \
            throw std::runtime_error(boost::str(boost::format("fatal error at %1%:%2% errno: %3% message: %4%") % __FILE__ % __LINE__ % errno % (const char*)errorDesc)); \
        } \
    } while(0)

int tcpSocketListen(const char* host, const char* port, int backlog)
{
    struct addrinfo hints;
    struct addrinfo *res;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;// use IPv4 or IPv6, whichever
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;// fill in my IP for me
    getaddrinfo(host, port, &hints, &res);

    int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    ERRNO_ASSERT(sockfd >= 0);
    int one = 1;
    ERRNO_ASSERT(!setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (void*)&one, sizeof(one)));
    ERRNO_ASSERT(!bind(sockfd, res->ai_addr, res->ai_addrlen));
    ERRNO_ASSERT(!listen(sockfd, backlog));
    return sockfd;
}

int unixSocketListen(const char* path, int backlog)
{
    //references:
    //http://beej.us/guide/bgipc/output/html/multipage/unixsock.html
    int sockfd;
    struct sockaddr_un local;
    int len;

    local.sun_family = AF_UNIX;
    QUICK_ASSERT(strlen(path) < sizeof(local.sun_path));
    strncpy(local.sun_path, path, sizeof(local.sun_path));
    len = strlen(local.sun_path) + sizeof(local.sun_family);
    unlink(local.sun_path);

    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    ERRNO_ASSERT(sockfd >= 0);
    ERRNO_ASSERT(!bind(sockfd, (struct sockaddr*)&local, len));
    ERRNO_ASSERT(!listen(sockfd, backlog));

    return sockfd;
}

void sendFd(int unixSocket, int fd)
{
    //references:
    //http://www.normalesup.org/~george/comp/libancillary/
    //http://lists.canonical.org/pipermail/kragen-hacks/2002-January/000292.htmls
    char dummy = '@';
    struct iovec iov;
    iov.iov_base = &dummy;
    iov.iov_len = 1;

    char ccmsg[CMSG_SPACE(sizeof(fd))];
    memset(ccmsg, 0, sizeof(ccmsg));

    struct msghdr msg;
    memset(&msg, 0, sizeof(msg));
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = ccmsg;
    msg.msg_controllen = sizeof(ccmsg);

    struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(fd));
    *(int*)CMSG_DATA(cmsg) = fd;
    ERRNO_ASSERT(sendmsg(unixSocket, &msg, 0) == 1);
}

int main(int argc, char* argv[])
{
    int tcpSocket = -1;
    int unixSocket = -1;
    try {
        namespace po = boost::program_options;
        po::options_description desc("Allowed options");
        desc.add_options()
            ("help,h", "produce help message")
            ("name,n", po::value<std::string>(), "name of the UNIX socket to bind")
            ("host,h", po::value<std::string>()->default_value("localhost"), "host to listen for tcp connections on")
            ("port,p", po::value<std::string>(), "port to listen for tcp connections on")
            ("listenBacklog,l", po::value<int>()->default_value(5), "backlog parameter passed to listen()")
        ;

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        if (vm.count("help")) {
            std::cout << desc << std::endl;
            return 1;
        }

        tcpSocket = tcpSocketListen(vm["host"].as<std::string>().c_str(), vm["port"].as<std::string>().c_str(), vm["listenBacklog"].as<int>());

        unixSocket = unixSocketListen(vm["name"].as<std::string>().c_str(), vm["listenBacklog"].as<int>());

        std::vector<struct pollfd> pfds(2);
        pfds[0].fd = tcpSocket;
        pfds[0].events = POLLIN | POLLERR;
        pfds[1].fd = unixSocket;
        pfds[1].events = POLLIN | POLLERR;

        size_t nextIndex = 0;

        while(poll(&pfds[0], pfds.size(), -1) > 0) {std::cout << "polled" << std::endl;
            if(pfds[0].revents) {
                const int clientSock = accept(tcpSocket, NULL, NULL);
                std::cout << "got socket: " << clientSock << std::endl;
                ERRNO_ASSERT(clientSock >= 0);
                if(pfds.size() > 2) {
                    const size_t index = nextIndex % (pfds.size() - 2) + 2;
                    sendFd(pfds[index].fd, clientSock);
                    ++nextIndex;
                    std::cout << "sent socket to acceptor with fd " << pfds[index].fd << std::endl;
                } else {
                    std::cout << "got client connection but no acceptors are connected " << pfds.size() << std::endl;
                }
                close(clientSock);
            }
            if(pfds[1].revents) {
                const int acceptorSocket = accept(unixSocket, NULL, NULL);
                ERRNO_ASSERT(acceptorSocket >= 0);
                struct pollfd pfd = {};
                pfd.fd = acceptorSocket;
                pfd.events = POLLIN | POLLERR;
                pfds.push_back(pfd);
                std::cout << "new acceptor connected with fd " << acceptorSocket << std::endl;
            }
            std::vector<struct pollfd>::iterator cur = pfds.begin() + 2;
            while(cur != pfds.end()) {
                if(cur->revents) {
                    close(cur->fd);
                    std::cout << "acceptor with fd " << cur->fd << " disconnected" << std::endl;
                    cur = pfds.erase(cur);
                } else {
                    ++cur;
                }
            }std::cout << "polling" << std::endl;
        }

    } catch(std::exception& e) {
        std::cerr << "exception: " << e.what() << std::endl;
    }

    if(tcpSocket >= 0) {
        close(tcpSocket);
    }
    if(unixSocket >= 0) {
        close(unixSocket);
    }
    return 0;
}

