#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <fcntl.h>

#define AS_IP "tejo.tecnico.ulisboa.pt"
#define AS_PORT "58011"

#define AS_RESPONSETOKEN_LOGIN "RLU"
#define AS_RESPONSETOKEN_ACCEPTED "OK"
#define AS_RESPONSETOKEN_LOGIN_FAIL "NOK"
#define AS_RESPONSETOKEN_LOGIN_REG "REG"

#define MAX_BUFFER_SIZE 1024
#define MAX_FILENAME_LENGTH 24
#define MAX_FILE_SIZE 10000000 // 10 MB

struct User
{
    char uid[7];       // Assuming UID is a 6-digit string (plus null terminator)
    char password[20]; // Adjust the size according to your requirements
    bool loggedIn;     // Flag to indicate whether the user is logged in or not
};
// My User
struct User myUser;

// Function to send a message to the Auction Server using UDP
void sendUDPMessage(const char *message, char *response)
{
    int fd, errcode;
    ssize_t n;
    socklen_t addrlen;
    struct addrinfo hints, *res;
    struct sockaddr_in addr;
    char buffer[MAX_BUFFER_SIZE];

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1)
    {
        perror("socket");
        exit(1);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;      // IPv4
    hints.ai_socktype = SOCK_DGRAM; // UDP socket

    errcode = getaddrinfo(AS_IP, AS_PORT, &hints, &res);
    if (errcode != 0) /*error*/
        exit(1);

    n = sendto(fd, message, strlen(message), 0, res->ai_addr, res->ai_addrlen);
    if (n == -1) /*error*/
        exit(1);

    addrlen = sizeof(addr);
    n = recvfrom(fd, buffer, 128, 0,
                 (struct sockaddr *)&addr, &addrlen);
    if (n == -1) /*error*/
        exit(1);
    write(1, buffer, n);
    strcpy(response, buffer);
    freeaddrinfo(res);
    close(fd);
}

// Function to send a message to the Auction Server using TCP
void sendTCPMessage(const char *message)
{
    int fd, errcode;
    ssize_t n;
    struct addrinfo hints, *res;
    struct sockaddr_in addr;
    char buffer[MAX_BUFFER_SIZE];

    fd = socket(AF_INET, SOCK_STREAM, 0); // TCP socket
    if (fd == -1)
        exit(1); // error

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;       // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP socket

    // Replace "127.0.0.1" with the actual server IP address
    errcode = getaddrinfo(AS_IP, AS_PORT, &hints, &res);
    if (errcode != 0)
        exit(1); // error

    n = connect(fd, res->ai_addr, res->ai_addrlen);
    if (n == -1)
        exit(1); // error

    // Send the message in chunks
    size_t bytesRead;

    while ((bytesRead = read(message, buffer, sizeof(buffer))) > 0)
    {
        n = write(fd, buffer, bytesRead);
        if (n == -1)
        {
            perror("Error writing to socket");
            exit(EXIT_FAILURE);
        }
    }

    // Read the server response
    n = read(fd, buffer, sizeof(buffer));
    if (n == -1)
    {
        perror("Error reading from socket");
        exit(EXIT_FAILURE);
    }

    write(1, "echo: ", 6);
    write(1, buffer, n);

    freeaddrinfo(res);
    close(fd);
}

// Function to handle login request
void handleLogin()
{
    char request[MAX_BUFFER_SIZE];
    char response[MAX_BUFFER_SIZE];
    char token[4], status[3];
    // check inputs
    if (strlen(myUser.uid) != 6 || strlen(myUser.password) < 8)
    {
        write(1, "invalid login input\n", 20);
    }
    else
    {
        sprintf(request, "LIN %s %s\n", myUser.uid, myUser.password);

        // Send the request to the Auction Server using UDP
        sendUDPMessage(request, response);

        // handle response
        sscanf(response, "%s %s", token, status);
        if (!strcmp(status, AS_RESPONSETOKEN_ACCEPTED) || !strcmp(status, AS_RESPONSETOKEN_LOGIN_REG))
        {
            write(1, "success\n", 9);
            myUser.loggedIn = true;
        }
        else
        {
            printf("%s %s", token, status);
        }
    }
}

// Function to handle logout request
void handleLogout()
{
    char request[MAX_BUFFER_SIZE];
    char response[MAX_BUFFER_SIZE];
    char token[4], status[3];
    sprintf(request, "LOU %s %s\n", myUser.uid, myUser.password);

    // Send the request to the Auction Server using UDP
    sendUDPMessage(request, response);

    sscanf(response, "%s %s", token, status);
    if (!strcmp(status, AS_RESPONSETOKEN_ACCEPTED))
    {
        write(1, "success\n", 9);
        myUser.loggedIn = false;
    }
    else
    {
        printf("%s %s\n", token, status);
        scanf("%s", token);
    }
}

// Function to handle unregister request
void handleUnregister()
{
    char request[MAX_BUFFER_SIZE];
    char response[MAX_BUFFER_SIZE];
    char token[4], status[3];
    sprintf(request, "unregister %s %s\n", myUser.uid, myUser.password);

    // Send the request to the Auction Server using UDP

    sscanf(response, "%s %s", token, status);
    if (!strcmp(status, AS_RESPONSETOKEN_ACCEPTED))
    {
        write(1, "success\n", 9);
        myUser.loggedIn = false;
    }
    else
    {
        printf("%s %s\n", token, status);
        scanf("%s", token);
    }
}

// Function to handle exit command
void handleExit()
{
    // Perform local exit operations
    // ...
}

// Function to handle open auction request
void handleOpenAuction(char *name, char *assetFname,
                       char *startValue, char *timeActive)
{
    // Read file data
    int imageFd = open(assetFname, O_RDONLY);
    if (imageFd == -1)
    {
        perror("Error opening image file");
        exit(EXIT_FAILURE);
    }

    char fileBuffer[MAX_BUFFER_SIZE];
    size_t bytesRead;
    size_t bytesTotal;
    while ((bytesRead = read(imageFd, fileBuffer, sizeof(fileBuffer))) > 0)
    {
        bytesTotal += bytesRead;
        // The file data is read into fileBuffer
        // You can process or send it as needed
    }

    char request[MAX_BUFFER_SIZE];
    char response[MAX_BUFFER_SIZE];
    sprintf(request, "OPA %s %s %s %s %s %s %d %s \n", myUser.uid, myUser.password, name, startValue, timeActive, assetFname, bytesTotal, fileBuffer);

    printf("->%s", request);
    scanf("ola %d", &bytesRead);
    // Send the request to the Auction Server using TCP
    sendTCPMessage(request);

    // Receive and handle the response
    // ...
}

// Function to handle close auction request
void handleCloseAuction(const char *uid, const char *aid)
{
    char request[MAX_BUFFER_SIZE];
    sprintf(request, "close %s %s\n", uid, aid);

    // Send the request to the Auction Server using TCP

    // Receive and handle the response
    // ...
}

// Function to handle myauctions request
void handleMyAuctions()
{
    char request[MAX_BUFFER_SIZE];
    char response[MAX_BUFFER_SIZE];
    char token[4], status[3], list[MAX_BUFFER_SIZE][MAX_BUFFER_SIZE];
    sprintf(request, "LMA %s\n", myUser.uid);

    // Send the request to the Auction Server using UDP

    sendUDPMessage(request, response);
    // Receive and handle the response
    sscanf(response, "%s %s %s", token, status, list);
    printf("verificar-->%s", response);
    printf("-->%s", list);
    if (!strcmp(status, AS_RESPONSETOKEN_ACCEPTED))
    {
        write(1, "success\n", 9);
        myUser.loggedIn = true;
    }
    else
    {
        printf("%s %s", token, status);
    }
}

// Function to handle mybids request
void handleMyBids(const char *uid)
{
    char request[MAX_BUFFER_SIZE];
    sprintf(request, "mybids %s\n", uid);

    // Send the request to the Auction Server using UDP

    // Receive and handle the response
    // ...
}

// Function to handle list request
void handleList()
{
    char request[MAX_BUFFER_SIZE];
    sprintf(request, "list\n");

    // Send the request to the Auction Server using UDP

    // Receive and handle the response
    // ...
}

// Function to handle show_asset request
void handleShowAsset(const char *uid, const char *aid)
{
    // Implement file handling to store the received image data
    // ...

    char request[MAX_BUFFER_SIZE];
    sprintf(request, "show_asset %s %s\n", uid, aid);

    // Send the request to the Auction Server using TCP

    // Receive and handle the response
    // ...
}

// Function to handle bid request
void handleBid(const char *uid, const char *aid, const char *value)
{
    char request[MAX_BUFFER_SIZE];
    sprintf(request, "bid %s %s %s\n", uid, aid, value);

    // Send the request to the Auction Server using TCP

    // Receive and handle the response
    // ...
}

// Function to handle show_record request
void handleShowRecord(const char *uid, const char *aid)
{
    char request[MAX_BUFFER_SIZE];
    sprintf(request, "show_record %s %s\n", uid, aid);

    // Send the request to the Auction Server using UDP

    // Receive and handle the response
    // ...
}

// Main function
int main(int argc, char *argv[])
{
    write(1, "HelloWorld\n", 11);
    char *command[128];
    myUser.loggedIn = false;
    while (1)
    {
        scanf("%s", command);
        printf("-> %s\n", command);

        // login command
        if (!strcmp(command, "login"))
        {
            if (!myUser.loggedIn)
            {
                scanf("%s", myUser.uid);
                scanf("%s", myUser.password);
                handleLogin();
            }
            else
            {
                write(1, "You are already logged in\n", 27);
            }
        }
        // logout command
        else if (!strcmp(command, "logout"))
        {
            if (myUser.loggedIn)
            {
                handleLogout();
            }
            else
            {
                write(1, "You are not logged in\n", 23);
            }
            // unregister command
        }
        // unregister command
        else if (!strcmp(command, "unregister"))
        {
            if (myUser.loggedIn)
            {
                handleUnregister();
            }
            else
            {
                write(1, "You are not logged in\n", 23);
            }
        }
        // command myauctions
        else if (!strcmp(command, "myauctions"))
        {
            if (myUser.loggedIn)
            {
                handleMyAuctions();
            }
            else
            {
                write(1, "You are not logged in\n", 23);
            }
        }
        // command open auction
        else if (!strcmp(command, "open"))
        {
            char *name[128], asset_fname[128], start_value[128], time_active[128];
            scanf("%s", name);
            scanf("%s", asset_fname);
            scanf("%s", start_value);
            scanf("%s", time_active);
            if (myUser.loggedIn)
            {
                handleOpenAuction(name, asset_fname, start_value, time_active);
            }
            else
            {
                write(1, "You are not logged in\n", 23);
            }
        }
    }
    handleLogin("000423", "password");

    return 0;
}
