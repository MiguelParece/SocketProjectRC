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
#include <sys/sendfile.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/sendfile.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/sendfile.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/sendfile.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/sendfile.h>
#include <sys/mman.h>

// structs
typedef struct
{
    int auction_bid_id; // auction id
    int bid_ammount;    // bid ammount
} BID;

typedef struct
{
    int no_bids;  // Number of bids in the list
    BID bids[50]; // Assuming a maximum of 50 bids, adjust as needed
} BIDLIST;

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

// handle auctions
int CreateAUCTIONDir(int AID)
{
    char AID_dirname[15];
    char BIDS_dirname[20];
    int ret;

    // Check if AID is within a valid range (1 to 999)
    if (AID < 1 || AID > 999)
        return 0;

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
    return 1;
}

// create auction
int StartAuction(int *AID, char *UID, char *nameDescription, char *fileName, char *startValue, char *timeactive)
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
    // write in the start file UID name asset fname start value timeactive start datetime start fulltime
    fprintf(fp, "%s %s %s %s %s \n", UID, nameDescription, fileName, startValue, timeactive);
}

int MakeBid(int AID, int UID, int bid_ammount)
{
    char AID_dirname[15];
    char BIDS_dirname[20];
    char UID_dirname[15];
    char BID_filename[25];
    int ret;

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

    // Check if bid_ammount is valid
    BIDLIST list;
    int n = GetBidList(AID, &list);
    if (n == 0)
    {
        printf("0 bids in this auction\n");
        return 0;
    }
    int lastBid = list.bids[0].bid_ammount;

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

    // Attempt to create the bid file
    ret = creat(BID_filename, 0700);

    // Check if the bid file creation was unsuccessful
    if (ret == -1)
    {
        printf("Bid file creation on the server Dir side was unsuccessful\n");
        return 0;
    }

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

// Fazer um initializer ?
int GetBidList(int AID, BIDLIST *list)
{
    struct dirent **filelist;
    int entries, nbids, len;
    char dirname[20];
    char pathname[32];

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
            if (LoadBid(pathname, list))
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

int LoadBid(const char *filename, BIDLIST *list)
{
    if (list->no_bids == 50)
    {
        printf("Bid list is full\n");
        return 0;
    }
    sscanf(filename, "AUCTIONS/%d/BIDS/%d", &list->bids[list->no_bids].auction_bid_id, &list->bids[list->no_bids].bid_ammount);
    list->no_bids++;
    return 1;
}

int main()
{
    time_t fulltime;        // Declare a variable to store the current time in seconds.
    struct tm *currenttime; // Declare a pointer to a structure to store the broken-down time.

    char timestr[20]; // Declare an array to store the formatted time as a string.

    time(&fulltime); // Get the current time in seconds since 1970.

    currenttime = gmtime(&fulltime);
    printf("UTC date and time: %s", asctime(currenttime));

    printf("Hello, World!\n");
    BIDLIST list;
    list.no_bids = 0;
    createUserDir("000001");
    CreateLogin("000001");
    CreatePassword("000001", "123456");
    MakeBid(322, 000001, 8000);
    return 0;
}