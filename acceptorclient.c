#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <errno.h>

int acceptor_connect(const char* path)
{
    int sockfd = -1;
    struct sockaddr_un remote = {};
    int len = 0;

    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if(sockfd < 0)
        return -1;

    remote.sun_family = AF_UNIX;
    if(strlen(path) >= sizeof(remote.sun_path)) {
        close(sockfd);
        errno = ENAMETOOLONG;
        return -2;
    }
    strncpy(remote.sun_path, path, sizeof(remote.sun_path));
    len = strlen(remote.sun_path) + sizeof(remote.sun_family);

    if (connect(sockfd, (struct sockaddr*)&remote, len) == -1) {
        close(sockfd);
        return -3;
    }

    return sockfd;
}

int acceptor_accept(int acceptor)
{
    //references:
    //http://www.normalesup.org/~george/comp/libancillary/
    //http://lists.canonical.org/pipermail/kragen-hacks/2002-January/000292.htmls
    struct msghdr msg = {};
    struct iovec iov = {};
    char dummy;
    int sockfd = -1;
    int ccmsg[CMSG_SPACE(sizeof(sockfd)) / sizeof(int)];
    struct cmsghdr* cmsg = NULL;
    int ret = -1;

    iov.iov_base = &dummy;
    iov.iov_len = 1;

    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = ccmsg;
    msg.msg_controllen = sizeof(ccmsg);

    if(recvmsg(acceptor, &msg, 0) <= 0) {
        return -1;
    }

    cmsg = CMSG_FIRSTHDR(&msg);
    if (!cmsg->cmsg_type == SCM_RIGHTS) {
        errno = EBADMSG;
        return -2;
    }

    ret = *(int*)CMSG_DATA(cmsg);
    if(ret < 0) {
        errno = EBADF;
        return -3;
    }
    return ret;
}

