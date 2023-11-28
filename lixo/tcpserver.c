#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define PORT 58001

int main()
{
    int fd, newfd, errcode;
    ssize_t n;
    socklen_t addrlen;
    struct addrinfo hints, *res;
    struct sockaddr_in addr;
    char buffer[128];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;       // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP socket
    hints.ai_flags = AI_PASSIVE;     // For binding to any available address

    errcode = getaddrinfo(NULL, "58001", &hints, &res);
    if (errcode != 0)
    {
        fprintf(stderr, "getaddrinfo failed: %s\n", gai_strerror(errcode));
        exit(1);
    }

    fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (fd == -1)
    {
        perror("socket");
        exit(1);
    }

    if (bind(fd, res->ai_addr, res->ai_addrlen) == -1)
    {
        perror("bind");
        close(fd);
        exit(1);
    }

    if (listen(fd, 5) == -1)
    {
        perror("listen");
        close(fd);
        exit(1);
    }

    while (1)
    {
        addrlen = sizeof(addr);
        newfd = accept(fd, (struct sockaddr *)&addr, &addrlen);
        if (newfd == -1)
        {
            perror("accept");
            close(fd);
            exit(1);
        }

        n = read(newfd, buffer, sizeof(buffer) - 1);
        if (n == -1)
        {
            perror("read");
            close(newfd);
            continue;
        }

        buffer[n] = '\0'; // Null-terminate the received data
        printf("Received: %s", buffer);

        n = write(newfd, buffer, n);
        if (n == -1)
        {
            perror("write");
            close(newfd);
        }

        close(newfd);
    }

    freeaddrinfo(res);
    close(fd);

    return 0;
}
