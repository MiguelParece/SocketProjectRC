// server side code

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/sendfile.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <ctype.h>

// #define AS_IP "tejo.tecnico.ulisboa.pt"
#define AS_IP_BASE "localhost"
#define AS_PORT_BASE "58016"

#define STATUS_OK "OK"
#define STATUS_NOK "NOK"
#define STATUS_ERR "ERR"
#define STATUS_NLG "NLG"
#define STATUS_EAU "EAU"
#define STATUS_EOW "EOW"
#define STATUS_END "END"
#define STATUS_REF "REF"
#define STATUS_ILG "ILG"
#define STATUS_REG "REG"
#define STATUS_UNR "UNR"

#define OPEN_AUCTION_REQUEST "OPA"
#define OPEN_AUCTION_RESPONSE "ROA"

#define CLOSE_AUCTION_REQUEST "CLS"
#define CLOSE_AUCTION_RESPONSE "RCL"

#define SHOW_ASSET_REQUEST "SAS "
#define SHOW_ASSET_RESPONSE "RSA"

#define BID_REQUEST "BID"
#define BID_RESPONSE "RBD"

#define LOGIN_REQUEST "LIN"
#define LOGIN_RESPONSE "RLI"

#define LOGOUT_REQUEST "LOU"
#define LOGOUT_RESPONSE "RLO"

#define UNREGISTER_REQUEST "UNR"
#define UNREGISTER_RESPONSE "RUR"

#define LIST_MY_AUC_REQUEST "LMA"
#define LIST_MY_AUC_RESPONSE "RMA"

#define LIST_MY_BID_REQUEST "LMB"
#define LIST_MY_BID_RESPONSE "RMB"

#define LIST_AUC_REQUEST "LST"
#define LIST_AUC_RESPONSE "RLS"

#define SHOW_REC_REQUEST "SRC"
#define SHOW_REC_RESPONSE "RRC"

#define MAX_BUFFER_SIZE 1024 * 100
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
// global variables
char AS_IP[128];
char AS_PORT[128];

// check valid UID
int checkUID(char *UID)
{
    int i;
    if (strlen(UID) != 6)
        return 0;
    for (i = 0; i < 6; i++)
    {
        if (!isdigit(UID[i]))
        {
            return 0;
        }
    }
    return 1;
}

// check valid password
int checkPassword(char *password)
{
    int i;
    if (strlen(password) != 8)
        return 0;
    for (i = 0; i < strlen(password); i++)
    {
        if (!isalnum(password[i]))
        {
            return 0;
        }
    }
    return 1;
}

// check valid AID
int checkAID(char *AID)
{
    int i;
    if (strlen(AID) != 3)
        return 0;
    for (i = 0; i < 3; i++)
    {
        if (!isdigit(AID[i]))
        {
            return 0;
        }
    }
    return 1;
}

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

int reciveTCPFile(char *message)
{
    int fd, errcode;
    ssize_t n;
    struct addrinfo hints, *res;
    struct sockaddr_in addr;
    char buffer[MAX_BUFFER_SIZE];
    fd = socket(AF_INET, SOCK_STREAM, 0); // TCP socket
    if (fd == -1)
        return 1; // error
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;       // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP socket

    errcode = getaddrinfo(AS_IP, AS_PORT, &hints, &res);
    if (errcode != 0)
        return 1; // error

    n = connect(fd, res->ai_addr, res->ai_addrlen);
    if (n == -1)
        return 1; // error

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
        return 0;
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
    return 1;
}

// Function to handle login request
void handleLogin()
{
    char request[MAX_BUFFER_SIZE];
    char response[MAX_BUFFER_SIZE];
    char token[4], status[3];

    sprintf(request, "%s %s %s\n", LOGIN_REQUEST, myUser.uid, myUser.password);

    // Send the request to the Auction Server using UDP
    sendUDPMessage(request, response);

    printf("reses->%s\n", response);
    // handle response
    sscanf(response, "%s %s", token, status);
    if (!strcmp(token, LOGIN_RESPONSE) || !strcmp(status, STATUS_OK) || !strcmp(status, STATUS_REG))
    {
        write(1, "success\n", 9);
        myUser.loggedIn = true;
    }
    else
    {
        printf("err: %s %s\n", token, status);
    }
}

// Function to handle logout request
void handleLogout()
{
    char request[MAX_BUFFER_SIZE];
    char response[MAX_BUFFER_SIZE];
    char token[4], status[3];
    sprintf(request, "%s %s %s\n", LOGOUT_REQUEST, myUser.uid, myUser.password);

    // Send the request to the Auction Server using UDP
    sendUDPMessage(request, response);

    sscanf(response, "%s %s", token, status);
    if (!strcmp(token, LOGOUT_RESPONSE) && !strcmp(status, STATUS_OK))
    {
        write(1, "success\n", 9);
        myUser.loggedIn = false;
    }
    else
    {
        printf("err: %s %s\n", token, status);
        scanf("%s", token);
    }
}

// Function to handle unregister request
void handleUnregister()
{
    char request[MAX_BUFFER_SIZE];
    char response[MAX_BUFFER_SIZE];
    char token[4], status[3];
    sprintf(request, "%s %s %s\n", UNREGISTER_REQUEST, myUser.uid, myUser.password);

    // Send the request to the Auction Server using UDP
    sendUDPMessage(request, response);

    sscanf(response, "%s %s", token, status);
    if (!strcmp(token, UNREGISTER_RESPONSE) && !strcmp(status, STATUS_OK))
    {
        write(1, "success\n", 9);
        myUser.loggedIn = false;
    }
    else
    {
        printf("err: %s %s\n", token, status);
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

    sprintf(request, "%s %s %s %s %s %s %s %ld ", OPEN_AUCTION_REQUEST, myUser.uid, myUser.password, name, startValue, timeActive, assetFname, bytesTotal);
    // Send the request to the Auction Server using TCP
    sendTCPMessage(request, imageFd, response);
    close(imageFd);

    // Handle the response
    char token[4];
    char status[3];
    char aid[7];
    sscanf(response, "%s %s %s\n", token, status, aid);
    if (!strcmp(token, OPEN_AUCTION_RESPONSE) && !strcmp(status, STATUS_OK))
    {
        printf("Auction %s opened successfully.\n", aid);
    }
    else if (!strcmp(status, STATUS_NLG))
    {
        printf("User not logged in.\n");
    }
    else if (!strcmp(status, STATUS_NOK))
    {
        printf("Auc could not be started.\n");
    }
}

void handleCloseAuction(const char *aid)
{
    char request[MAX_BUFFER_SIZE];
    char response[MAX_BUFFER_SIZE];
    sprintf(request, "%s %s %s %s\n", CLOSE_AUCTION_REQUEST, myUser.uid, myUser.password, aid);
    // Send the request to the Auction Server using TCP
    sendTCPMessage(request, 0, response);
    // Handle the response
    char token[4], status[3];
    sscanf(response, "%s %s", token, status);

    if (!strcmp(token, CLOSE_AUCTION_RESPONSE))
    {
        if (!strcmp(status, STATUS_OK))
        {
            printf("Auction %s closed successfully.\n", aid);
        }
        else if (!strcmp(status, STATUS_NOK))
        {
            printf("Auction %s could not be closed.\n", aid);
        }
    }
    else
    {

        printf("Err in response\n");
    }
}

// Function to handle myauctions request
void handleMyAuctions()
{
    char request[MAX_BUFFER_SIZE];
    char response[MAX_BUFFER_SIZE];
    char token[4], status[3], list[MAX_BUFFER_SIZE];
    sprintf(request, "%s %s\n", LIST_MY_AUC_REQUEST, myUser.uid);

    // Send the request to the Auction Server using UDP

    sendUDPMessage(request, response);
    // Receive and handle the response
    int n = 0;
    sscanf(response, "%s %s %n", token, status, &n);
    strcpy(list, response + n);
    if (!strcmp(token, LIST_MY_BID_RESPONSE) && !strcmp(status, STATUS_OK))
    {
        write(1, "success\n", 9);
        printf("Auctions owned by user %s:\n", myUser.uid);
        printf("%s\n", list);
    }
    else
    {
        if (!strcmp(status, STATUS_NLG))
        {
            printf("User %s is not logged in the server.\n", myUser.uid);
        }
        else if (!strcmp(status, STATUS_NOK))
        {
            printf("User %s does not have any auctions.\n", myUser.uid);
        }
    }
}

// Function to handle mybids request
void handleMyBids()
{
    char request[MAX_BUFFER_SIZE];
    char response[MAX_BUFFER_SIZE];
    sprintf(request, "%s %s\n", LIST_MY_BID_REQUEST, myUser.uid);

    // Send the request to the Auction Server using UDP
    sendUDPMessage(request, response);

    // Receive and handle the response
    char token[4], status[3], list[MAX_BUFFER_SIZE];
    int n = 0;
    sscanf(response, "%s %s %n", token, status, &n);
    strcpy(list, response + n);
    if (!strcmp(token, LIST_MY_BID_RESPONSE) && !strcmp(status, STATUS_OK))
    {
        write(1, "success\n", 9);
        printf("Auctions where user %s has bid:\n", myUser.uid);
        printf("%s\n", list);
    }
    else
    {
        if (!strcmp(status, STATUS_NLG))
        {
            printf("User %s is not logged in the server.\n", myUser.uid);
        }
        else if (!strcmp(status, STATUS_NOK))
        {
            printf("User %s does not have any bids.\n", myUser.uid);
        }
    }
}

// Function to handle list request
void handleList()
{
    char request[MAX_BUFFER_SIZE];
    char response[MAX_BUFFER_SIZE];

    // clear response
    sprintf(request, "%s\n", LIST_AUC_REQUEST);

    // Send the request to the Auction Server using UDP
    sendUDPMessage(request, response);

    char token[4], status[3], list[MAX_BUFFER_SIZE];
    int n = 0;
    sscanf(response, "%s %s %n", token, status, &n);
    strcpy(list, response + n);
    if (!strcmp(token, LIST_AUC_RESPONSE) && !strcmp(status, STATUS_OK))
    {
        write(1, "success\n", 9);
        printf("Auctions available:\n");
        printf("%s\n", list);
    }
    else
    {

        if (!strcmp(status, STATUS_NOK))
        {
            printf("No auctions has started.\n");
        }
    }
}

// Function to handle show_asset request
void handleShowAsset(const char *aid)
{

    char request[MAX_BUFFER_SIZE];
    sprintf(request, "%s %s\n", SHOW_ASSET_REQUEST, aid);
    // Send the request to the Auction Server using TCP
    int i = reciveTCPFile(request);
    if (i == 1)
    {
        printf("Asset Acquired\n");
    }
}

// Function to handle bid request
void handleBid(const char *aid, const char *value)
{
    char request[MAX_BUFFER_SIZE];
    char response[MAX_BUFFER_SIZE];

    sprintf(request, "%s %s %s %s %s\n", BID_REQUEST, myUser.uid, myUser.password, aid, value);

    // Send the request to the Auction Server using TCP
    sendTCPMessage(request, 0, response);

    char token[4], status[3];
    sscanf(response, "%s %s", token, status);
    if (!strcmp(token, BID_RESPONSE))
    {
        if (!strcmp(status, STATUS_OK))
        {
            printf("Bid placed successfully.\n");
        }
        else if (!strcmp(status, STATUS_NOK))
        {
            printf("Bid could not be placed.\n");
        }
        else if (!strcmp(status, STATUS_NLG))
        {
            printf("User is not logged in.\n");
        }
        else if (!strcmp(status, STATUS_ILG))
        {
            printf("You cannot bid on your auctions.\n");
        }
    }
    else
    {
        printf("Err in response\n");
    }
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
    char list[MAX_BUFFER_SIZE];
    char host_UID[MAX_BUFFER_SIZE];
    char auction_name[MAX_BUFFER_SIZE];
    char asset_fname[MAX_BUFFER_SIZE];
    int startValue;
    int timeactive;
    int n, year, month, day, hour, minute, second;

    sscanf(response, "%s %s %s %s %s %d %4d-%02d-%02d %02d:%02d:%02d %d %n", token, status, host_UID, auction_name, asset_fname, &startValue, &year, &month, &day, &hour, &minute, &second, &timeactive, &n);
    // timestring
    char start_date_time[20];
    sprintf(start_date_time, "%4d-%02d-%02d %02d:%02d:%02d", year, month, day, hour, minute, second);

    strcpy(list, response + n);

    if (!strcmp(token, SHOW_REC_RESPONSE) && !strcmp(status, STATUS_OK))
    {
        printf("Auction %s:\n", aid);
        printf("Host UID: %s\n", host_UID);
        printf("Auction name: %s\n", auction_name);
        printf("Asset filename: %s\n", asset_fname);
        printf("Start value: %d\n", startValue);
        printf("Start time: %s\n", start_date_time);
        printf("Time active: %dc\n", timeactive);

        printf("Bids:\n");
        printf("%s\n", list);
    }
    else if (!strcmp(status, STATUS_NOK))
    {
        printf("Auction %s does not exist.\n", aid);
    }
    else
    {
        printf("Err in response\n");
    }
}

// Main function
int main(int argc, char *argv[])
{
    printf("Welcome to the Auction Client!\n");
    strcpy(AS_PORT, AS_PORT_BASE);
    strcpy(AS_IP, AS_IP_BASE);
    int i = 1;
    // check is verbose is on and chosen port
    while (i < argc)
    {

        if (!strcmp(argv[i], "-n"))
        {
            strcpy(AS_IP, argv[i + 1]);
        }
        if (!strcmp(argv[i], "-p"))
        {
            strcpy(AS_PORT, argv[i + 1]);
        }
        i++;
    }

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
                if (checkUID(myUser.uid) && checkPassword(myUser.password))
                {
                    handleLogin();
                }
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
            if (myUser.loggedIn && checkAID(aid))
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
            scanf("%s", aid);
            if (checkAID(aid))
                handleShowAsset(aid);
        }
        // command bid
        else if (!strcmp(command, "bid") || !strcmp(command, "b"))
        {
            char aid[128], value[128];
            if (myUser.loggedIn && checkAID(aid))
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

            handleList();
        }
        // command show_record
        else if (!strcmp(command, "show_record") || !strcmp(command, "sr"))
        {
            char aid[128];

            scanf("%s", aid);
            if (checkAID(aid))
                handleShowRecord(aid);
        }
        else
        {
            write(1, "Invalid command\n", 16);
        }
    }
}
