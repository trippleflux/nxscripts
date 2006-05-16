/*

nxSDK - ioFTPD Software Development Kit
Copyright (c) 2006 neoxed

Module Name:
    Credits

Author:
    neoxed (neoxed@gmail.com) May 16, 2006

Abstract:
    Example application, demonstrates how to add or remove credits from a user.

*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <nxsdk.h>

#undef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

static void
Usage(
    const char *argv0
    );


int
main(
    int argc,
    char **argv
    )
{
    int result = 1;
    int section;
    int userId;
    IO_MEMORY *memory;
    IO_SESSION session;
    USERFILE userFile;

    // Check if section argument was specified.
    switch (argc) {
        case 4:
            section = 0;
            break;
        case 5:
            section = atoi(argv[4]);
            if (section < 0 || section > 9) {
                printf("Invalid section %d, must be from 0 to 9.\n", section);
                return 1;
            }
            break;
        default:
            Usage(argv[0]);
            return 1;
    }

    // Locate ioFTPD's message window.
    if (!Io_ShmInit(argv[1], &session)) {
        printf("The message window \"%s\" does not exist.\n", argv[1]);
        return 1;
    }

    // Allocate memory for user resolving and manipulating user files.
    memory = Io_ShmAlloc(&session, MAX(sizeof(DC_NAMEID), sizeof(USERFILE)));
    if (memory == NULL) {
        printf("Unable to allocate shared memory (error %lu).\n", GetLastError());
        return 1;
    }

    // Resolve the user name to its ID.
    if (!Io_UserNameToId(memory, argv[2], &userId)) {
        printf("The user name \"%s\" does not exist.\n", argv[2]);

    } else {
        // Retrieve the user file.
        if (!Io_UserGetFile(memory, userId, &userFile)) {
            printf("Unable to retrieve the user file for \"%s\".\n", argv[2]);

        } else {
            // Add, remove, or set credits.
            INT64 amount = _strtoi64(argv[3], NULL, 10);
            INT64 before = userFile.Credits[section];

            if (*argv[3] == '+' || *argv[3] == '-') {
                userFile.Credits[section] = before + amount;
            } else {
                userFile.Credits[section] = amount;
            }

            // Update the user file.
            if (!Io_UserSetFile(memory, &userFile)) {
                printf("Unable to update the user file for \"%s\".\n", argv[2]);

            } else {
                printf("Changed credits for \"%s\" from %I64dKB to %I64dKB in section %d.\n",
                    argv[2], before, userFile.Credits[section], section);
                result = 0;
            }
        }
    }

    // Clean up.
    Io_ShmFree(memory);

    return result;
}

void Usage(const char *argv0)
{
    printf("\n");
    printf("Usage: %s <window> <user> <amount> [section]\n\n", argv0);
    printf("Arguments:\n");
    printf("  window  - ioFTPD's message window.\n");
    printf("  user    - User to modify the credits of.\n");
    printf("  amount  - Amount of credits to change, in kilobytes.\n");
    printf("  section - Credit section number, 0 if not specified.\n\n");
    printf("Examples:\n");
    printf("  %s ioFTPD::MessageWindow bill +102400\n", argv0);
    printf("  %s ioFTPD::MessageWindow john 0\n", argv0);
    printf("  %s ioFTPD::MessageWindow paul -102400 5\n", argv0);
}
