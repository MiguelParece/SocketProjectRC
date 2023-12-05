// server side code
#include <stdio.h>
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

int main()
{
    printf("Hello, World!\n");
    CreateAUCTIONDir(322);
    return 0;
}