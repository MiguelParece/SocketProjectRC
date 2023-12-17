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
#include <sys/sendfile.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <sys/stat.h>

#define AS_IP "tejo.tecnico.ulisboa.pt"
// #define AS_IP "localhost"
#define AS_PORT "58011"

#define AS_RESPONSETOKEN_LOGIN "RLU"
#define AS_RESPONSETOKEN_ACCEPTED "OK"
#define AS_RESPONSETOKEN_LOGIN_FAIL "NOK"
#define AS_RESPONSETOKEN_LOGIN_REG "REG"

#define MAX_BUFFER_SIZE 1024 * 1000
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
    // clear buffer
    memset(buffer, 0, sizeof(buffer));

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
    n = recvfrom(fd, response, MAX_BUFFER_SIZE, 0,
                 (struct sockaddr *)&addr, &addrlen);
    if (n == -1) /*error*/
        exit(1);
    freeaddrinfo(res);
    close(fd);
}

// Function to send a message to the Auction Server using TCP
void sendTCPMessage(const char *message, int fdImage, char *response)
{
    int fd, errcode;
    ssize_t n;
    struct addrinfo hints, *res;
    struct sockaddr_in addr;
    char buffer[MAX_BUFFER_SIZE];
    memset(buffer, 0, sizeof(buffer));

    fd = socket(AF_INET, SOCK_STREAM, 0); // TCP socket
    if (fd == -1)
        exit(1); // error

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;       // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP socket

    errcode = getaddrinfo(AS_IP, AS_PORT, &hints, &res);
    if (errcode != 0)
        exit(1); // error

    n = connect(fd, res->ai_addr, res->ai_addrlen);
    if (n == -1)
        exit(1); // error

    size_t message_len = strlen(message);
    size_t bytes_sent = 0;
    while (bytes_sent < message_len)
    {
        n = write(fd, message + bytes_sent, message_len - bytes_sent);
        if (n == -1)
        {
            perror("Error writing to socket");
            exit(EXIT_FAILURE);
        }
        bytes_sent += n;
    }
    if (fdImage)
    { // Use sendfile() to send the contents of the fdImage file
        off_t offset = 0;
        struct stat file_stat;
        if (fstat(fdImage, &file_stat) == -1)
        {
            perror("Error getting file status");
            exit(EXIT_FAILURE);
        }

        n = sendfile(fd, fdImage, &offset, file_stat.st_size);
        if (n == -1)
        {
            perror("Error sending file");
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

    strcpy(response, buffer);
    freeaddrinfo(res);
    close(fd);
}

void reciveTCPFile(char *message)
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

    errcode = getaddrinfo(AS_IP, AS_PORT, &hints, &res);
    if (errcode != 0)
        exit(1); // error

    n = connect(fd, res->ai_addr, res->ai_addrlen);
    if (n == -1)
        exit(1); // error

    // Send the message to the server
    size_t message_len = strlen(message);
    size_t bytes_sent = 0;
    while (bytes_sent < message_len)
    {
        n = write(fd, message + bytes_sent, message_len - bytes_sent);
        if (n == -1)
        {
            perror("Error writing to socket");
            exit(EXIT_FAILURE);
        }
        bytes_sent += n;
    }

    int nSpaces = 0;
    int i = 0;
    char token[3];
    char status[3];
    char Fname[28];
    char Fsize[8];
    // clear buffer, token ,status, Fname, Fsize
    memset(buffer, 0, sizeof(buffer));
    memset(token, 0, sizeof(token));
    memset(status, 0, sizeof(status));
    memset(Fname, 0, sizeof(Fname));
    memset(Fsize, 0, sizeof(Fsize));
    while (nSpaces < 4)
    {

        if (read(fd, buffer + i, 1))
        {
            if (buffer[i] == ' ')
            {
                nSpaces++;
                if (nSpaces == 4)
                    strcpy(Fsize, buffer);
            }
            i++;
        }
    }
    // printf("->%s\n", buffer);
    //  get the token and status and Fname and Fsize from buffer
    sscanf(buffer, "%s %s %s %s", token, status, Fname, Fsize);
    printf("Fname->%s\n", Fname);
    printf("Fsize->%s\n", Fsize);
    // open file
    int fdFile = open(Fname, O_WRONLY | O_CREAT, 0666);
    if (fdFile == -1)
    {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }
    // read file
    int bytes_read = 0;
    int bytes_total = atoi(Fsize);
    while (bytes_read < bytes_total)
    {
        n = read(fd, buffer, sizeof(buffer));
        if (n == -1)
        {
            perror("Error reading from socket");
            exit(EXIT_FAILURE);
        }
        bytes_read += n;
        write(fdFile, buffer, n);
    }

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

        printf("reses->%s\n", response);
        // handle response
        sscanf(response, "%s %s", token, status);
        if (!strcmp(status, AS_RESPONSETOKEN_ACCEPTED) || !strcmp(status, AS_RESPONSETOKEN_LOGIN_REG))
        {
            write(1, "success\n", 9);
            myUser.loggedIn = true;
        }
        else
        {
            printf("%s %s\n", token, status);
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
    sprintf(request, "UNR %s %s\n", myUser.uid, myUser.password);

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

    long int bytesTotal = lseek(imageFd, 0, SEEK_END);
    lseek(imageFd, 0, SEEK_SET);

    char request[MAX_BUFFER_SIZE];
    char response[MAX_BUFFER_SIZE];

    sprintf(request, "OPA %s %s %s %s %s %s %ld ", myUser.uid, myUser.password, name, startValue, timeActive, assetFname, bytesTotal);
    // Send the request to the Auction Server using TCP
    sendTCPMessage(request, imageFd, response);

    // Handle the response
    char status[3];
    char aid[7];
    sscanf(response, "ROA %s %s", status, aid);

    if (!strcmp(status, "NOK"))
    {
        printf("Auction could not be started.\n");
    }
    else if (!strcmp(status, "NLG"))
    {
        printf("User not logged in.\n");
    }
    else if (!strcmp(status, "OK"))
    {
        printf("Auction started successfully. Auction ID: %s\n", aid);
        // Store local copy of the asset file using the filename Fname
        // ...
    }
    else
    {
        printf("Invalid response status.\n");
    }

    close(imageFd);
    // Receive and handle the response
    // ...
}

void handleCloseAuction(const char *aid)
{
    char request[MAX_BUFFER_SIZE];
    char response[MAX_BUFFER_SIZE];
    sprintf(request, "CLS %s %s %s\n", myUser.uid, myUser.password, aid);
    // Send the request to the Auction Server using TCP
    sendTCPMessage(request, 0, response);

    // Handle the response
    char token[4], status[3];
    sscanf(response, "RCL %s", status);

    if (!strcmp(status, "OK"))
    {
        printf("Auction %s closed successfully.\n", aid);
    }
    else if (!strcmp(status, "NLG"))
    {
        printf("User not logged in.\n");
    }
    else if (!strcmp(status, "EAU"))
    {
        printf("Auction %s does not exist.\n", aid);
    }
    else if (!strcmp(status, "EOW"))
    {
        printf("Auction %s is not owned by user %s.\n", aid, myUser.uid);
    }
    else if (!strcmp(status, "END"))
    {
        printf("Auction %s owned by user %s has already finished.\n", aid, myUser.uid);
    }
}

// Function to handle myauctions request
void handleMyAuctions()
{
    char request[MAX_BUFFER_SIZE];
    char response[MAX_BUFFER_SIZE * 10];
    char token[4], status[3], list[MAX_BUFFER_SIZE];
    sprintf(request, "LMA %s\n", myUser.uid);

    // Send the request to the Auction Server using UDP

    sendUDPMessage(request, response);
    // Receive and handle the response
    int n = 0;
    sscanf(response, "%s %s %n", token, status, &n);
    strcpy(list, response + n);
    if (!strcmp(status, AS_RESPONSETOKEN_ACCEPTED))
    {
        write(1, "success\n", 9);
        printf("Auctions owned by user %s:\n", myUser.uid);
        printf("%s\n", list);
    }
    else
    {
        printf("%s %s\n", token, status);
    }
}

// Function to handle mybids request
void handleMyBids()
{
    char request[MAX_BUFFER_SIZE];
    char response[MAX_BUFFER_SIZE];
    sprintf(request, "LMB %s\n", myUser.uid);

    // Send the request to the Auction Server using UDP
    sendUDPMessage(request, response);

    // Receive and handle the response
    printf("->%s", response);
    // ...
}

// Function to handle list request
void handleList()
{
    char request[MAX_BUFFER_SIZE];
    char response[MAX_BUFFER_SIZE];

    // clear response
    sprintf(request, "LST\n");

    // Send the request to the Auction Server using UDP
    sendUDPMessage(request, response);

    printf("->%s\n", response);
    // Receive and handle the response
    // ...
}

// Function to handle show_asset request
void handleShowAsset(const char *aid)
{

    char request[MAX_BUFFER_SIZE];
    sprintf(request, "SAS %s\n", aid);
    // Send the request to the Auction Server using TCP
    reciveTCPFile(request);
}

// Function to handle bid request
void handleBid(const char *aid, const char *value)
{
    char request[MAX_BUFFER_SIZE];
    char response[MAX_BUFFER_SIZE];

    sprintf(request, "BID %s %s %s %s\n", myUser.uid, myUser.password, aid, value);

    // Send the request to the Auction Server using TCP
    sendTCPMessage(request, 0, response);

    printf("->%s", response);
    // Receive and handle the response
    // ...
}

// Function to handle show_record request

void handleShowRecord(const char *aid)
{
    char request[MAX_BUFFER_SIZE];
    char response[MAX_BUFFER_SIZE];
    sprintf(request, "SRC %s\n", aid);

    // Send the request to the Auction Server using UDP
    sendUDPMessage(request, response);

    printf("->%s\n", response);

    // Parse the response
    char token[4];
    char status[4];
    char host_UID[MAX_BUFFER_SIZE];
    char auction_name[MAX_BUFFER_SIZE];
    char asset_fname[MAX_BUFFER_SIZE];
    char start_value[MAX_BUFFER_SIZE];
    char start_date_time[20];
    char start_date[11];
    char timeactive[7];

    sscanf(response, "%s %s %s %s %s %s %s %s %s", token, status, host_UID, auction_name, asset_fname, start_value, start_date, start_date_time, timeactive);

    if (strcmp(status, "NOK") == 0)
    {
        printf("Auction does not exist.\n");
    }
    else if (strcmp(status, "OK") == 0)
    {
        printf("Auction details:\n");
        printf("Host UID: %s\n", host_UID);
        printf("Auction Name: %s\n", auction_name);
        printf("Asset File Name: %s\n", asset_fname);
        printf("Start Value: %s\n", start_value);
        printf("Start Date and Time: %s - %s\n", start_date, start_date_time);
        printf("Time Active: %s seconds\n", timeactive);

        // Calculate the length of the parsed part
        size_t parsed_length = strlen(token) + strlen(status) + strlen(host_UID) + strlen(auction_name) + strlen(asset_fname) +
                               strlen(start_value) + strlen(start_date_time) + strlen(start_date) + strlen(timeactive) + 9;

        // Check if there are bids
        printf("Bids:\n %s\n", response + parsed_length);
    }
}
// ...

int main(int argc, char *argv[])
{
    printf("Welcome to the Auction Client!\n");
    char command[128];
    while (1)
    {
        scanf("%s", command);

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
            char name[128], asset_fname[128], start_value[128], time_active[128];
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
        // command close
        else if (!strcmp(command, "close"))
        {
            char aid[128];
            scanf("%s", aid);
            write(1, "MAMAS\n", 6);
            if (myUser.loggedIn)
            {
                handleCloseAuction(aid);
            }
            else
            {
                write(1, "You are not logged in\n", 23);
            }
        }
        // command show_assets
        else if (!strcmp(command, "show_assets") || !strcmp(command, "sa"))
        {
            char aid[128];
            if (myUser.loggedIn)
            {
                scanf("%s", aid);
                handleShowAsset(aid);
            }
            else
            {
                write(1, "You are not logged in\n", 23);
            }
        }
        // command bid
        else if (!strcmp(command, "bid") || !strcmp(command, "b"))
        {
            char aid[128], value[128];
            if (myUser.loggedIn)
            {
                scanf("%s", aid);
                scanf("%s", value);
                handleBid(aid, value);
            }
            else
            {
                write(1, "You are not logged in\n", 23);
            }
        }
        // command mybids
        else if (!strcmp(command, "mybids") || !strcmp(command, "mb"))
        {
            if (myUser.loggedIn)
            {
                handleMyBids();
            }
            else
            {
                write(1, "You are not logged in\n", 23);
            }
        }
        // command list
        else if (!strcmp(command, "list"))
        {
            if (myUser.loggedIn)
            {
                handleList();
            }
            else
            {
                write(1, "You are not logged in\n", 23);
            }
        }
        // command show_record
        else if (!strcmp(command, "show_record") || !strcmp(command, "sr"))
        {
            char aid[128];
            if (myUser.loggedIn)
            {
                scanf("%s", aid);
                handleShowRecord(aid);
            }
            else
            {
                write(1, "You are not logged in\n", 23);
            }
        }
    }
}
