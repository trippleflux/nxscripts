/*++

AlcoTools - Alcoholicz dupe checker, zipscript, and utilities.
Copyright (c) 2005-2006 Alcoholicz Scripting Team

Module Name:
    Configuration Reader

Author:
    neoxed (neoxed@gmail.com) Aug 3, 2005

Abstract:
    Configuration file reading function prototypes.

--*/

#ifndef _CFGREAD_H_
#define _CFGREAD_H_

bool_t
ConfigInit(
    void
    );

void
ConfigFinalise(
    void
    );

int
ConfigGetArray(
    const char *sectionName,
    const char *keyName,
    tchar_t ***array,
    uint32_t *elements
    );

int
ConfigGetBool(
    const char *sectionName,
    const char *keyName,
    bool_t *boolean
    );

int
ConfigGetInt(
    const char *sectionName,
    const char *keyName,
    uint32_t *integer
    );

int
ConfigGetString(
    const char *sectionName,
    const char *keyName,
    tchar_t **string,
    uint32_t *length
    );


//
// Section values
//

#define SectionDupeCheck        "DupeCheck"
#define SectionForceFiles       "ForceFiles"
#define SectionGeneral          "General"
#define SectionZipScript        "ZipScript"

//
// Key values
//

#define DupeCheckDirs           "checkDirs"           // BOOL
#define DupeCheckFiles          "checkFiles"          // BOOL
#define DupeExcludeCheck        "excludeCheck"        // ARRAY
#define DupeExcludeLog          "excludeLog"          // ARRAY
#define DupeIgnoreDirs          "ignoreDirs"          // ARRAY
#define DupeIgnoreFiles         "ignoreFiles"         // ARRAY

#define ForceExcludePaths       "excludePaths"        // ARRAY
#define ForceFilePaths          "filePaths"           // ARRAY
#define ForceNfoFirst           "nfoFirst"            // BOOL
#define ForceSampleFirst        "sampleFirst"         // BOOL
#define ForceSamplePaths        "samplePaths"         // ARRAY
#define ForceSfvFirst           "sfvFirst"            // BOOL

#define GeneralDataPath         "dataPath"            // STRING
#define GeneralLogLevel         "logLevel"            // INTEGER
#define GeneralMsgWindow        "msgWindow"           // STRING
#define GeneralTextPath         "textPath"            // STRING

#define ZsExcludePaths          "excludePaths"        // ARRAY
#define ZsExtractDiz            "extractDiz"          // BOOL
#define ZsExtractNfo            "extractNfo"          // BOOL
#define ZsGroupPaths            "groupPaths"          // ARRAY
#define ZsHalfwayFiles          "halfwayFiles"        // INTEGER
#define ZsLeaderFiles           "leaderFiles"         // INTEGER
#define ZsTagComplete           "tagComplete"         // STRING
#define ZsTagCompleteMP3        "tagCompleteMP3"      // STRING
#define ZsTagIncomplete         "tagIncomplete"       // STRING
#define ZsTagIncompleteParent   "tagIncompleteParent" // STRING

#endif // _CFGREAD_H_
