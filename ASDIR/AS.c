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

// Defines
#define AS_PORT_BASE "58011"

#define MAX_BUFFER_SIZE 1024 * 100
#define MAX_FILENAME_LENGTH 24
#define MAX_FILE_SIZE 10000000 // 10 MB

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

#define OPEN_AUCTION_REQUEST "OPA "
#define OPEN_AUCTION_RESPONSE "ROA"

#define CLOSE_AUCTION_REQUEST "CLS "
#define CLOSE_AUCTION_RESPONSE "RCL"

#define SHOW_ASSET_REQUEST "SAS "
#define SHOW_ASSET_RESPONSE "RSA"

#define BID_REQUEST "BID "
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

struct User
{
    char uid[7];
    char password[20];
};

// Global variables
bool verbose = false;
char AS_PORT[128];

// UDP addrs info as global variables
socklen_t addrlen;
struct sockaddr_in addr;

// structs
typedef struct
{
    int auction_bid_id; // auction id
    int user_bid_id;    // user id
    int bid_ammount;    // bid ammount
    char bid_datetime[30];
    long bid_sec_time; // time in seconds since the start of the auction
    bool active;       // auction active
} BID;

typedef struct
{
    int AID;     // auction idº
    bool active; // auction active
} auction;

typedef struct
{
    int no_bids;   // Number of bids in the list
    BID bids[999]; // Assuming a maximum of 50 bids, adjust as needed
} BIDLIST;

typedef struct
{
    int no_auctions;  // Number of auctions in the list
    auction AID[999]; // Assuming a maximum of 50 auctions, adjust as needed
} AUCTIONLIST;

volatile sig_atomic_t num_children = 0;

// declare all functions bellow

// handle users
int createUserDir(char *UID)
{
    char userDir[35];

    if (strlen(UID) != 6)
        return 0;
    snprintf(userDir, sizeof(userDir), "USERS/%s", UID);
    mkdir(userDir, 0777);
    snprintf(userDir, sizeof(userDir), "USERS/%s/HOSTED", UID);
    mkdir(userDir, 0777);
    snprintf(userDir, sizeof(userDir), "USERS/%s/BIDDED", UID);
    mkdir(userDir, 0777);

    return 1;
}

int CreateLogin(char *UID)
{
    char loginName[35];
    FILE *fp;

    if (strlen(UID) != 6)
        return 0;

    // Use snprintf to avoid potential buffer overflow
    snprintf(loginName, sizeof(loginName), "USERS/%s/%s_login.txt", UID, UID);

    fp = fopen(loginName, "w");
    if (fp == NULL)
    {
        printf("Error opening file!\n");
        return 0;
    }
    fprintf(fp, "Logged in\n");

    fclose(fp);

    return 1;
}

int EraseLogin(char *UID)
{
    char loginName[35];

    if (strlen(UID) != 6)
        return 0;

    // Use snprintf to avoid potential buffer overflow
    snprintf(loginName, sizeof(loginName), "USERS/%s/%s_login.txt", UID, UID);

    // Use unlink to delete the file
    if (unlink(loginName) != 0)
        return 0; // Return 0 on failure

    return 1; // Return 1 on success
}

int CreatePassword(char *UID, char *password)
{
    char passwordName[35];
    FILE *fp;

    if (strlen(UID) != 6)
        return 0;

    // Use snprintf to avoid potential buffer overflow
    snprintf(passwordName, sizeof(passwordName), "USERS/%s/%s_password.txt", UID, UID);

    fp = fopen(passwordName, "w");
    if (fp == NULL)
        return 0;

    fprintf(fp, "%s\n", password);

    fclose(fp);

    return 1;
}

int erasePassword(char *UID)
{
    char passwordName[35];

    if (strlen(UID) != 6)
        return 0;

    // Use snprintf to avoid potential buffer overflow
    snprintf(passwordName, sizeof(passwordName), "USERS/%s/%s_password.txt", UID, UID);

    // Use unlink to delete the file
    if (unlink(passwordName) != 0)
        return 0; // Return 0 on failure

    return 1; // Return 1 on success
}

// check if user is registered
int CheckUserRegistered(char *UID)
{
    char userDir[35];

    if (strlen(UID) != 6)
        return 0;

    // Use snprintf to avoid potential buffer overflow
    snprintf(userDir, sizeof(userDir), "USERS/%s/%s_password.txt", UID, UID);

    // Check if the directory exists
    if (access(userDir, F_OK) == -1)
        return 0;

    return 1;
}

// check if the user is logged in
int CheckLogin(char *UID)
{
    // check if the user is registered
    if (!CheckUserRegistered(UID))
    {
        printf("User is not registered\n");
        return 0;
    }
    printf("UID: %s\n", UID);
    char userID[45];
    sprintf(userID, "USERS/%s/%s_login.txt", UID, UID);
    printf("userID: %s\n", userID);

    if (access(userID, F_OK) == -1)
    {
        printf("User is not logged in\n");
        return 0;
    }
    else
    {

        return 1;
    }
}

// check if password is correct
int CheckPassword(char *UID, char *password)
{
    char passwordName[35];
    FILE *fp;
    char pass[20];

    if (strlen(UID) != 6)
        return 0;

    // Use snprintf to avoid potential buffer overflow
    snprintf(passwordName, sizeof(passwordName), "USERS/%s/%s_password.txt", UID, UID);

    fp = fopen(passwordName, "r");
    if (fp == NULL)
        return 0;

    fscanf(fp, "%s", pass);

    fclose(fp);

    if (strcmp(pass, password) == 0)
        return 1;

    return 0;
}

// get next auction id
int getSequencialAuctionID()
{
    // get the number of directories in the auction directory
    struct dirent **filelist;
    int entries, len;
    char dirname[20];
    char pathname[32];
    // Create the directory path for the bids within the auction directory
    sprintf(dirname, "AUCTIONS/");

    // Scan the directory and get the list of files
    entries = scandir(dirname, &filelist, 0, alphasort);

    // Check if there are no entries in the directory
    if (entries <= 0)
        return 0;

    return entries - 1;
}

// handle auctions
int CreateAUCTIONDir()
{
    char AID_dirname[15];
    char BIDS_dirname[20];
    int ret;

    int AID = getSequencialAuctionID();

    // Create the directory path for the auction
    sprintf(AID_dirname, "AUCTIONS/%03d", AID);

    // Attempt to create the auction directory with read, write, and execute permissions for the owner
    ret = mkdir(AID_dirname, 0700);

    // Check if the directory creation was successful
    if (ret == -1)
        return 0;

    // Create the directory path for the bids within the auction directory
    sprintf(BIDS_dirname, "AUCTIONS/%03d/BIDS", AID);

    // Attempt to create the bids directory within the auction directory
    ret = mkdir(BIDS_dirname, 0700);

    // Check if the bids directory creation was unsuccessful
    if (ret == -1)
    {
        // If unsuccessful, remove the previously created auction directory
        rmdir(AID_dirname);
        return 0;
    }

    // Return 1 to indicate successful creation of both auction and bids directories
    return AID;
}

// create host file if the hosted directory in the user
int CreateHostFile(int AID, char *UID)
{
    char hostName[35];
    FILE *fp;

    if (strlen(UID) != 6)
        return 0;

    // Use snprintf to avoid potential buffer overflow
    snprintf(hostName, sizeof(hostName), "USERS/%s/HOSTED/%03d.txt", UID, AID);

    fp = fopen(hostName, "w");
    if (fp == NULL)
    {
        printf("Error opening file!\n");
        return 0;
    }
    fprintf(fp, "Hosted\n");

    fclose(fp);

    return 1;
}

// Start auction (create start file)
int StartAuction(int AID, char *UID, char *nameDescription, char *fileName, int startValue, int timeactive)
{
    // create the start file
    char startName[35];
    snprintf(startName, sizeof(startName), "AUCTIONS/%03d/Start_%03d.txt", AID, AID);
    FILE *fp;
    fp = fopen(startName, "w");
    if (fp == NULL)
    {
        printf("Error opening file!\n");
        return 0;
    }
    // convert the timeactive to seconds
    int timeactive_seconds = timeactive * 60;
    // get the current time
    time_t fulltime;
    struct tm *currenttime;
    time(&fulltime);
    currenttime = gmtime(&fulltime);

    // create time string
    char timeString[20];
    sprintf(timeString, "%4d-%02d-%02d %02d:%02d:%02d", currenttime->tm_year + 1900, currenttime->tm_mon + 1, currenttime->tm_mday, currenttime->tm_hour, currenttime->tm_min, currenttime->tm_sec);

    // write in the start file UID name asset fname start value timeactive start datetime start fulltime
    fprintf(fp, "%s %s %s %d %d %s %ld \n", UID, nameDescription, fileName, startValue, timeactive_seconds, timeString, fulltime);
    fclose(fp);
    return 1;
}

// get the start fulltime of the auction
int GetAuctionStartFullTime(int AID)
{
    // access the start file
    char startName[35];
    snprintf(startName, sizeof(startName), "AUCTIONS/%03d/Start_%03d.txt", AID, AID);
    // check if the file exists
    if (access(startName, F_OK) == -1)
    {
        printf("Start file does not exist\n");
        return 0;
    }
    // read the start fulltime from the file
    FILE *fp;
    fp = fopen(startName, "r");
    if (fp == NULL)
    {
        printf("Error opening file!\n");
        return 0;
    }
    // read the start fulltime
    long start_fulltime;
    fscanf(fp, "%*s %*s %*s %*s %*d %*d-%*d-%*d %*d:%*d:%*d %ld", &start_fulltime);
    fclose(fp);
    return start_fulltime;
}

// end auction (create end file)
int EndAuction(int AID)
{
    // create the end file
    char endName[35];
    snprintf(endName, sizeof(endName), "AUCTIONS/%03d/End_%03d.txt", AID, AID);
    FILE *fp;
    fp = fopen(endName, "w");
    if (fp == NULL)
    {
        printf("Error opening file!\n");
        return 0;
    }
    // get the current time
    time_t fulltime;
    struct tm *currenttime;
    time(&fulltime);
    currenttime = gmtime(&fulltime);

    // get the time active of the auction
    int AuctionStartTime = GetAuctionStartFullTime(AID);
    int timeactive_seconds = fulltime - AuctionStartTime;

    // create time string
    char timeString[20];
    sprintf(timeString, "%4d-%02d-%02d %02d:%02d:%02d", currenttime->tm_year + 1900, currenttime->tm_mon + 1, currenttime->tm_mday, currenttime->tm_hour, currenttime->tm_min, currenttime->tm_sec);
    // write in the end file end datetime end fulltime
    fprintf(fp, "%s %d \n", timeString, timeactive_seconds);
    fclose(fp);

    return 1;
}

// get the time active of the auction from the start file
int GetAuctionTimeActive(int AID)
{
    // access the start file
    char startName[35];
    snprintf(startName, sizeof(startName), "AUCTIONS/%03d/Start_%03d.txt", AID, AID);
    // check if the file exists
    if (access(startName, F_OK) == -1)
    {
        printf("Start file does not exist\n");
        return 0;
    }
    // read the time active from the file
    FILE *fp;
    fp = fopen(startName, "r");
    if (fp == NULL)
    {
        printf("Error opening file!\n");
        return 0;
    }
    // read the time active
    int timeactive;
    fscanf(fp, "%*s %*s %*s %*s %d", &timeactive);
    fclose(fp);
    return timeactive;
}

// check if auction exists
int CheckAuctionExists(int AID)
{
    // access the start file
    char startName[35];
    snprintf(startName, sizeof(startName), "AUCTIONS/%03d/Start_%03d.txt", AID, AID);
    // check if the file exists
    if (access(startName, F_OK) == -1)
    {
        printf("Start file does not exist\n");
        return 0;
    }
    return 1;
}

// check if the auction should end or not (compare the time active + the fulltime with the current fulltime )
int CheckAuctionEnd(int AID)
{
    // get the time active of the auction
    int timeactive = GetAuctionTimeActive(AID);
    // get the start fulltime of the auction
    int AuctionStartTime = GetAuctionStartFullTime(AID);
    // get the current time
    time_t fulltime;
    struct tm *currenttime;
    time(&fulltime);
    currenttime = gmtime(&fulltime);
    // get the current fulltime
    int current_fulltime = fulltime;
    // check if the auction should end or not
    if (current_fulltime >= AuctionStartTime + timeactive)
    {
        int i = EndAuction(AID);
        if (i == 0)
        {
            printf("Error ending auction\n");
            return 0;
        }
        return 1;
    }
    else
        return 0;
}

// Update if all auctions should end or not
// check if the auction is still active (check if the end file exists)
int CheckAuctionActive(int AID)
{
    // access the end file
    char endName[35];
    snprintf(endName, sizeof(endName), "AUCTIONS/%03d/End_%03d.txt", AID, AID);
    // check if the file exists
    if (access(endName, F_OK) == -1)
    {
        return 1;
    }
    else
    {
        printf("Auction %d is not active\n", AID);
        return 0;
    }
}

int UpdateAllAuctions()
{
    // get all entries dirs from directory AUCTIONS
    struct dirent **filelist;
    int entries, len;
    char dirname[MAX_BUFFER_SIZE];
    char pathname[MAX_BUFFER_SIZE];
    // Create the directory path for the bids within the auction directory
    sprintf(dirname, "AUCTIONS/");
    // Scan the directory and get the list of files
    entries = scandir(dirname, &filelist, 0, alphasort);
    // Check if there are no entries in the directory
    if (entries <= 0)
        return 0;
    // Iterate through the directory entries
    while (entries--)
    {
        len = strlen(filelist[entries]->d_name);
        // Discard entries with leng
        if (len >= 3)
        {
            // Create the full path to the bid file
            sprintf(pathname, "AUCTIONS/%s", filelist[entries]->d_name);
            // Update the auction
            int AID = atoi(filelist[entries]->d_name);
            // check if auction is active
            if (CheckAuctionActive(AID))
            {
                // check if auction should end
                if (CheckAuctionEnd(AID))
                {
                    printf("Auction %d has been ended\n", AID);
                }
            }
        }
        // Free memory allocated for the dirent structure
        free(filelist[entries]);
    }
}

// check if user owns the auction
int CheckUserOwnsAuction(int AID, char *UID)
{
    // access the host file
    char hostName[35];
    snprintf(hostName, sizeof(hostName), "USERS/%s/HOSTED/%03d.txt", UID, AID);
    // check if the file exists
    if (access(hostName, F_OK) == -1)
    {
        return 0;
    }
    return 1;
}

int LoadBidAuction(const char *filename, BIDLIST *list)
{
    if (list->no_bids == 50)
    {
        printf("Bid list is full\n");
        return 0;
    }
    sscanf(filename, "AUCTIONS/%d/BIDS/%d", &list->bids[list->no_bids].auction_bid_id, &list->bids[list->no_bids].bid_ammount);
    // read the filename and get UID bid_amount bid_datetime bid_sec_time
    FILE *fp;
    fp = fopen(filename, "r");
    if (fp == NULL)
    {
        printf("Error opening file!\n");
        return 0;
    }
    // read the UID bid_amount bid_datetime bid_sec_time
    int year, month, day, hour, min, sec;
    fscanf(fp, "%d %d %4d-%02d-%02d %02d:%02d:%02d %ld", &list->bids[list->no_bids].user_bid_id, &list->bids[list->no_bids].bid_ammount, &year, &month, &day, &hour, &min, &sec, &list->bids[list->no_bids].bid_sec_time);
    fclose(fp);
    // create time string
    char timeString[20];
    sprintf(timeString, "%4d-%02d-%02d %02d:%02d:%02d", year, month, day, hour, min, sec);
    strcpy(list->bids[list->no_bids].bid_datetime, timeString);
    list->no_bids++;
    return 1;
}
// get a list of the 50 last bids from auction
int GetBidListAuction(int AID, BIDLIST *list)
{
    struct dirent **filelist;
    int entries, nbids, len;
    char dirname[MAX_BUFFER_SIZE];
    char pathname[MAX_BUFFER_SIZE];

    // Create the directory path for the bids within the auction directory
    sprintf(dirname, "AUCTIONS/%03d/BIDS/", AID);

    // Scan the directory and get the list of files
    entries = scandir(dirname, &filelist, 0, alphasort);

    // Check if there are no entries in the directory
    if (entries <= 0)
        return 0;

    nbids = 0;
    list->no_bids = 0;

    // Iterate through the directory entries
    while (entries--)
    {
        len = strlen(filelist[entries]->d_name);

        // Discard entries with length 10 (presumably, skip '.' and '..')
        if (len == 10)
        {
            // Create the full path to the bid file
            sprintf(pathname, "AUCTIONS/%03d/BIDS/%s", AID, filelist[entries]->d_name);

            // Load the bid information from the file and increment the bid count
            if (LoadBidAuction(pathname, list))
                ++nbids;
        }

        // Free memory allocated for the dirent structure
        free(filelist[entries]);

        // Break if the bid count reaches 50
        if (nbids == 50)
            break;
    }

    // Free memory allocated for the file list
    free(filelist);

    // Return the number of bids loaded
    return nbids;
}

// get last bid from auction
int GetLastBid(int AID)
{
    BIDLIST list;
    int n = GetBidListAuction(AID, &list);
    // if there is 0 bids in the auction return the start value in the start file
    if (n == 0)
    {
        printf("0 bids in this auction\n");
        // get the start value of the auction
        char startName[35];
        snprintf(startName, sizeof(startName), "AUCTIONS/%03d/Start_%03d.txt", AID, AID);
        // check if the file exists
        if (access(startName, F_OK) == -1)
        {
            printf("Start file does not exist\n");
            return 0;
        }
        // read the start value from the file
        FILE *fp;
        fp = fopen(startName, "r");
        if (fp == NULL)
        {
            printf("Error opening file!\n");
            return 0;
        }
        // read the start value
        int startValue;
        fscanf(fp, "%*s %*s %*s %d", &startValue);
        fclose(fp);
        return startValue;
    }
    // return the last bid from the auction
    int lastBid = list.bids[0].bid_ammount;
    return lastBid;
}

// make a bid
int MakeBid(int AID, char *uid, int bid_ammount)
{
    char AID_dirname[MAX_BUFFER_SIZE];
    char BIDS_dirname[MAX_BUFFER_SIZE];
    char UID_dirname[MAX_BUFFER_SIZE];
    char BID_filename[MAX_BUFFER_SIZE];
    int ret;
    // cast uid to int
    int UID = atoi(uid);

    // Check if AID is within a valid range (1 to 999)
    if (AID < 1 || AID > 999)
    {
        printf("Auction ID is invalid\n");
        return 0;
    }

    // Create the directory path for the auction
    sprintf(AID_dirname, "AUCTIONS/%03d", AID);

    // Check if the auction directory exists
    if (access(AID_dirname, F_OK) == -1)
    {
        printf("Auction directory does not exist\n");
        return 0;
    }

    int lastBid = GetLastBid(AID);

    if (bid_ammount <= lastBid)
    {
        printf("Bid ammount is invalid\n");
        return 0;
    }
    // Create the directory path for the bids within the auction directory
    sprintf(BIDS_dirname, "AUCTIONS/%03d/BIDS", AID);

    // Check if the bids directory exists
    if (access(BIDS_dirname, F_OK) == -1)
    {
        printf("Bids directory does not exist\n");
        return 0;
    }

    // Check if UID is within a valid range (1 to 999999)
    if (UID < 1 || UID > 999999)
    {
        printf("User ID is invalid\n");
        return 0;
    }

    sprintf(UID_dirname, "USERS/%06d", UID);

    // Check if the user directory exists
    if (access(UID_dirname, F_OK) == -1)
    {
        printf("User directory does not exist\n");
        return 0;
    }
    // Create the full path to the bid file
    sprintf(BID_filename, "AUCTIONS/%03d/BIDS/%06d.txt", AID, bid_ammount);

    // write the bid ammount, UID , bid_datetime and bid_sec_time in the file
    FILE *fp;
    fp = fopen(BID_filename, "w");
    if (fp == NULL)
    {
        printf("Error opening file!\n");
        return 0;
    }
    // get the current time
    time_t fulltime;
    struct tm *currenttime;
    time(&fulltime);
    currenttime = gmtime(&fulltime);

    // create time string
    char timeString[20];
    sprintf(timeString, "%4d-%02d-%02d %02d:%02d:%02d", currenttime->tm_year + 1900, currenttime->tm_mon + 1, currenttime->tm_mday, currenttime->tm_hour, currenttime->tm_min, currenttime->tm_sec);
    // write in the bid file UID bid_value bid_datetime bid_sec_time
    long startTime = GetAuctionStartFullTime(AID); // time in seconds since 1970 to the start of the auction
    long bid_sec_time = fulltime - startTime;      // time in seconds since 1970 to the bid minus the start time = time in seconds since the start of the auction
    fprintf(fp, "%s %d %s %ld \n", uid, bid_ammount, timeString, bid_sec_time);
    fclose(fp);
    // add the bid to the user directory
    sprintf(BID_filename, "USERS/%06d/BIDDED/%03d.txt", UID, AID);
    ret = creat(BID_filename, 0700);
    if (ret == -1)
    {
        printf("Bid file creation on the user Dir side was unsuccessful\n");
        return 0;
    }
    // Return 1 to indicate successful creation of the bid file
    return 1;
}

// load the info from the bid file in the user directory
int LoadBidUser(const char *filename, BIDLIST *list)
{
    if (list->no_bids == 999)
    {
        printf("Bid list is full\n");
        return 0;
    }
    sscanf(filename, "USERS/%*d/BIDDED/%d.txt", &list->bids[list->no_bids].auction_bid_id);
    int lastBid;
    lastBid = GetLastBid(list->bids[list->no_bids].auction_bid_id);
    list->bids[list->no_bids].bid_ammount = lastBid;
    int i = CheckAuctionActive(list->bids[list->no_bids].auction_bid_id);
    if (i == 1)
        list->bids[list->no_bids].active = true;
    else
        list->bids[list->no_bids].active = false;
    list->no_bids++;
    return 1;
}

// get a list of the 50 last bids from user
int GetBidListUser(char *UID, BIDLIST *list)
{
    struct dirent **filelist;
    int entries, nbids, len;
    char dirname[MAX_BUFFER_SIZE];
    char pathname[MAX_BUFFER_SIZE];

    // Create the directory path for the bids within the auction directory
    sprintf(dirname, "USERS/%s/BIDDED/", UID);

    // Scan the directory and get the list of files
    entries = scandir(dirname, &filelist, 0, alphasort);
    printf("entries: %d\n", entries);

    // Check if there are no entries in the directory
    if (entries <= 0)
        return 0;

    nbids = 0;
    list->no_bids = 0;
    // Iterate through the directory entries
    while (entries--)
    {
        len = strlen(filelist[entries]->d_name);

        // Discard entries with length 10 (presumably, skip '.' and '..')
        if (len >= 3)
        {
            // Create the full path to the bid file
            sprintf(pathname, "USERS/%s/BIDDED/%s", UID, filelist[entries]->d_name);
            printf("pathname: %s\n", pathname);
            // Load the bid information from the file and increment the bid count
            if (LoadBidUser(pathname, list))
                ++nbids;
        }

        // Free memory allocated for the dirent structure
        free(filelist[entries]);

        // Break if the bid count reaches 50
        if (nbids == 50)
            break;
    }

    // Free memory allocated for the file list
    free(filelist);

    // Return the number of bids loaded
    return nbids;
}

// load the info from the auction file in the user Hosted directory
int LoadAuction(const char *filename, AUCTIONLIST *list)
{
    if (list->no_auctions == 999)
    {
        printf("Auction list is full\n");
        return 0;
    }
    sscanf(filename, "USERS/%*d/HOSTED/%d.txt", &list->AID[list->no_auctions].AID);
    if (CheckAuctionActive(list->AID[list->no_auctions].AID))
        list->AID[list->no_auctions].active = true;
    else
        list->AID[list->no_auctions].active = false;
    list->no_auctions++;

    return 1;
}

// load the info from the auction directory
int LoadAuctionDir(const char *filename, AUCTIONLIST *list)
{
    if (list->no_auctions == 999)
    {
        printf("Auction list is full\n");
        return 0;
    }
    sscanf(filename, "AUCTIONS/%d", &list->AID[list->no_auctions].AID);
    if (CheckAuctionActive(list->AID[list->no_auctions].AID))
        list->AID[list->no_auctions].active = true;
    else
        list->AID[list->no_auctions].active = false;
    list->no_auctions++;

    return 1;
}

// get list of the 50 last auctions hosted by user
int ListHostedAuctions(char *UID, AUCTIONLIST *list)
{
    struct dirent **filelist;
    int entries, nauctions, len;
    char dirname[20];
    char pathname[32];

    // Create the directory path for the bids within the auction directory
    sprintf(dirname, "USERS/%s/HOSTED/", UID);

    // Scan the directory and get the list of files
    entries = scandir(dirname, &filelist, 0, alphasort);

    // Check if there are no entries in the directory
    if (entries <= 0)
        return 0;

    nauctions = 0;
    list->no_auctions = 0;

    // print entrys
    printf("entries: %d\n", entries);

    // Iterate through the directory entries
    while (entries--)
    {
        len = strlen(filelist[entries]->d_name);

        // Discard entries with leng
        if (len >= 3)
        {
            // Create the full path to the bid file
            sprintf(pathname, "USERS/%s/HOSTED/%s", UID, filelist[entries]->d_name);
            // Load the bid information from the file and increment the bid count
            if (LoadAuction(pathname, list))
                ++nauctions;
        }

        // Free memory allocated for the dirent structure
        free(filelist[entries]);

        // Break if the bid count reaches 50
        if (nauctions == 999)
            break;
    }

    // Free memory allocated for the file list
    free(filelist);

    // Return the number of bids loaded
    return nauctions;
}

// list all auctions from the auction directory
int ListAllAuctions(AUCTIONLIST *list)
{
    struct dirent **filelist;
    int entries, nauctions, len;
    char dirname[20];
    char pathname[32];

    // Create the directory path for the bids within the auction directory
    sprintf(dirname, "AUCTIONS/");

    // Scan the directory and get the list of files
    entries = scandir(dirname, &filelist, 0, alphasort);

    // Check if there are no entries in the directory
    if (entries <= 0)
        return 0;

    nauctions = 0;
    list->no_auctions = 0;

    // Iterate through the directory entries
    while (entries--)
    {
        len = strlen(filelist[entries]->d_name);

        // Discard entries with leng
        if (len >= 3)
        {
            // Create the full path to the bid file
            sprintf(pathname, "AUCTIONS/%s", filelist[entries]->d_name);

            // Load the bid information from the file and increment the bid count
            if (LoadAuctionDir(pathname, list))
                ++nauctions;
        }

        // Free memory allocated for the dirent structure
        free(filelist[entries]);

        // Break if the bid count reaches 50
        if (nauctions == 999)
            break;
    }

    // Free memory allocated for the file list
    free(filelist);

    // Return the number of bids loaded
    return nauctions;
}

//==================================================================================================
//==================================================================================================
// write udp message to socket
int sendUDPMessage(int sockfd, char *message)
{
    int n = sendto(sockfd, message, strlen(message), 0, (struct sockaddr *)&addr, addrlen);
    if (n == -1)
    {
        printf("Error writing to socket\n");
        exit(EXIT_FAILURE);
    }
    return 1;
}

int handleLoginRequest(int sockfd, char *UID, char *Password)
{

    char response[MAX_BUFFER_SIZE];

    // check if the user is registered
    if (!CheckUserRegistered(UID))
    {
        createUserDir(UID);
        CreatePassword(UID, Password);
        CreateLogin(UID);
        sprintf(response, "%s %s\n", LOGIN_RESPONSE, STATUS_REG);
        sendUDPMessage(sockfd, response);
        return 1;
    }
    // check password
    if (!CheckPassword(UID, Password))
    {
        printf("Password is incorrect\n");
        sprintf(response, "%s %s\n", LOGIN_RESPONSE, STATUS_NOK);
        // send udp socket response
        sendUDPMessage(sockfd, response);
        return 0;
    }
    // check if the user is already logged in
    if (CheckLogin(UID))
    {
        printf("User is already logged in\n");
        sprintf(response, "%s %s\n", LOGIN_RESPONSE, STATUS_NOK);
        sendUDPMessage(sockfd, response);
        return 0;
    }
    // create login file
    if (!CreateLogin(UID))
    {
        printf("Error creating login file\n");
        return 0;
    }
    // send response
    sprintf(response, "%s %s\n", LOGIN_RESPONSE, STATUS_OK);
    sendUDPMessage(sockfd, response);
    return 1;
}

int handleLogoutRequest(int sockfd, char *UID, char *Password)
{
    char response[MAX_BUFFER_SIZE];

    // check if the user is registered
    if (!CheckUserRegistered(UID))
    {
        printf("User is not registered\n");
        sprintf(response, "%s %s\n", LOGOUT_RESPONSE, STATUS_UNR);
        sendUDPMessage(sockfd, response);
        return 0;
    }
    // check if the user is logged in
    if (!CheckLogin(UID))
    {
        printf("User is not logged in\n");
        sprintf(response, "%s %s\n", LOGOUT_RESPONSE, STATUS_NOK);
        sendUDPMessage(sockfd, response);
        return 0;
    }
    // check password
    if (!CheckPassword(UID, Password))
    {
        printf("Password is incorrect\n");
        sprintf(response, "%s %s\n", LOGOUT_RESPONSE, STATUS_NOK);
        sendUDPMessage(sockfd, response);
        return 0;
    }
    // erase login file
    if (!EraseLogin(UID))
    {
        printf("Error erasing login file\n");
        return 0;
    }
    // send response
    sprintf(response, "%s %s\n", LOGOUT_RESPONSE, STATUS_OK);
    sendUDPMessage(sockfd, response);
    return 1;
}

int handleUnregisterRequest(int sockfd, char *UID, char *Password)
{
    char response[MAX_BUFFER_SIZE];

    // check if the user is registered
    if (!CheckUserRegistered(UID))
    {
        printf("User is not registered\n");
        sprintf(response, "%s %s\n", UNREGISTER_RESPONSE, STATUS_UNR);
        sendUDPMessage(sockfd, response);
        return 0;
    }
    // check if the user is logged in
    if (!CheckLogin(UID))
    {
        printf("User is not logged in\n");
        sprintf(response, "%s %s\n", UNREGISTER_RESPONSE, STATUS_NOK);
        sendUDPMessage(sockfd, response);
        return 0;
    }
    // check password
    if (!CheckPassword(UID, Password))
    {
        printf("Password is incorrect\n");
        sprintf(response, "%s %s\n", UNREGISTER_RESPONSE, STATUS_NOK);
        sendUDPMessage(sockfd, response);
        return 0;
    }
    // erase login file
    if (!EraseLogin(UID))
    {
        printf("Error erasing login file\n");
        return 0;
    }
    // erase password file
    if (!erasePassword(UID))
    {
        printf("Error erasing password file\n");
        return 0;
    }
    // send response
    sprintf(response, "%s %s\n", UNREGISTER_RESPONSE, STATUS_OK);
    sendUDPMessage(sockfd, response);
    return 1;
}

int handleListMyAucRequest(int sockfd, char *UID)
{
    char response[MAX_BUFFER_SIZE * 2];

    // check if user is logged in
    if (!CheckLogin(UID))
    {
        printf("User is not logged in\n");
        sprintf(response, "%s %s\n", LIST_MY_AUC_RESPONSE, STATUS_NLG);
        sendUDPMessage(sockfd, response);
        return 0;
    }

    AUCTIONLIST list;
    int n = ListHostedAuctions(UID, &list);
    printf("n: %d\n", n);
    // send response
    if (n == 0)
    {
        printf("0 auctions hosted by this user\n");
        sprintf(response, "%s %s\n", LIST_MY_AUC_RESPONSE, STATUS_NOK);

        sendUDPMessage(sockfd, response);
        return 0;
    }
    // put the the auctins in a string
    char auctions[MAX_BUFFER_SIZE];
    memset(auctions, 0, sizeof(auctions));

    for (int i = 0; i < n; i++)
    {
        char AID[4];
        sprintf(AID, "%03d", list.AID[i].AID);
        strcat(auctions, AID);
        if (list.AID[i].active)
            strcat(auctions, " 1 ");
        else
            strcat(auctions, " 0 ");
    }

    sprintf(response, "%s %s %s\n", LIST_MY_AUC_RESPONSE, STATUS_OK, auctions);
    printf("response: %s\n", response);
    sendUDPMessage(sockfd, response);
    return 1;
}

int handleListMyBidRequest(int sockfd, char *UID)
{
    char response[MAX_BUFFER_SIZE];

    // check if user is logged in
    if (!CheckLogin(UID))
    {
        printf("User is not logged in\n");
        sprintf(response, "%s %s\n", LIST_MY_BID_RESPONSE, STATUS_NLG);
        sendUDPMessage(sockfd, response);
        return 0;
    }

    BIDLIST list;
    int n = GetBidListUser(UID, &list);
    printf("n: %d\n", n);
    // send response
    if (n == 0)
    {
        printf("0 bids made by this user\n");
        sprintf(response, "%s %s\n", LIST_MY_BID_RESPONSE, STATUS_NOK);

        sendUDPMessage(sockfd, response);
        return 0;
    }
    // put the the auctins in a string
    char bids[1000];
    memset(bids, 0, sizeof(bids));

    for (int i = 0; i < n; i++)
    {
        char AID[4];
        sprintf(AID, "%03d ", list.bids[i].auction_bid_id);
        strcat(bids, AID);
        if (list.bids[i].active)
            strcat(bids, "1 ");
        else
            strcat(bids, "0 ");
    }
    sprintf(response, "%s %s %s\n", LIST_MY_BID_RESPONSE, STATUS_OK, bids);
    sendUDPMessage(sockfd, response);
    return 1;
}

int handleListAllAuctionsRequest(int sockfd)
{
    char response[MAX_BUFFER_SIZE];

    AUCTIONLIST list;
    int n = ListAllAuctions(&list);
    printf("n: %d\n", n);
    // send response
    if (n == 0)
    {
        printf("0 auctions hosted by this user\n");
        sprintf(response, "%s %s\n", LIST_AUC_RESPONSE, STATUS_NOK);

        sendUDPMessage(sockfd, response);
        return 0;
    }
    // put the the auctins in a string
    char auctions[MAX_BUFFER_SIZE];
    memset(auctions, 0, sizeof(auctions));

    for (int i = 0; i < n; i++)
    {
        char AID[4];
        sprintf(AID, "%03d", list.AID[i].AID);
        strcat(auctions, AID);
        if (list.AID[i].active)
            strcat(auctions, " 1 ");
        else
            strcat(auctions, " 0 ");
    }
    sprintf(response, "%s %s %s\n", LIST_AUC_RESPONSE, STATUS_OK, auctions);
    sendUDPMessage(sockfd, response);
    return 1;
}

int handleShowRecRequest(int sockfd, char *AID)
{
    char response[MAX_BUFFER_SIZE];
    // check if auction exists
    if (!CheckAuctionExists(atoi(AID)))
    {
        printf("Auction does not exist\n");
        sprintf(response, "%s %s\n", SHOW_REC_RESPONSE, STATUS_NOK);
        sendUDPMessage(sockfd, response);
        return 0;
    }
    // read the start file
    char startName[35];
    snprintf(startName, sizeof(startName), "AUCTIONS/%03d/Start_%03d.txt", atoi(AID), atoi(AID));

    // read the start value from the file
    FILE *fp;
    fp = fopen(startName, "r");
    if (fp == NULL)
    {
        printf("Error opening file!\n");
        sprintf(response, "%s %s\n", SHOW_REC_RESPONSE, STATUS_NOK);
        sendUDPMessage(sockfd, response);
        return 0;
    }
    // read all things from the start value
    char UID[7];
    char Name[128];
    char Fname[128];
    int startValue;
    // datetime
    int year;
    int month;
    int day;
    int hour;
    int min;
    int sec;
    int timeactive;
    fscanf(fp, "%s %s %s %d %d %d-%d-%d %d:%d:%d %*s", UID, Name, Fname, &startValue, &timeactive, &year, &month, &day, &hour, &min, &sec);
    fclose(fp);
    // limit the timeactive to the 5 digits
    if (timeactive > 99999)
    {
        timeactive = 99999;
    }

    // get the last 50 bids
    BIDLIST list;
    int n = GetBidListAuction(atoi(AID), &list);
    char bids[MAX_BUFFER_SIZE];

    for (int i = 0; i < n; i++)
    {
        char bid[256];
        // limit the bid sec time to 5 digits
        if (list.bids[i].bid_sec_time > 99999)
        {
            list.bids[i].bid_sec_time = 99999;
        }
        sprintf(bid, " B %06d %d %s %ld", list.bids[i].user_bid_id, list.bids[i].bid_ammount, list.bids[i].bid_datetime, list.bids[i].bid_sec_time);
        strcat(bids, bid);
    }
    // make response

    sprintf(response, "%s %s %s %s %s %d %d %d-%d-%d %d:%d:%d %d%s", SHOW_REC_RESPONSE, STATUS_OK, UID, Name, Fname, startValue, timeactive, year, month, day, hour, min, sec, n, bids);
    // check if the auction is active
    int i = CheckAuctionActive(atoi(AID));
    if (i == 0)
    {
        // read from end file
        char endName[35];
        snprintf(endName, sizeof(endName), "AUCTIONS/%s/End_%s.txt", AID, AID);
        // time string
        char timeString[20];
        // read the end value from the file
        FILE *fp;
        fp = fopen(endName, "r");
        if (fp == NULL)
        {
            printf("Error opening file!\n");
            return 0;
        }
        // read all things from the end value
        int year, month, day, hour, min, sec;
        long end_sec_time;
        fscanf(fp, "%4d-%02d-%02d %02d:%02d:%02d %ld", &year, &month, &day, &hour, &min, &sec, &end_sec_time);
        fclose(fp);
        // create time string
        sprintf(timeString, "%4d-%02d-%02d %02d:%02d:%02d", year, month, day, hour, min, sec);
        // add the end time to the response
        char end[256];
        sprintf(end, " E %s %ld", timeString, end_sec_time);
        strcat(response, end);
    }
    strcat(response, "\n");
    // send response
    sendUDPMessage(sockfd, response);
    return 1;
}

// handle udp message
int handleUDPmessage(int sockfd, char *message)
{
    // read socket
    char command[4];
    sscanf(message, "%s ", command);
    printf("message: %s\n", command);
    // check if command is valid

    // handle the command
    if (!strcmp(command, LOGIN_REQUEST))
    {
        char UID[7];
        char Password[8];
        sscanf(message, "%*s %s %s", UID, Password);
        printf("UID: %s\n", UID);
        printf("Password: %s\n", Password);
        handleLoginRequest(sockfd, UID, Password);
    }
    else if (!strcmp(command, LOGOUT_REQUEST))
    {
        char UID[7];
        char Password[8];
        sscanf(message, "%*s %s %s", UID, Password);
        printf("UID: %s\n", UID);
        printf("Password: %s\n", Password);
        handleLogoutRequest(sockfd, UID, Password);
    }
    else if (!strcmp(command, UNREGISTER_REQUEST))
    {
        char UID[7];
        char Password[8];
        sscanf(message, "%*s %s %s", UID, Password);
        printf("UID: %s\n", UID);
        printf("Password: %s\n", Password);
        handleUnregisterRequest(sockfd, UID, Password);
    }
    else if (!strcmp(command, LIST_MY_AUC_REQUEST))
    {
        char UID[7];
        sscanf(message, "%*s %s", UID);
        printf("UID: %s\n", UID);
        handleListMyAucRequest(sockfd, UID);
    }
    else if (!strcmp(command, LIST_MY_BID_REQUEST))
    {
        char UID[7];
        sscanf(message, "%*s %s", UID);
        printf("UID: %s\n", UID);
        handleListMyBidRequest(sockfd, UID);
    }
    else if (!strcmp(command, LIST_AUC_REQUEST))
    {
        handleListAllAuctionsRequest(sockfd);
    }
    else if (!strcmp(command, SHOW_REC_REQUEST))
    {
        char AID[4];
        sscanf(message, "%*s %s", AID);
        printf("AID: %s\n", AID);
        handleShowRecRequest(sockfd, AID);
    }
}

// Tread to handle udp messages
void *udpThread(void *arg)
{
    int fd, errcode;
    ssize_t n;
    struct addrinfo hints, *res;
    char buffer[128];

    // Create UDP socket
    int udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket == -1)
    {
        perror("Error creating UDP socket");
        exit(EXIT_FAILURE);
    }

    fd = socket(AF_INET, SOCK_DGRAM, 0); // UDP socket
    if (fd == -1)                        /*error*/
        exit(1);
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;      // IPv4
    hints.ai_socktype = SOCK_DGRAM; // UDP socket
    hints.ai_flags = AI_PASSIVE;
    errcode = getaddrinfo(NULL, AS_PORT, &hints, &res);
    if (errcode != 0) /*error*/
        exit(1);
    n = bind(fd, res->ai_addr, res->ai_addrlen);
    if (n == -1) /*error*/
        exit(1);

    // Handle UDP messages
    while (1)
    {
        addrlen = sizeof(addr);
        n = recvfrom(fd, buffer, 128, 0,
                     (struct sockaddr *)&addr, &addrlen);
        if (n == -1)
        { /*error*/
            close(udp_socket);
            exit(1);
        }

        printf("FORK!\n");
        handleUDPmessage(udp_socket, buffer);
    }

    // Close UDP socket
    close(udp_socket);

    return NULL;
}

//==================================================================================================
//==================================================================================================
// Recieve the file from the socket
int recieveFile(int sockfd, char *FilePath, int Fsize)
{
    int n;
    char buffer[MAX_BUFFER_SIZE];
    FILE *fp;
    int bytesReceived = 0;
    fp = fopen(FilePath, "w");
    if (fp == NULL)
    {
        printf("Error opening file!\n");
        return 0;
    }
    while (bytesReceived < Fsize)
    {
        memset(buffer, 0, sizeof(buffer));
        n = read(sockfd, buffer, sizeof(buffer));
        if (n == -1)
        {
            printf("Error reading from socket\n");
            exit(EXIT_FAILURE);
        }
        else if (n)
        {
            fwrite(buffer, 1, n, fp);
            bytesReceived += n;
        }
    }
    fclose(fp);
    return 1;
}

// send a message through the tcp socket
int sendTCPMessage(int sockfd, char *message)
{
    int n;
    n = write(sockfd, message, strlen(message));
    if (n == -1)
    {
        printf("Error writing to socket\n");
        exit(EXIT_FAILURE);
    }
    return 1;
}

// Handle open auction request
int handleOpenAuction(int sockfd)
{

    // variables
    char buffer[MAX_BUFFER_SIZE];
    char response[MAX_BUFFER_SIZE];
    int nSpaces = 0;
    char UID[7];
    char Password[9];
    char Name[128];
    int startValue;
    int timeactive; // duration of auction in seconds
    char FileName[24];
    int Fsize;
    char dirpath[MAX_BUFFER_SIZE];
    memset(dirpath, 0, sizeof(dirpath));
    memset(buffer, 0, sizeof(buffer));

    // Receive the message
    int i = 0;
    while (nSpaces < 7) // number of spaces until the Fdata ( UID password name start_value timeactive Fname Fsize Fdata)
    {
        int k = 0;
        if ((k = read(sockfd, buffer + i, 1)) == -1)
        {
            printf("Error reading from socket\n");
            exit(EXIT_FAILURE);
        }
        else if (k)
        {
            if (buffer[i] == ' ')
            {
                nSpaces++;
            }
            i++;
        }
    }
    // align the message  to start reading the file
    sscanf(buffer, "%s %s %s %d %d %s %d ", UID, Password, Name, &startValue, &timeactive, FileName, &Fsize);

    printf(" info -> %s %s %s %d %d %s %d \n", UID, Password, Name, startValue, timeactive, FileName, Fsize);

    // check if the user is logged in
    if (!CheckLogin(UID))
    {
        printf("User is not logged in\n");
        sprintf(response, "%s %s", OPEN_AUCTION_RESPONSE, STATUS_NLG);
        sendTCPMessage(sockfd, response);
        return 0;
    }
    // check password
    if (!CheckPassword(UID, Password))
    {
        printf("Wrong password\n");
        sprintf(response, "%s %s", OPEN_AUCTION_RESPONSE, STATUS_NOK);
        sendTCPMessage(sockfd, response);
        return 0;
    }

    // TODO other checks

    // create the sequencial auction directory
    int AID = CreateAUCTIONDir();

    //  create the start file
    i = StartAuction(AID, UID, Name, FileName, startValue, timeactive);
    if (i == 0)
    {
        printf("Error starting auction\n");
        return 0;
    }

    // get the location to place the asset file
    sprintf(dirpath, "AUCTIONS/%03d/%s", AID, FileName);

    // start reading an creating the asset file
    i = recieveFile(sockfd, dirpath, Fsize);
    if (i == 0)
    {
        printf("Error creating file\n");
        return 0;
    }
    // create host file
    i = CreateHostFile(AID, UID);
    if (i == 0)
    {
        printf("Error creating host file\n");
        return 0;
    }
    // create the "OK" response
    sprintf(response, "%s %s %03d", OPEN_AUCTION_RESPONSE, STATUS_OK, AID);
    sendTCPMessage(sockfd, response);

    return 1;
}

// Handle close auction request
int handleCloseAuction(int sockfd)
{
    char buffer[MAX_BUFFER_SIZE];
    char response[MAX_BUFFER_SIZE];
    char UID[7];
    char AID[4];
    char password[9];
    memset(buffer, 0, sizeof(buffer));
    int bytes_read = read(sockfd, buffer, MAX_BUFFER_SIZE);
    sscanf(buffer, " %s %s %s\n", UID, password, AID);

    // TODO CHECKS

    // check if the user is logged in
    if (!CheckLogin(UID))
    {
        printf("User is not logged in\n");
        sprintf(response, "%s %s", CLOSE_AUCTION_RESPONSE, STATUS_NLG);
        sendTCPMessage(sockfd, response);
        return 0;
    }
    // check if user owns the auction
    if (!CheckUserOwnsAuction(atoi(AID), UID))
    {
        printf("User does not own the auction\n");
        sprintf(response, "%s %s", CLOSE_AUCTION_RESPONSE, STATUS_EOW);
        sendTCPMessage(sockfd, response);
        return 0;
    }

    // check if auction exists
    if (!CheckAuctionExists(atoi(AID)))
    {
        printf("Auction does not exist\n");
        sprintf(response, "%s %s", CLOSE_AUCTION_RESPONSE, STATUS_EAU);
        sendTCPMessage(sockfd, response);
        return 0;
    }
    // check if the auction is active
    if (!CheckAuctionActive(atoi(AID)))
    {
        printf("Auction is not active\n");
        sprintf(response, "%s %s", CLOSE_AUCTION_RESPONSE, STATUS_END);
        sendTCPMessage(sockfd, response);
        return 0;
    }

    int i = EndAuction(atoi(AID));
    if (i == 0)
    {
        printf("Error ending auction\n");
        return 0;
    }
    sprintf(response, "%s %s", CLOSE_AUCTION_RESPONSE, STATUS_OK);
    sendTCPMessage(sockfd, response);
    return 1;
}

// handle list hosted auctions request
int handleBidRequest(int sockfd)
{
    char buffer[MAX_BUFFER_SIZE];
    char response[MAX_BUFFER_SIZE];
    char UID[MAX_BUFFER_SIZE];
    char AID[MAX_BUFFER_SIZE];
    int bid_ammount;
    char password[MAX_BUFFER_SIZE];
    memset(buffer, 0, sizeof(buffer));
    memset(response, 0, sizeof(response));
    memset(UID, 0, sizeof(UID));
    memset(AID, 0, sizeof(AID));
    memset(password, 0, sizeof(password));
    ssize_t bytes_read = read(sockfd, buffer, MAX_BUFFER_SIZE);
    if (bytes_read == -1)
    {
        printf("Error reading from socket\n");
        exit(EXIT_FAILURE);
    }

    // get the info from the buffer
    sscanf(buffer, "%s %s %s %d", UID, password, AID, &bid_ammount);

    // check if password is correct

    if (!CheckPassword(UID, password))
    {
        printf("Wrong password\n");
        sprintf(response, "%s %s\n", BID_RESPONSE, STATUS_NOK);
        sendTCPMessage(sockfd, response);
        return 0;
    }
    // check if user is logged in
    if (!CheckLogin(UID))
    {
        printf("User is not logged in\n");
        sprintf(response, "%s %s\n", BID_RESPONSE, STATUS_NLG);
        sendTCPMessage(sockfd, response);
        return 0;
    }
    // check if auction exists
    if (!CheckAuctionExists(atoi(AID)))
    {
        printf("Auction does not exist\n");
        sprintf(response, "%s %s\n", BID_RESPONSE, STATUS_NOK);
        sendTCPMessage(sockfd, response);
        return 0;
    }
    // check if the auction is active
    if (!CheckAuctionActive(atoi(AID)))
    {
        printf("Auction is not active\n");
        sprintf(response, "%s %s\n", BID_RESPONSE, STATUS_NOK);
        sendTCPMessage(sockfd, response);
        return 0;
    }
    // check the ownership of the auction
    if (CheckUserOwnsAuction(atoi(AID), UID))
    {
        printf("User owns the auction\n");
        sprintf(response, "%s %s\n", BID_RESPONSE, STATUS_ILG);
        sendTCPMessage(sockfd, response);
        return 0;
    }
    // check if the bid is valid
    int lastBid = GetLastBid(atoi(AID));
    printf("lastBid: %d\n", lastBid);
    if (bid_ammount <= lastBid)
    {
        printf("Bid ammount is invalid\n");
        sprintf(response, "%s %s\n", BID_RESPONSE, STATUS_REF);
        sendTCPMessage(sockfd, response);
        return 0;
    }
    // make the bid
    int i = MakeBid(atoi(AID), UID, bid_ammount);
    if (i == 0)
    {
        printf("Error making bid\n");
        sprintf(response, "%s %s\n", BID_RESPONSE, STATUS_ERR);
        sendTCPMessage(sockfd, response);
        return 0;
    }
    sprintf(response, "%s %s\n", BID_RESPONSE, STATUS_OK);
    sendTCPMessage(sockfd, response);
    return 1;
}

// Handle show asset request
int handleShowAsset(int sockfd)
{
    char buffer[MAX_BUFFER_SIZE];
    char response[MAX_BUFFER_SIZE];
    char AID[4];
    char fileName[24];
    char startName[35];

    memset(buffer, 0, sizeof(buffer));
    int bytes_read = read(sockfd, buffer, MAX_BUFFER_SIZE);
    sscanf(buffer, " %s\n", AID);

    printf("SHOW ASSET OF -> %s\n", AID);
    // check if auction exists
    if (!CheckAuctionExists(atoi(AID)))
    {
        printf("Auction does not exist\n");
        sprintf(response, "%s %s", SHOW_ASSET_RESPONSE, STATUS_EAU);
        sendTCPMessage(sockfd, response);
        return 0;
    }
    // get file name on the start file

    sprintf(startName, "AUCTIONS/%s/Start_%s.txt", AID, AID);
    FILE *fp;
    fp = fopen(startName, "r");
    if (fp == NULL)
    {
        sprintf(response, "%s %s", SHOW_ASSET_RESPONSE, STATUS_NOK);
        sendTCPMessage(sockfd, response);
        return 0;
    }

    memset(fileName, 0, sizeof(fileName));
    fscanf(fp, "%*d %*s %s ", fileName);

    fclose(fp);

    // get file location
    char dirpath[MAX_BUFFER_SIZE];
    memset(dirpath, 0, sizeof(dirpath));
    sprintf(dirpath, "AUCTIONS/%s/%s", AID, fileName);
    printf("dirpath: %s\n", dirpath);

    // get file size
    struct stat st;
    stat(dirpath, &st);
    size_t Fsize = st.st_size;

    // send response RSA status Fname Fsize Fdata
    sprintf(response, "%s %s %s %ld ", SHOW_ASSET_RESPONSE, STATUS_OK, fileName, Fsize);
    sendTCPMessage(sockfd, response);

    // get file descriptor of the file
    int fp2 = open(dirpath, O_RDONLY);
    if (fp2 == -1)
    {
        perror("Error opening file for reading");
        exit(EXIT_FAILURE);
    }
    // send file
    off_t offset = 0;
    struct stat file_stat;
    if (fstat(fp2, &file_stat) == -1)
    {
        perror("Error getting file status");
        exit(EXIT_FAILURE);
    }

    ssize_t n = sendfile(sockfd, fp2, &offset, file_stat.st_size);
    if (n == -1)
    {
        perror("Error sending file");
        exit(EXIT_FAILURE);
    }
    return 1;
}

// handle tcp commands
int handleTCPCommand(int sockfd, char *command)
{
    // check if command is valid
    if (strlen(command) - 1 != 3)
    {
        printf("Command is invalid\n");
        return 0;
    }
    if (command[3] != ' ')
    {
        printf("Command is invalid\n");
        return 0;
    }
    for (int i = 0; i < strlen(command) - 1; i++)
    {
        if (!isalpha(command[i]) || !isupper(command[i]))
        {
            printf("Command is invalid\n");
            return 0;
        }
    }
    // handle the command
    if (!strcmp(command, OPEN_AUCTION_REQUEST))
    {
        int i = handleOpenAuction(sockfd);
        if (i == 0)
        {
            printf("Error handling open auction request\n");
            return 0;
        }
    }
    else if (!strcmp(command, CLOSE_AUCTION_REQUEST))
    {
        int i = handleCloseAuction(sockfd);

        if (i == 0)
        {
            printf("Error handling open auction response\n");
            return 0;
        }
    }
    else if (!strcmp(command, SHOW_ASSET_REQUEST))
    {
        int i = handleShowAsset(sockfd);

        if (i == 0)
        {
            printf("Error handling show asset response\n");
            return 0;
        }
    }
    else if (!strcmp(command, BID_REQUEST))
    {
        int i = handleBidRequest(sockfd);
        if (i == 0)
        {
            printf("Error handling bid request\n");
            return 0;
        }
    }
    else
    {
        printf("Command is invalid\n");
        return 0;
    }
}

// handle tcp messages
void handleTCPMessage(int sockfd)
{

    char command[4];

    int i = read(sockfd, command, 4);
    if (i == -1)
    {
        printf("Error reading from socket\n");
        exit(EXIT_FAILURE);
    };

    // Handle the TCP message
    printf("TCP command received: %s\n", command);
    printf("========Update all auctions===========\n");
    UpdateAllAuctions();
    printf("==========finished update=============\n");
    handleTCPCommand(sockfd, command);
    return;
}

// signal handler
void sigchld_handler(int signo)
{
    // WNOHANG specifies non-blocking behavior
    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;
}

// Thread to handle TCP messages
void *tcpThread(void *arg)
{
    int fd, errcode, newSockfd;
    ssize_t n;
    socklen_t addrlen;
    struct addrinfo hints, *res;
    struct sockaddr_in addr;
    char buffer[128];
    // Create TCP socket
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("TCP socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;       // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP socket
    hints.ai_flags = AI_PASSIVE;
    errcode = getaddrinfo(NULL, AS_PORT, &hints, &res);
    if ((errcode) != 0) /*error*/
        exit(1);
    n = bind(fd, res->ai_addr, res->ai_addrlen);
    if (n == -1) /*error*/
        exit(1);

    // Listen for incoming connections
    if (listen(fd, 5) == -1)
    {
        perror("TCP socket listen failed");
        exit(EXIT_FAILURE);
    }

    printf("TCP server is listening on port %s\n", AS_PORT);

    // set up signal handler
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if (sigaction(SIGCHLD, &sa, NULL) == -1)
    {
        perror("Error setting up SIGCHLD handler");
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        // Accept a connection
        if ((newSockfd = accept(fd, (struct sockaddr *)&addr, &addrlen)) == -1)
        {
            perror("TCP socket accept failed");
            exit(EXIT_FAILURE);
        }
        else
        {

            // Handle TCP messages in a forked process
            if (fork() == 0)
            {
                close(fd); // Close the original socket in the child process
                handleTCPMessage(newSockfd);
                sleep(10);
                printf("conection ended\n");
                close(newSockfd);
                exit(EXIT_SUCCESS);
            }
            else
            {
                close(newSockfd); // Close the new socket in the parent process
            }
        }
    }

    close(fd);
    pthread_exit(NULL);
}
//==================================================================================================
//==================================================================================================

int main(int argc, char *argv[])
{
    strcpy(AS_PORT, AS_PORT_BASE);
    printf("AS_PORT_base: %s\n", AS_PORT);
    int i = 1;
    printf("argc: %d\n", argc);
    printf("argv: %s\n", argv[0]);
    printf("argv: %s\n", argv[1]);
    printf("argv: %s\n", argv[2]);
    printf("argv: %s\n", argv[3]);
    while (i < argc)
    {

        if (!strcmp(argv[i], "-v"))
        {
            verbose = true;
        }
        if (!strcmp(argv[i], "-p"))
        {
            strcpy(AS_PORT, argv[i + 1]);
        }
        i++;
    }

    pthread_t udpThreadID, tcpThreadID;

    // Create UDP thread
    if (pthread_create(&udpThreadID, NULL, udpThread, NULL) != 0)
    {
        perror("Error creating UDP thread");
        exit(EXIT_FAILURE);
    }

    // Create TCP thread
    if (pthread_create(&tcpThreadID, NULL, tcpThread, NULL) != 0)
    {
        perror("Error creating TCP thread");
        exit(EXIT_FAILURE);
    }

    // Wait for threads to finish
    pthread_join(udpThreadID, NULL);
    pthread_join(tcpThreadID, NULL);

    return 0;
}