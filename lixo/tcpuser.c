#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <fcntl.h>
#define PORT 58001

int main()
{
    int fd, errcode;
    ssize_t n;
    struct addrinfo hints, *res;
    char buffer[128];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;       // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP socket

    errcode = getaddrinfo("tejo.tecnico.ulisboa.pt", "58001", &hints, &res);
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

    if (connect(fd, res->ai_addr, res->ai_addrlen) == -1)
    {
        perror("connect");
        close(fd);
        exit(1);
    }

    n = write(fd, "LIN 123123 password323\n", 7);
    if (n == -1)
    {
        perror("write");
        close(fd);
        exit(1);
    }

    n = read(fd, buffer, sizeof(buffer) - 1);
    if (n == -1)
    {
        perror("read");
        close(fd);
        exit(1);
    }

    buffer[n] = '\0'; // Null-terminate the received data
    printf("Received: %s", buffer);

    freeaddrinfo(res);
    close(fd);

    return 0;
}
