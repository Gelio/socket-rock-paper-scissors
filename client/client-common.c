#include "client-common.h"

void setHandler(int signal, void (*handler)(int))
{
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = handler;

    if (sigaction(signal, &action, NULL) < 0)
        ERR("sigaction");
}

int makeSocket(int domain, int type)
{
    int socketDes = socket(domain, type, 0);
    if (socketDes < 0)
        ERR("socket");
    return socketDes;
}

struct sockaddr_in makeAddress(char *serverHostname, char *port)
{
    int ret;
    struct sockaddr_in addr;
    struct addrinfo *result;
    struct addrinfo hints = {};
    hints.ai_family = AF_INET;
    if ((ret = getaddrinfo(serverHostname, port, &hints, &result)))
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
        exit(EXIT_FAILURE);
    }
    addr = *(struct sockaddr_in *)(result->ai_addr);
    freeaddrinfo(result);
    return addr;
}

int connectSocket(char *name, char *port)
{
    struct sockaddr_in addr;
    int socketDes;
    socketDes = makeSocket(AF_INET, SOCK_STREAM);
    addr = makeAddress(name, port);
    if (connect(socketDes, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0)
    {
        if (errno != EINTR)
            ERR("connect");
        else
        {
            fd_set writeFds;
            int status;
            socklen_t size = sizeof(int);
            FD_ZERO(&writeFds);
            FD_SET(socketDes, &writeFds);
            if (TEMP_FAILURE_RETRY(select(socketDes + 1, NULL, &writeFds, NULL, NULL)) < 0)
                ERR("select");
            if (getsockopt(socketDes, SOL_SOCKET, SO_ERROR, &status, &size) < 0)
                ERR("getsockopt");
            if (status != 0)
                ERR("connect");
        }
    }
    return socketDes;
}

ssize_t bulkRead(int fd, char *buf, size_t length)
{
    size_t totalBytes = 0;
    while (length > 0)
    {
        ssize_t bytesRead = TEMP_FAILURE_RETRY(read(fd, buf, length));
        if (bytesRead < 0)
            return bytesRead;
        if (bytesRead == 0)
            return totalBytes;

        buf += bytesRead;
        totalBytes += bytesRead;
        length -= bytesRead;
    }

    return totalBytes;
}

ssize_t bulkWrite(int fd, char *buf, size_t length)
{
    size_t totalBytes = 0;
    while (length > 0)
    {
        ssize_t bytesWritten = TEMP_FAILURE_RETRY(write(fd, buf, length));
        if (bytesWritten < 0)
            return bytesWritten;
        if (bytesWritten == 0)
            return bytesWritten;

        buf += bytesWritten;
        totalBytes += bytesWritten;
        length -= bytesWritten;
    }

    return totalBytes;
}
