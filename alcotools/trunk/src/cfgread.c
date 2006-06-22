/*++

AlcoTools - Alcoholicz dupe checker, zipscript, and utilities.
Copyright (c) 2005-2006 Alcoholicz Scripting Team

Module Name:
    Configuration Reader

Author:
    neoxed (neoxed@gmail.com) Aug 3, 2005

Abstract:
    This module implements a configuration file reader. The CRC32 checksum
    of every key and section name is saved and used when looking up values.
    This is not the most foolproof scheme (e.g. CRC32 checksum collisions),
    but it is "good enough" for this application.

--*/

#include "alcoholicz.h"

// Maximum length of a key or section name, in characters
#define MAX_LEN         24

// Value types
#define TYPE_ARRAY      1
#define TYPE_BOOLEAN    2
#define TYPE_INTEGER    3
#define TYPE_STRING     4

// Forward type declarations
typedef struct CONFIG_KEY CONFIG_KEY;
typedef struct CONFIG_KEY_HEAD CONFIG_KEY_HEAD;
typedef struct CONFIG_SECTION CONFIG_SECTION;
typedef struct CONFIG_SECTION_HEAD CONFIG_SECTION_HEAD;

struct CONFIG_KEY {
    LIST_ENTRY(CONFIG_KEY) link;    // Pointer to next section structure
    uint32_t  crc;                  // CRC32 checksum of the key name.
    uint32_t  length;               // Length of the string or number of array elements.
    union {
        tchar_t **array;
        bool_t  boolean;
        int32_t integer;
        tchar_t *string;
    } value;                        // Internal value representation.
    uint8_t type;                   // Value type.
};
LIST_HEAD(CONFIG_KEY_HEAD, CONFIG_KEY);

struct CONFIG_SECTION {
    SLIST_ENTRY(CONFIG_SECTION) link;   // Pointer to next section structure
    CONFIG_KEY_HEAD             keys;   // Keys belonging to this section
    uint32_t                    crc;    // CRC32 checksum of the section name
};
SLIST_HEAD(CONFIG_SECTION_HEAD, CONFIG_SECTION);

// Section structure list head
static CONFIG_SECTION_HEAD sectionHead = SLIST_HEAD_INITIALIZER(sectionHead);

static const tchar_t *configFile  = TEXT("AlcoTools.conf");


/*++

CreateSection

    Creates a section structure and inserts it into the section list.

Arguments:
    sectionName   - Pointer to a string represeting the section's name.
                    This string is NOT null terminated.

    sectionLength - Length of the section name, in characters.

Return Value:
    If the function succeeds, the return value is a pointer to an allocated
    "CONFIG_SECTION" structure. If the function fails, the return value is NULL.

--*/
static
CONFIG_SECTION *
CreateSection(
    tchar_t *sectionName,
    uint32_t sectionLength
    )
{
    CONFIG_SECTION *section;
    uint32_t sectionCrc;
#ifdef UNICODE
    char ansiName[MAX_LEN];
    uint32_t ansiLength;
#endif

    ASSERT(sectionName  != NULL);
    ASSERT(sectionLength > 0);

    // Calculate the CRC32 checksum of the section name
#ifdef UNICODE
    ansiLength = (uint32_t)WideCharToMultiByte(CP_ACP, 0, sectionName,
        (int)sectionLength, ansiName, ARRAYSIZE(ansiName), NULL, NULL);

    sectionCrc = Crc32Memory(ansiName, ansiLength);
#else
    sectionCrc = Crc32Memory(sectionName, sectionLength);
#endif // UNICODE

    // Check if the section already exists
    SLIST_FOREACH(section, &sectionHead, link) {
        if (section->crc == sectionCrc) {
            return section;
        }
    }

    section = MemAlloc(sizeof(CONFIG_SECTION));
    if (section != NULL) {
        section->crc  = sectionCrc;
        LIST_INIT(&section->keys);
        SLIST_INSERT_HEAD(&sectionHead, section, link);
    }

    return section;
}

/*++

CreateKey

    Creates a key structure and inserts it into the section list.

Arguments:
    section     - Pointer to a section structure which the key belongs to.

    keyName     - Pointer to a string represeting the key's name.
                  This string is NOT null terminated.

    keyLength   - Length of the key name, in characters.

    value       - Pointer to a string represeting the key's value.
                  This string is NOT null terminated.

    valueLength - Length of the value, in characters.

Return Value:
    None.

--*/
static
void
CreateKey(
    CONFIG_SECTION *section,
    tchar_t *keyName,
    uint32_t keyLength,
    tchar_t *value,
    uint32_t valueLength
    )
{
    bool_t exists = FALSE;
    CONFIG_KEY *key;
    uint32_t keyCrc;
#ifdef UNICODE
    char ansiName[MAX_LEN];
    uint32_t ansiLength;
#endif

    ASSERT(section   != NULL);
    ASSERT(keyName   != NULL);
    ASSERT(keyLength  > 0);
    ASSERT(value     != NULL);

    // Remove trailing whitespace from the key name.
    while (keyLength > 0 && ISSPACE(keyName[keyLength - 1])) {
        keyLength--;
    }

    // Remove leading and trailing whitespace from the value.
    while (valueLength > 0 && ISSPACE(*value)) {
        value++;
        valueLength--;
    }
    while (valueLength > 0 && ISSPACE(value[valueLength - 1])) {
        valueLength--;
    }

    // Calculate the CRC32 checksum of the key name.
#ifdef UNICODE
    ansiLength = (uint32_t)WideCharToMultiByte(CP_ACP, 0, keyName,
        (int)keyLength, ansiName, ARRAYSIZE(ansiName), NULL, NULL);

    keyCrc = Crc32Memory(ansiName, ansiLength);
#else
    keyCrc = Crc32Memory(keyName, keyLength);
#endif // UNICODE

    // Check if the key already exists in the section.
    LIST_FOREACH(key, &section->keys, link) {
        if (key->crc == keyCrc) {
            exists = TRUE;
            break;
        }
    }

    if (!exists) {
        key = MemAlloc(sizeof(CONFIG_KEY));
        if (key == NULL) {
            return;
        }

        // Initialise key structure
        key->value.string = MemAlloc((valueLength + 1) * sizeof(tchar_t));
        if (key->value.string == NULL) {
            MemFree(key);
            return;
        }
        key->crc  = keyCrc;
        key->type = TYPE_STRING;

        // Insert key at the list's head
        LIST_INSERT_HEAD(&section->keys, key, link);

    } else if (key->length < valueLength) {
        // Resize memory block to accommodate the larger string.
        key->value.string = MemRealloc(key->value.string, (valueLength + 1) * sizeof(tchar_t));

        if (key->value.string == NULL) {
            LIST_REMOVE(key, link);
            MemFree(key);
            return;
        }
    }

    // Update the key's value and length.
    memcpy(key->value.string, value, valueLength * sizeof(tchar_t));
    key->value.string[valueLength] = TEXT('\0');
    key->length = valueLength;
}

/*++

GetKey

    Locates the data structure for the specified key.

Arguments:
    sectionName - Pointer to a null-terminated string that specifies the section name.

    keyName     - Pointer to a null-terminated string that specifies the key name.

Return Value:
    If the key exists, the return value is a pointer to a "Configkey"
    structure. If the key does not exist, the return value is NULL.

--*/
static
CONFIG_KEY *
GetKey(
    const char *sectionName,
    const char *keyName
    )
{
    CONFIG_KEY *key;
    CONFIG_SECTION *section;
    uint32_t keyCrc;
    uint32_t sectionCrc;

    ASSERT(sectionName != NULL);
    ASSERT(keyName     != NULL);

    keyCrc = Crc32String(keyName);
    sectionCrc = Crc32String(sectionName);
    VERBOSE(TEXT("Looking up key 0x%08X in section 0x%08X.\n"), keyCrc, sectionCrc);

    SLIST_FOREACH(section, &sectionHead, link) {
        if (section->crc != sectionCrc) {
            continue;
        }

        LIST_FOREACH(key, &section->keys, link) {
            if (key->crc == keyCrc) {
                return key;
            }
        }
    }

    WARNING(TEXT("Could not find key \"%s\" in section \"%s\".\n"), keyCrc, sectionCrc);
    return NULL;
}

/*++

ParseArray

    Parse the contents of a buffer into an array of null terminated strings.

Arguments:
    buffer      - The null terminated string to parse.

    array       - Location to the array of pointers. This argument can be null.

    elements    - Number of elements in the array.

    data        - Location to store the elements. This argument can be null.

    charCount   - Number of characters in the array elements.

Return Value:
    None.

--*/
static
void
ParseArray(
    tchar_t *buffer,
    tchar_t **array,
    uint32_t *elements,
    tchar_t *data,
    uint32_t *charCount
    )
{
    bool_t copyChar;
    bool_t inQuote = FALSE;
    uint32_t slashCount;

    ASSERT(buffer    != NULL);
    ASSERT(elements  != NULL);
    ASSERT(charCount != NULL);

    *elements = 0;
    *charCount = 0;

    for (;;) {
        // Ignore whitespace.
        while (ISSPACE(*buffer)) {
            buffer++;
        }
        if (*buffer == TEXT('\0')) {
            break;
        }
        if (array != NULL) {
            *array++ = data;
        }
        ++*elements;

        // Process the array element.
        for (;;) {
            copyChar = TRUE;
            slashCount = 0;

            // Count the number of blackslashes.
            while (*buffer == TEXT('\\')) {
                buffer++;
                slashCount++;
            }

            if (*buffer == TEXT('"')) {
                if ((slashCount & 1) == 0) {
                    if (inQuote && *buffer+1 == TEXT('"')) {
                        // Double quote the inside string.
                        buffer++;
                    } else {
                        // Skip the first quote and copy the second one.
                        copyChar = FALSE;
                        inQuote = !inQuote;
                    }
                }
                slashCount /= 2;
            }

            // Append backslashes to the array element.
            while (slashCount--) {
                if (data != NULL) {
                    *data++ = TEXT('\\');
                }
                ++*charCount;
            }

            // Check if we have reached the end of the input buffer or found
            // the next array element (whitespace outside of quote characters).
            if (*buffer == TEXT('\0') || (!inQuote && ISSPACE(*buffer))) {
                break;
            }

            if (copyChar) {
                if (data != NULL) {
                    *data++ = *buffer;
                }
                ++*charCount;
            }
            buffer++;
        }

        if (data != NULL) {
            // Null terminate the array element.
            *data++ = TEXT('\0');
        }
        ++*charCount;
    }
}

/*++

ParseBuffer

    Parse the buffered contents of a configuration file.

Arguments:
    buffer  - Contents of the configuration file.

    length  - Length of the buffer, in characters.

Return Value:
    None.

--*/
static void
ParseBuffer(
    tchar_t *buffer,
    uint32_t length
    )
{
    CONFIG_SECTION *section;
    tchar_t *bufferEnd;
    tchar_t *name;
    tchar_t *value;
    uint32_t nameLength;

    bufferEnd = buffer + length;
    section = NULL;

    for (; buffer < bufferEnd; buffer++) {
        switch (*buffer) {
            case TEXT('#'):
                // Ignore commented lines.
                while (buffer < bufferEnd && !ISEOL(*buffer)) {
                    buffer++;
                }
                break;
            case TEXT(' '):
            case TEXT('\t'):
            case TEXT('\n'):
            case TEXT('\r'):
                // Ignore whitespace and EOL characters.
                break;
            case TEXT('['):
                name = buffer+1;
                nameLength = 0;

                // Retrieve the section name.
                while (buffer < bufferEnd && !ISEOL(*buffer)) {
                    buffer++;

                    //
                    // Update the length of the section's name everytime a
                    // closing bracket is found. This allows the parser to
                    // handle section names containing brackets.
                    //
                    // For example, the name of "[[[Foo]]]" would be "[[Foo]]".
                    //
                    if (*buffer == TEXT(']')) {
                        nameLength = (uint32_t)(buffer - name);
                    }
                }

                if (nameLength > 0) {
                    section = CreateSection(name, nameLength);
                } else {
                    // Invalid section name.
                    section = NULL;
                }
                break;
            default:
                name = buffer;
                nameLength = 0;

                // Retrieve the key name.
                while (buffer < bufferEnd && !ISEOL(*buffer)) {
                    buffer++;

                    // Stop when a key/value separator is found (i.e. equal sign).
                    if (*buffer == TEXT('=')) {
                        nameLength = (uint32_t)(buffer - name);
                        break;
                    }
                }

                if (nameLength > 0) {
                    value = buffer;
                    while (buffer < bufferEnd && !ISEOL(*buffer)) {
                        buffer++;
                    }

                    // Make sure the key was found in a valid section.
                    if (section != NULL) {
                        // Skip past the key/value separator.
                        value++;
                        CreateKey(section, name, nameLength, value, (uint32_t)(buffer - value));
                    }
                }
                break;
        }
    }
}


/*++

ConfigInit

    Initialise the configuration file subsystem.

Arguments:
    None.

Return Value:
    If the function succeeds, the return value is nonzero. If the function
    fails, the return value is zero. To get extended error information, call
    GetSystemErrorMessage.

--*/
bool_t
ConfigInit(
    void
    )
{
    bool_t result = FALSE;
    byte_t *buffer = NULL;
    FILE_HANDLE handle;
    uint32_t bufferLen;
    uint64_t size;
#ifdef UNICODE
    byte_t *convBuffer;
    int wideChars;
    UINT codePage;
    wchar_t *wideBuffer = NULL;
#endif

    handle = FileOpen(configFile, FACCESS_READ, FEXIST_PRESENT, FOPTION_SEQUENTIAL);
    if (handle == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    // Buffer the contents of the configuration file. It's not
    // worth memory-mapping the file due to its small size.
    if (!FileSize(handle, &size)) {
        goto cleanUp;
    }

    buffer = MemAlloc((size_t)size);
    if (buffer == NULL || !FileRead(handle, buffer, (uint32_t)size, &bufferLen)) {
        goto cleanUp;
    }

#ifdef UNICODE
    //
    // Byte Order Markers
    // http://www.unicode.org/faq/utf_bom.html
    //
    // Bytes            Encoding Form
    // 00 00 FE FF      UTF-32, big endian
    // FF FE 00 00      UTF-32, little endian
    // FE FF            UTF-16, big endian
    // FF FE            UTF-16, little endian
    // EF BB BF         UTF-8
    //

    // Check if the UTF-8 BOM is present (I'm too lazy to check for other BOMs).
    convBuffer = buffer;
    if (bufferLen >= 3 && *buffer == 0xEF && *buffer+1 == 0xBB && *buffer+2 == 0xBF) {
        codePage = CP_UTF8;
        *convBuffer =+ 3;
    } else {
        codePage = CP_ACP;
    }

    // Calculate the required destination buffer size.
    wideChars = MultiByteToWideChar(codePage, 0, (char *)convBuffer,
        (int)bufferLen, NULL, 0);
    if (wideChars < 1) {
        goto cleanUp;
    }

    wideBuffer = MemAlloc((size_t)wideChars * sizeof(wchar_t));
    if (wideBuffer == NULL) {
        goto cleanUp;
    }

    wideChars = MultiByteToWideChar(codePage, 0, (char *)convBuffer,
        (int)bufferLen, wideBuffer, wideChars);
    if (wideChars < 1) {
        goto cleanUp;
    }

    ParseBuffer(wideBuffer, (uint32_t)wideChars);
#else
    ParseBuffer((char *)buffer, bufferLen);
#endif // UNICODE

    result = TRUE;
cleanUp:

    if (buffer != NULL) {
        MemFree(buffer);
    }
#ifdef UNICODE
    if (wideBuffer != NULL) {
        MemFree(wideBuffer);
    }
#endif // UNICODE

    FileClose(handle);
    return result;
}

/*++

ConfigFinalise

    Finalise the configuration file subsystem.

Arguments:
    None.

Return Value:
    None.

--*/
void
ConfigFinalise(
    void
    )
{
    CONFIG_KEY *key;
    CONFIG_SECTION *section;

    while (!SLIST_EMPTY(&sectionHead)) {
        section = SLIST_FIRST(&sectionHead);

        while (!LIST_EMPTY(&section->keys)) {
            key = LIST_FIRST(&section->keys);

            // Free the key's value.
            switch (key->type) {
                case TYPE_ARRAY:
                    MemFree(key->value.array);
                    break;
                case TYPE_STRING:
                    MemFree(key->value.string);
                    break;
            }

            // Remove key structure from the list and free it
            LIST_REMOVE(key, link);
            MemFree(key);
        }

        // Remove section structure from the list and free it
        SLIST_REMOVE_HEAD(&sectionHead, link);
        MemFree(section);
    }
}

/*++

ConfigGetArray

    Retrieves the key's value as an array of strings.

Arguments:
    sectionName - Pointer to a null-terminated string that specifies the section name.

    keyName     - Pointer to a null-terminated string that specifies the key name.

    array       - Location to store the address of the array of null terminated
                  strings. Do not attempt to free this value.

    elements    - Location to store the number of elements in the array.

Return Value:
    If the key exists, the return value is ALCOHOL_OK. If the key does not
    exist, the return value is an appropriate error code.

--*/
int
ConfigGetArray(
    const char *sectionName,
    const char *keyName,
    tchar_t ***array,
    uint32_t *elements
    )
{
    CONFIG_KEY *key;
    tchar_t **buffer;
    tchar_t *offset;
    uint32_t bufferChars;
    uint32_t bufferElements;

    ASSERT(sectionName != NULL);
    ASSERT(keyName     != NULL);
    ASSERT(array       != NULL);
    ASSERT(elements    != NULL);

    key = GetKey(sectionName, keyName);
    if (key == NULL) {
        return ALCOHOL_UNKNOWN;
    }

    if (key->type == TYPE_ARRAY) {
        // No conversion needed.
        *array = key->value.array;
        *elements = key->length;
        return ALCOHOL_OK;
    } else if (key->type != TYPE_STRING) {
        // Only convert from strings.
        ASSERT(0);
        return ALCOHOL_ERROR;
    }

    // Calculate the required space to store the array and its elements.
    ParseArray(key->value.string, NULL, &bufferElements, NULL, &bufferChars);

    buffer = MemAlloc((bufferElements * sizeof(tchar_t *)) + (bufferChars * sizeof(tchar_t)));
    if (buffer == NULL) {
        return ALCOHOL_INSUFFICIENT_MEMORY;
    }

    // Convert the buffer into an array.
    offset = (tchar_t *)buffer + (bufferElements * sizeof(tchar_t *));
    ParseArray(key->value.string, buffer, &bufferElements, offset, &bufferChars);

    // Change the key's internal value representation to an array.
    MemFree(key->value.string);
    key->type = TYPE_ARRAY;
    key->length = *elements = bufferElements;
    key->value.array = *array = buffer;
    return ALCOHOL_OK;
}

/*++

ConfigGetBool

    Retrieves the key's value as a boolean.

Arguments:
    sectionName - Pointer to a null-terminated string that specifies the section name.

    keyName     - Pointer to a null-terminated string that specifies the key name.

    boolean     - Location to store the boolean value of the key.

Return Value:
    If the key exists and its value is a valid boolean, the return value is
    ALCOHOL_OK. If the key does not exist or its valid is an invalid boolean,
    the return value is an appropriate error code.

Remarks:
    Accepted boolean values are: "1", "0", "yes", "no", "true", or "false".

--*/
int
ConfigGetBool(
    const char *sectionName,
    const char *keyName,
    bool_t *boolean
    )
{
    CONFIG_KEY *key;
    uint8_t i;
    static const tchar_t text[] = TEXT("01onoffalseyestrue");
    static const struct {
        uint8_t offset;
        uint8_t length;
        bool_t  value;
    } values[] = {
        {0,  1, FALSE}, // "0"
        {1,  1, TRUE},  // "1"
        {2,  2, TRUE},  // "on"
        {3,  2, FALSE}, // "no"
        {4,  3, FALSE}, // "off"
        {6,  5, FALSE}, // "false"
        {11, 3, TRUE},  // "yes"
        {14, 4, TRUE}   // "true"
    };

    ASSERT(sectionName != NULL);
    ASSERT(keyName     != NULL);
    ASSERT(boolean     != NULL);

    key = GetKey(sectionName, keyName);
    if (key == NULL) {
        return ALCOHOL_UNKNOWN;
    }

    if (key->type == TYPE_BOOLEAN) {
        // No conversion needed.
        *boolean = key->value.boolean;
        return ALCOHOL_OK;
    } else if (key->type != TYPE_STRING) {
        // Only convert from strings.
        ASSERT(0);
        return ALCOHOL_ERROR;
    }

    for (i = 0; i < ARRAYSIZE(values); i++) {
        if (key->length == values[i].length && t_strncasecmp(&text[values[i].length], key->value.string, key->length) == 0) {
            // Change the key's internal value representation to a boolean.
            MemFree(key->value.string);
            key->type = TYPE_BOOLEAN;
            key->value.boolean = *boolean = values[i].value;
            return ALCOHOL_OK;
        }
    }

    return ALCOHOL_INVALID_DATA;
}

/*++

ConfigGetInt

    Retrieves the key's value as an integer.

Arguments:
    sectionName - Pointer to a null-terminated string that specifies the section name.

    keyName     - Pointer to a null-terminated string that specifies the key name.

    integer     - Location to store the integer value of the key.

Return Value:
    If the key exists and its value is a valid integer, the return value is
    ALCOHOL_OK. If the key does not exist or its valid is an invalid integer,
    the return value is an appropriate error code.

--*/
int
ConfigGetInt(
    const char *sectionName,
    const char *keyName,
    uint32_t *integer
    )
{
    CONFIG_KEY *key;
    uint32_t value;

    ASSERT(sectionName != NULL);
    ASSERT(keyName     != NULL);
    ASSERT(integer     != NULL);

    key = GetKey(sectionName, keyName);
    if (key == NULL) {
        return ALCOHOL_UNKNOWN;
    }

    if (key->type == TYPE_INTEGER) {
        // No conversion needed.
        *integer = key->value.integer;
        return ALCOHOL_OK;
    } else if (key->type != TYPE_STRING) {
        // Only convert from strings.
        ASSERT(0);
        return ALCOHOL_ERROR;
    }

    value = t_strtoul(key->value.string, NULL, 10);
    if (value == ULONG_MAX) {
        return ALCOHOL_INVALID_DATA;
    }

    // Change the key's internal value representation to an integer.
    MemFree(key->value.string);
    key->type = TYPE_INTEGER;
    key->value.integer = *integer = value;
    return ALCOHOL_OK;
}

/*++

ConfigGetString

    Retrieves the key's value as a string.

Arguments:
    sectionName - Pointer to a null-terminated string that specifies the section name.

    keyName     - Pointer to a null-terminated string that specifies the key name.

    string      - Location to store the null terminated string's address.
                  Do NOT attempt to free this value.

    length      - Location to store the string's length. This argument can be null.

Return Value:
    If the key exists, the return value is ALCOHOL_OK. If the key does not
    exist, the return value is an appropriate error code.

--*/
int
ConfigGetString(
    const char *sectionName,
    const char *keyName,
    tchar_t **string,
    uint32_t *length
    )
{
    CONFIG_KEY *key;

    ASSERT(sectionName != NULL);
    ASSERT(keyName     != NULL);
    ASSERT(string      != NULL);

    key = GetKey(sectionName, keyName);
    if (key == NULL) {
        return ALCOHOL_UNKNOWN;
    }

    if (key->type == TYPE_STRING) {
        // No conversion needed.
        *string = key->value.string;
        if (length != NULL) {
            *length = key->length;
        }
        return ALCOHOL_OK;
    }

    // Only convert from strings.
    ASSERT(0);
    return ALCOHOL_ERROR;
}
