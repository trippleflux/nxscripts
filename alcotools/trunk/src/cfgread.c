/*++

AlcoTools - Alcoholicz dupe checker, zipscript, and utilities.
Copyright (c) 2005-2006 Alcoholicz Scripting Team

Module Name:
    Configuration Reader

Author:
    neoxed (neoxed@gmail.com) Aug 3, 2005

Abstract:
    This module implements a configuration file reader. The CRC-32 checksum
    of every key and section name is saved and used when looking up values.
    This is not the most foolproof scheme (e.g. CRC-32 checksum collisions),
    but it is "good enough" for this application.

--*/

#include "alcoholicz.h"

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
    LIST_ENTRY(CONFIG_KEY)  link;   // Pointer to next section structure
    apr_uint32_t            crc;    // CRC-32 checksum of the key name.
    apr_size_t              length; // Length of the string or number of array elements.
    union {
        char         **array;
        bool_t       boolean;
        apr_uint32_t integer;
        char         *string;
    }                       value;  // Internal value representation.
    apr_byte_t              type;   // Value type.
};
LIST_HEAD(CONFIG_KEY_HEAD, CONFIG_KEY);

struct CONFIG_SECTION {
    SLIST_ENTRY(CONFIG_SECTION) link;   // Pointer to next section structure
    apr_uint32_t                crc;    // CRC-32 checksum of the section name
    CONFIG_KEY_HEAD             keys;   // Keys belonging to this section
};
SLIST_HEAD(CONFIG_SECTION_HEAD, CONFIG_SECTION);

// Section structure list head
static CONFIG_SECTION_HEAD sectionHead;

// Sub-pool used for config allocations
static apr_pool_t *cfgPool;


/*++

CreateSection

    Creates a section structure and inserts it into the section list.

Arguments:
    sectionName   - Pointer to a string represeting the section's name.
                    This string is NOT null-terminated.

    sectionLength - Length of the section name, in characters.

Return Value:
    If the function succeeds, the return value is a pointer to a CONFIG_SECTION structure.

    If the function fails, the return value is null.

--*/
static
CONFIG_SECTION *
CreateSection(
    char *sectionName,
    apr_size_t sectionLength
    )
{
    apr_uint32_t sectionCrc;
    CONFIG_SECTION *section;

    ASSERT(sectionName  != NULL);
    ASSERT(sectionLength > 0);

    // Calculate the CRC-32 checksum of the section name
    sectionCrc = Crc32Memory(sectionName, sectionLength);

    // Check if the section already exists
    SLIST_FOREACH(section, &sectionHead, link) {
        if (section->crc == sectionCrc) {
            return section;
        }
    }

    section = apr_palloc(cfgPool, sizeof(CONFIG_SECTION));
    if (section != NULL) {
        section->crc = sectionCrc;
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
                  This string is NOT null-terminated.

    keyLength   - Length of the key name, in characters.

    value       - Pointer to a string represeting the key's value.
                  This string is NOT null-terminated.

    valueLength - Length of the value, in characters.

Return Value:
    None.

--*/
static
void
CreateKey(
    CONFIG_SECTION *section,
    char *keyName,
    apr_size_t keyLength,
    char *value,
    apr_size_t valueLength
    )
{
    apr_uint32_t keyCrc;
    bool_t exists = FALSE;
    CONFIG_KEY *key;

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

    // Calculate the CRC-32 checksum of the key name.
    keyCrc = Crc32Memory(keyName, keyLength);

    // Check if the key already exists in the section.
    LIST_FOREACH(key, &section->keys, link) {
        if (key->crc == keyCrc) {
            exists = TRUE;
            break;
        }
    }

    if (!exists) {
        key = apr_palloc(cfgPool, sizeof(CONFIG_KEY));
        if (key == NULL) {
            return;
        }

        // Initialize key structure
        key->value.string = apr_palloc(cfgPool, valueLength + 1);
        if (key->value.string == NULL) {
            return;
        }
        key->crc  = keyCrc;
        key->type = TYPE_STRING;

        // Insert key at the list's head
        LIST_INSERT_HEAD(&section->keys, key, link);

    } else if (key->length < valueLength) {
        // Allocate a memory block to accommodate the larger string.
        key->value.string = apr_palloc(cfgPool, valueLength + 1);

        if (key->value.string == NULL) {
            LIST_REMOVE(key, link);
            return;
        }
    }

    // Update the key's value and length.
    memcpy(key->value.string, value, valueLength);
    key->value.string[valueLength] = '\0';
    key->length = valueLength;
}

/*++

GetKey

    Locates the data structure for the specified key.

Arguments:
    sectionName - Pointer to a null-terminated string that specifies the section name.

    keyName     - Pointer to a null-terminated string that specifies the key name.

Return Value:
    If the key exists, the return value is a pointer to a CONFIG_KEY structure.

    If the key does not exist, the return value is null.

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
    apr_uint32_t keyCrc;
    apr_uint32_t sectionCrc;

    ASSERT(sectionName != NULL);
    ASSERT(keyName     != NULL);

    keyCrc = Crc32String(keyName);
    sectionCrc = Crc32String(sectionName);
    LOG_VERBOSE("Looking up key \"%s\" (0x%08X) in section \"%s\" (0x%08X).",
        keyName, keyCrc, sectionName, sectionCrc);

    SLIST_FOREACH(section, &sectionHead, link) {
        if (section->crc == sectionCrc) {
            LIST_FOREACH(key, &section->keys, link) {
                if (key->crc == keyCrc) {
                    return key;
                }
            }
            break;
        }
    }

    LOG_WARNING("Unable to find key \"%s\" in section \"%s\".", keyName, sectionName);
    return NULL;
}

/*++

ParseArray

    Parse the contents of a buffer into an array of null-terminated strings.

Arguments:
    buffer      - The null-terminated string to parse.

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
    char *buffer,
    char **array,
    apr_size_t *elements,
    char *data,
    apr_size_t *charCount
    )
{
    bool_t copyChar;
    bool_t inQuote = FALSE;
    apr_size_t slashCount;

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
        if (*buffer == '\0') {
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
            while (*buffer == '\\') {
                buffer++;
                slashCount++;
            }

            if (*buffer == '"') {
                if ((slashCount & 1) == 0) {
                    if (inQuote && *buffer+1 == '"') {
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
                    *data++ = '\\';
                }
                ++*charCount;
            }

            // Check if we have reached the end of the input buffer or found
            // the next array element (whitespace outside of quote characters).
            if (*buffer == '\0' || (!inQuote && ISSPACE(*buffer))) {
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
            *data++ = '\0';
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
    char *buffer,
    apr_size_t length
    )
{
    CONFIG_SECTION *section;
    char *bufferEnd;
    char *name;
    char *value;
    apr_size_t nameLength;

    bufferEnd = buffer + length;
    section = NULL;

    for (; buffer < bufferEnd; buffer++) {
        switch (*buffer) {
            case '#':
                // Ignore commented lines.
                while (buffer < bufferEnd && !ISEOL(*buffer)) {
                    buffer++;
                }
                break;
            case ' ':
            case '\t':
            case '\n':
            case '\r':
                // Ignore whitespace and EOL characters.
                break;
            case '[':
                name = buffer+1;
                nameLength = 0;

                // Retrieve the section name.
                while (buffer < bufferEnd && !ISEOL(*buffer)) {
                    buffer++;

                    //
                    // Update the length of the section's name every time a
                    // closing bracket is found. This allows the parser to
                    // handle section names containing brackets.
                    //
                    // For example, the name of "[[[Foo]]]" would be "[[Foo]]".
                    //
                    if (*buffer == ']') {
                        nameLength = (buffer - name);
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
                    if (*buffer == '=') {
                        nameLength = (buffer - name);
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
                        CreateKey(section, name, nameLength, value, (buffer - value));
                    }
                }
                break;
        }
    }
}


/*++

ConfigInit

    Initialize the configuration file subsystem.

Arguments:
    pool    - Main application pool, to create sub-pools from.

Return Values:
    Returns an APR status code.

--*/
apr_status_t
ConfigInit(
    apr_pool_t *pool
    )
{
    apr_byte_t   *buffer;
    apr_file_t   *file;
    apr_finfo_t  fileInfo;
    apr_size_t   length;
    apr_status_t status;

    // Initialize section list head
    SLIST_INIT(&sectionHead);

    // Create a sub-pool for configuration memory allocations
    status = apr_pool_create(&cfgPool, pool);
    if (status != APR_SUCCESS) {
        return status;
    }

    // Open configuration file for reading
    status = apr_file_open(&file, CONFIG_FILE, APR_FOPEN_READ, APR_OS_DEFAULT, cfgPool);
    if (status != APR_SUCCESS) {
        return status;
    }

    status = apr_file_info_get(&fileInfo, APR_FINFO_SIZE, file);
    if (status == APR_SUCCESS) {

        // Allocate a buffer large enough to contain the configuration file
        length = (apr_size_t)fileInfo.size;
        buffer = apr_palloc(cfgPool, length);
        if (buffer == NULL) {
            status = APR_ENOMEM;
        } else {
            // Buffer the configuration file
            status = apr_file_read(file, buffer, &length);
            if (status == APR_SUCCESS) {
                ParseBuffer((char *)buffer, length);
                status = APR_SUCCESS;
            }
        }
    }

    apr_file_close(file);
    return status;
}

/*++

ConfigGetArray

    Retrieves the key's value as an array of strings.

Arguments:
    sectionName - Pointer to a null-terminated string that specifies the section name.

    keyName     - Pointer to a null-terminated string that specifies the key name.

    array       - Location to store the address of the array of null-terminated
                  strings. Do not attempt to free this value.

    elements    - Location to store the number of elements in the array.

Return Value:
    If the key exists, the return value is APR_SUCCESS.

    If the key does not exist its value its cannot be converted, the return value
    is an appropriate APR status code.

--*/
int
ConfigGetArray(
    const char *sectionName,
    const char *keyName,
    char ***array,
    apr_size_t *elements
    )
{
    CONFIG_KEY *key;
    char **buffer;
    char *offset;
    apr_size_t bufferChars;
    apr_size_t bufferElements;

    ASSERT(sectionName != NULL);
    ASSERT(keyName     != NULL);
    ASSERT(array       != NULL);
    ASSERT(elements    != NULL);

    key = GetKey(sectionName, keyName);
    if (key == NULL) {
        return APR_EINVAL;
    }
    ASSERT(key->type == TYPE_ARRAY || key->type == TYPE_STRING);

    if (key->type == TYPE_ARRAY) {
        *array = key->value.array;
        *elements = key->length;
        return APR_SUCCESS;
    } else if (key->type != TYPE_STRING) {
        return APR_EINVAL;
    }

    // Calculate the required space to store the array and its elements.
    ParseArray(key->value.string, NULL, &bufferElements, NULL, &bufferChars);

    buffer = apr_palloc(cfgPool, (bufferElements * sizeof(char *)) + bufferChars);
    if (buffer == NULL) {
        return APR_ENOMEM;
    }

    // Convert the buffer into an array.
    offset = (char *)buffer + (bufferElements * sizeof(char *));
    ParseArray(key->value.string, buffer, &bufferElements, offset, &bufferChars);

    // Change the key's internal value representation to an array.
    key->type = TYPE_ARRAY;
    key->length = *elements = bufferElements;
    key->value.array = *array = buffer;
    return APR_SUCCESS;
}

/*++

ConfigGetBool

    Retrieves the key's value as a boolean.

Arguments:
    sectionName - Pointer to a null-terminated string that specifies the section name.

    keyName     - Pointer to a null-terminated string that specifies the key name.

    boolean     - Location to store the boolean value of the key.

Return Value:
    If the key exists, the return value is APR_SUCCESS.

    If the key does not exist its value its cannot be converted, the return value
    is an appropriate APR status code.

Remarks:
    Accepted boolean values are: 1, 0, yes, no, on, off, true, or false.

--*/
int
ConfigGetBool(
    const char *sectionName,
    const char *keyName,
    bool_t *boolean
    )
{
    CONFIG_KEY *key;
    apr_byte_t i;
    static const char text[] = "01onoffalseyestrue";
    static const struct {
        apr_byte_t offset;
        apr_byte_t length;
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
        return APR_EINVAL;
    }
    ASSERT(key->type == TYPE_BOOLEAN || key->type == TYPE_STRING);

    if (key->type == TYPE_BOOLEAN) {
        *boolean = key->value.boolean;
        return APR_SUCCESS;
    } else if (key->type != TYPE_STRING) {
        return APR_EINVAL;
    }

    for (i = 0; i < ARRAYSIZE(values); i++) {
        if (key->length == values[i].length && strncasecmp(&text[values[i].length], key->value.string, key->length) == 0) {
            // Change the key's internal value representation to a boolean.
            key->type = TYPE_BOOLEAN;
            key->value.boolean = *boolean = values[i].value;
            return APR_SUCCESS;
        }
    }

    return APR_EINVAL;
}

/*++

ConfigGetInt

    Retrieves the key's value as an integer.

Arguments:
    sectionName - Pointer to a null-terminated string that specifies the section name.

    keyName     - Pointer to a null-terminated string that specifies the key name.

    integer     - Location to store the integer value of the key.

Return Value:
    If the key exists, the return value is APR_SUCCESS.

    If the key does not exist its value its cannot be converted, the return value
    is an appropriate APR status code.

--*/
int
ConfigGetInt(
    const char *sectionName,
    const char *keyName,
    apr_uint32_t *integer
    )
{
    CONFIG_KEY *key;
    apr_uint32_t value;

    ASSERT(sectionName != NULL);
    ASSERT(keyName     != NULL);
    ASSERT(integer     != NULL);

    key = GetKey(sectionName, keyName);
    if (key == NULL) {
        return APR_EINVAL;
    }
    ASSERT(key->type == TYPE_INTEGER || key->type == TYPE_STRING);

    if (key->type == TYPE_INTEGER) {
        *integer = key->value.integer;
        return APR_SUCCESS;
    } else if (key->type != TYPE_STRING) {
        return APR_EINVAL;
    }

    value = (apr_uint32_t)strtoul(key->value.string, NULL, 10);
    if (value == ULONG_MAX) {
        return APR_EINVAL;
    }

    // Change the key's internal value representation to an integer.
    key->type = TYPE_INTEGER;
    key->value.integer = *integer = value;
    return APR_SUCCESS;
}

/*++

ConfigGetString

    Retrieves the key's value as a string.

Arguments:
    sectionName - Pointer to a null-terminated string that specifies the section name.

    keyName     - Pointer to a null-terminated string that specifies the key name.

    string      - Location to store the null-terminated string's address.
                  Do NOT attempt to free this value.

    length      - Location to store the string's length. This argument can be null.

Return Value:
    If the key exists, the return value is APR_SUCCESS.

    If the key does not exist, the return value is an appropriate APR status code.

--*/
int
ConfigGetString(
    const char *sectionName,
    const char *keyName,
    char **string,
    apr_size_t *length
    )
{
    CONFIG_KEY *key;

    ASSERT(sectionName != NULL);
    ASSERT(keyName     != NULL);
    ASSERT(string      != NULL);

    key = GetKey(sectionName, keyName);
    if (key == NULL) {
        return APR_EINVAL;
    }
    ASSERT(key->type == TYPE_STRING);

    if (key->type == TYPE_STRING) {
        *string = key->value.string;
        if (length != NULL) {
            *length = key->length;
        }
        return APR_SUCCESS;
    }

    return APR_EINVAL;
}
