/*++

AlcoBot - Alcoholicz site bot.
Copyright (c) 2005-2006 Alcoholicz Scripting Team

Module Name:
    Print Data Structures

Author:
    neoxed (neoxed@gmail.com) Nov 8, 2005

Abstract:
    This application displays the byte alignment, member size, and total size
    of the data structures used for glFTPD's binary files.

--*/

/* Microsoft's CRT uses an 8-byte time_t by default. */
#if defined(_WIN32) && !defined(_WIN64)
#   define _USE_32BIT_TIME_T
#endif

#include <stdio.h>
#include <stddef.h>
#include <string.h>
#ifdef _WIN32
#   include <time.h>
typedef unsigned short ushort;
#else
#   include <sys/time.h>
#   include <sys/types.h>
#endif /* _WIN32 */

/* Force structure alignment to 4 bytes. */
#pragma pack(push, 4)

struct dirlog {
    ushort status;              /* 0 = NEWDIR, 1 = NUKE, 2 = UNNUKE, 3 = DELETED */
    time_t uptime;              /* Creation time since epoch (man 2 time) */
    ushort uploader;            /* The userid of the creator */
    ushort group;               /* The groupid of the primary group of the creator */
    ushort files;               /* The number of files inside the dir */
    unsigned long long bytes;   /* The number of bytes in the dir */
    char dirname[255];          /* The name of the dir (fullpath) */
    struct dirlog *nxt;         /* Unused, kept for compatibility reasons */
    struct dirlog *prv;         /* Unused, kept for compatibility reasons */
};

struct dupefile {
    char filename[256];         /* The filename */
    time_t timeup;              /* Creation time since epoch (man 2 time) */
    char uploader[25];          /* Name of the uploader */
};

struct nukelog {
    ushort status;              /* 0 = NUKED, 1 = UNNUKED */
    time_t nuketime;            /* The nuke time since epoch (man 2 time) */
    char nuker[12];             /* The name of the nuker */
    char unnuker[12];           /* The name of the unnuker */
    char nukee[12];             /* The name of the nukee */
    ushort mult;                /* The nuke multiplier */
    float bytes;                /* The number of bytes nuked */
    char reason[60];            /* The nuke reason */
    char dirname[255];          /* The dirname (fullpath) */
    struct nukelog *nxt;        /* Unused, kept for compatibility reasons */
    struct nukelog *prv;        /* Unused, kept for compatibility reasons */
};

struct oneliner {
    char uname[24];             /* The user that added the oneliner */
    char gname[24];             /* The primary group of the user who added the oneliner */
    char tagline[64];           /* The tagline of the user who added the oneliner */
    time_t timestamp;           /* The time the message was added (epoch) */
    char message[100];          /* The message (oneliner) */
};

static void
PrintStruct(
    const char *name,
    const void *buffer,
    size_t length
    );


int
main(
    int argc,
    char **argv
    )
{
    struct dirlog   dirlog;
    struct dupefile dupefile;
    struct nukelog  nukelog;
    struct oneliner oneliner;

    /* Structure: dirlog */
    memset(&dirlog,          '0', sizeof(dirlog));
    memset(&dirlog.status,   'A', sizeof(dirlog.status));
    memset(&dirlog.uptime,   'B', sizeof(dirlog.uptime));
    memset(&dirlog.uploader, 'C', sizeof(dirlog.uploader));
    memset(&dirlog.group,    'D', sizeof(dirlog.group));
    memset(&dirlog.files,    'E', sizeof(dirlog.files));
    memset(&dirlog.bytes,    'F', sizeof(dirlog.bytes));
    memset(&dirlog.dirname,  'G', sizeof(dirlog.dirname));
    memset(&dirlog.nxt,      'H', sizeof(dirlog.nxt));
    memset(&dirlog.prv,      'I', sizeof(dirlog.prv));
    PrintStruct("dirlog", &dirlog, sizeof(dirlog));

    /* Structure: dupefile */
    memset(&dupefile,          '0', sizeof(dupefile));
    memset(&dupefile.filename, 'A', sizeof(dupefile.filename));
    memset(&dupefile.timeup,   'B', sizeof(dupefile.timeup));
    memset(&dupefile.uploader, 'C', sizeof(dupefile.uploader));
    PrintStruct("dupefile", &dupefile, sizeof(dupefile));

    /* Structure: nukelog */
    memset(&nukelog,          '0', sizeof(nukelog));
    memset(&nukelog.status,   'A', sizeof(nukelog.status));
    memset(&nukelog.nuketime, 'B', sizeof(nukelog.nuketime));
    memset(&nukelog.nuker,    'C', sizeof(nukelog.nuker));
    memset(&nukelog.unnuker,  'D', sizeof(nukelog.unnuker));
    memset(&nukelog.nukee,    'E', sizeof(nukelog.nukee));
    memset(&nukelog.mult,     'F', sizeof(nukelog.mult));
    memset(&nukelog.bytes,    'G', sizeof(nukelog.bytes));
    memset(&nukelog.reason,   'H', sizeof(nukelog.reason));
    memset(&nukelog.dirname,  'I', sizeof(nukelog.dirname));
    memset(&nukelog.nxt,      'J', sizeof(nukelog.nxt));
    memset(&nukelog.prv,      'K', sizeof(nukelog.prv));
    PrintStruct("nukelog", &nukelog, sizeof(nukelog));

    /* Structure: oneliner */
    memset(&oneliner,           '0', sizeof(oneliner));
    memset(&oneliner.uname,     'A', sizeof(oneliner.uname));
    memset(&oneliner.gname,     'B', sizeof(oneliner.gname));
    memset(&oneliner.tagline,   'C', sizeof(oneliner.tagline));
    memset(&oneliner.timestamp, 'D', sizeof(oneliner.timestamp));
    memset(&oneliner.message,   'E', sizeof(oneliner.message));
    PrintStruct("oneliner", &oneliner, sizeof(oneliner));

    return 0;
}

static void
PrintStruct(
    const char *name,
    const void *buffer,
    size_t length
    )
{
    size_t i, j;
    unsigned char *data = (unsigned char *)buffer;

    printf("############################################################\n");
    printf("# Structure: %-45s #\n", name);
    printf("############################################################\n\n");

    printf("Length: %lu bytes\n", length);
    printf("Format: <format string>\n");
    printf("Values: <variable list>\n\n");

    /* Print 50 bytes per line. */
    for (i = 0; i < length; i += 50) {
        j = length - i;
        if (j > 50) {
            j = 50;
        }
        printf("%.*s\n", j, data + i);
    }
    printf("\n");
}
