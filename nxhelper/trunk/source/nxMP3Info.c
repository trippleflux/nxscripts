#include <nxHelper.h>

// Function definitions
static LONGLONG SeekFile(HANDLE FileHandle, LONGLONG Offset, ULONG MoveMethod);
static VOID MP3CopyTag(PBYTE Source, PCHAR String, ULONG Length);
static LONG MP3GetFrameBitrate(MP3INFO *MP3Info);
static BOOL MP3ValidVbrHeader(PBYTE VbrBuffer, PLONG Frames);

_forceinline ULONG MP3GetFrameHeader(PBYTE FrameBuffer)
{
    return (
        ((FrameBuffer[0] & 255) << 24) |
        ((FrameBuffer[1] & 255) << 16) |
        ((FrameBuffer[2] & 255) <<  8) |
        (FrameBuffer[3] & 255)
    );
}

#define MP3ValidFrameHeader(FrameHeader) (                \
    (MP3GetFrameSync(FrameHeader)      & 2047) == 2047 && \
    (MP3GetVersionIndex(FrameHeader)   &    3) !=    1 && \
    (MP3GetLayerIndex(FrameHeader)     &    3) !=    0 && \
    (MP3GetBitrateIndex(FrameHeader)   &   15) !=    0 && \
    (MP3GetBitrateIndex(FrameHeader)   &   15) !=   15 && \
    (MP3GetFrequencyIndex(FrameHeader) &    3) !=    3 && \
    (MP3GetEmphasisIndex(FrameHeader)  &    3) !=    2    \
)

static LONGLONG SeekFile(HANDLE FileHandle, LONGLONG Offset, ULONG MoveMethod)
{
    LARGE_INTEGER LargeInt;

    LargeInt.QuadPart = Offset;
    LargeInt.LowPart = SetFilePointer(FileHandle, LargeInt.LowPart, &LargeInt.HighPart, MoveMethod);

    if (LargeInt.LowPart == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR) {
        LargeInt.QuadPart = -1;
    }

    return LargeInt.QuadPart;
}

static VOID MP3CopyTag(PBYTE Source, PCHAR String, ULONG Length)
{
    ULONG i;

    // This function will copy all data until a NULL or the value
    // of "Length" is reached, trailing whitespace is not copied.

    // Example scenarios:
    //
    // "012345678901234567890123456789"
    // "Eternal Tears of Sorrow0000000"
    // "Eternal Tears of Sorrow0XXXXXX"
    // "Eternal Tears of Sorrow       "

    // Locate the end index of the string.
    for (i = 0; i < Length && Source[i]; i++);

    // Trim off trailing whitespace.
    while (i > 0 && isspace(Source[i-1])) {
        i--;
    }

    CopyMemory(String, Source, i);
    String[i] = '\0';
}

static LONG MP3GetFrameBitrate(MP3INFO *MP3Info)
{
    const static LONG Table[2][3][16] = {
        //MPEG 2 and 2.5
        {
            {0,  8, 16, 24, 32, 40, 48, 56, 64, 80, 96,112,128,144,160,0}, // Layer III
            {0,  8, 16, 24, 32, 40, 48, 56, 64, 80, 96,112,128,144,160,0}, // Layer II
            {0, 32, 48, 56, 64, 80, 96,112,128,144,160,176,192,224,256,0}  // Layer I
        },
        //MPEG 1
        {
            {0, 32, 40, 48, 56, 64, 80, 96,112,128,160,192,224,256,320,0}, // Layer III
            {0, 32, 48, 56, 64, 80, 96,112,128,160,192,224,256,320,384,0}, // Layer II
            {0, 32, 64, 96,128,160,192,224,256,288,320,352,384,416,448,0}  // Layer I
        }
    };

    return Table[(MP3GetVersionIndex(MP3Info->FrameHeader) & 1)]
                [(MP3GetLayerIndex(MP3Info->FrameHeader) - 1)]
                [MP3GetBitrateIndex(MP3Info->FrameHeader)];
}

static BOOL MP3ValidVbrHeader(PBYTE VbrBuffer, PLONG Frames)
{
    ULONG Flags;

    // The VBR header must begin with "Xing".
    if (memcmp(VbrBuffer, "Xing", 4) != 0) {
        *Frames = -1;
        return FALSE;
    }

    // Retrieve the flags from the next four byte.
    Flags = (ULONG)(
        ((VbrBuffer[4] & 255) << 24) |
        ((VbrBuffer[5] & 255) << 16) |
        ((VbrBuffer[6] & 255) <<  8) |
         (VbrBuffer[7] & 255)
    );

    // This bit contains the number of frames, it will saved
    // for later use when calculating the bitrate and length.
    if (Flags & MP3_FLAG_VBR_FRAMES) {
        *Frames = (LONG)(
            ((VbrBuffer[8] & 255) << 24) |
            ((VbrBuffer[9] & 255) << 16) |
            ((VbrBuffer[10] & 255) <<  8) |
             (VbrBuffer[11] & 255)
        );
    } else {
        // The frames are not specified in the header.
        *Frames = -1;
    }

    return TRUE;
}

BOOL MP3OpenFile(PTCHAR FilePath, MP3INFO *MP3Info)
{
    ULONG SizeHigh;
    ULONG SizeLow;

    MP3Info->FileHandle = CreateFile(
        FilePath,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_SEQUENTIAL_SCAN,
        NULL);

    if (MP3Info->FileHandle == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    SizeLow = GetFileSize(MP3Info->FileHandle, &SizeHigh);

    if (SizeLow == INVALID_FILE_SIZE) {
        MP3Info->FileSize = -1;
    } else {
        MP3Info->FileSize = ((LONGLONG)SizeHigh << 32) + SizeLow;
    }

    return TRUE;
}

BOOL MP3CloseFile(MP3INFO *MP3Info)
{
    return CloseHandle(MP3Info->FileHandle);
}

BOOL MP3LoadHeader(MP3INFO *MP3Info)
{
    BYTE  Buffer[MP3_BUFFER];
    LONGLONG VbrOffset;
    ULONG BytesRead;
    ULONG i;

    SetFilePointer(MP3Info->FileHandle, 0, 0, FILE_BEGIN);

    // Read a 256 byte chunk and proceed if 3 or more bytes
    // were read, since the MP3 frame header is 4 byte long.
    while (ReadFile(MP3Info->FileHandle, Buffer, MP3_BUFFER, &BytesRead, NULL) && BytesRead > 3) {

        // Process the data chunk 1 byte at a time.
        for (i = 0; i < BytesRead - 4; i++) {

            // Validate the MP3 header.
            MP3Info->FrameHeader = MP3GetFrameHeader(Buffer+i);
            if (MP3ValidFrameHeader(MP3Info->FrameHeader)) {

                // Save the file position following the frame header.
                VbrOffset = SeekFile(MP3Info->FileHandle, 0, FILE_CURRENT) - BytesRead + i + 4;
                goto ValidHeader;
            }
        }

        // Move the file pointer 3 bytes back for the next read.
        SetFilePointer(MP3Info->FileHandle, -3, 0, FILE_CURRENT);
    }

    return FALSE;

    ValidHeader:

    // Determining the VBR header's offset from the first frame header
    // depends on the MPEG version and the mode (mono/stereo).

    if (MP3GetVersionIndex(MP3Info->FrameHeader) == 3) {
        // MPEG version 1
        if (MP3GetModeIndex(MP3Info->FrameHeader) == 3) {
            // Single channel
            VbrOffset += 17;
        } else {
            // Dual channel
            VbrOffset += 32;
        }
    } else {
        // MPEG version 2 and 2.5
        if (MP3GetModeIndex(MP3Info->FrameHeader) == 3) {
            // Single channel
            VbrOffset += 9;
        } else {
            // Dual channel
            VbrOffset += 17;
        }
    }

    // Read the next twelve bytes, which contain the VBR header.
    SeekFile(MP3Info->FileHandle, VbrOffset, FILE_BEGIN);
    ReadFile(MP3Info->FileHandle, Buffer, 12, &BytesRead, NULL);

    // Validate the VBR header.
    if (BytesRead == 12 && MP3ValidVbrHeader(Buffer, &(MP3Info->VbrFrames))) {
        MP3Info->IsVbr = TRUE;
    } else {
        MP3Info->IsVbr = FALSE;
        MP3Info->VbrFrames = -1;
    }

    return TRUE;
}

BOOL MP3GetTag(MP3INFO *MP3Info, MP3TAG *MP3Tag)
{
    BYTE  Buffer[128];
    BYTE  GenreIndex;
    CHAR  Year[5];
    ULONG BytesRead;
    const static CHAR *Genres[149] = {
        "Blues", "Classic Rock", "Country", "Dance", "Disco", "Funk", "Grunge",
        "Hip-Hop", "Jazz", "Metal", "New Age", "Oldies", "Other", "Pop", "R&B",
        "Rap", "Reggae", "Rock", "Techno", "Industrial", "Alternative", "Ska",
        "Death Metal", "Pranks", "Soundtrack", "Euro-Techno", "Ambient", "Trip-Hop",
        "Vocal", "Jazz+Funk", "Fusion", "Trance", "Classical", "Instrumental",
        "Acid", "House", "Game", "Sound Clip",  "Gospel","Noise", "Alt. Rock",
        "Bass", "Soul", "Punk",  "Space", "Meditative", "Instrumental Pop",
        "Instrumental Rock", "Ethnic", "Gothic", "Darkwave", "Techno-Industrial",
        "Electronic", "Pop-Folk", "Eurodance", "Dream", "Southern Rock","Comedy",
        "Cult", "Gangsta Rap", "Top 40", "Christian Rap", "Pop Funk", "Jungle",
        "Native American", "Cabaret", "New Wave", "Psychedelic", "Rave", "Showtunes",
        "Trailer", "Lo-Fi", "Tribal", "Acid Punk", "Acid Jazz", "Polka", "Retro",
        "Musical", "Rock & Roll", "Hard Rock", "Folk", "Folk Rock", "National Folk",
        "Swing", "Fast-Fusion", "Bebob", "Latin", "Revival", "Celtic", "Bluegrass",
        "Avantgarde", "Gothic Rock", "Progressive Rock", "Psychedelic Rock",
        "Symphonic Rock", "Slow Rock", "Big Band",  "Chorus", "Easy Listening",
        "Acoustic", "Humour", "Speech", "Chanson", "Opera", "Chamber Music",
        "Sonata", "Symphony", "Booty Bass", "Primus", "Porn Groove", "Satire",
        "Slow Jam", "Club", "Tango", "Samba",  "Folklore", "Ballad", "Power Ballad",
        "Rhythmic Soul", "Freestyle", "Duet", "Punk Rock", "Drum Solo", "A Cappella",
        "Euro-House", "Dance Hall",  "Goa", "Drum & Bass", "Club-House", "Hardcore",
        "Terror", "Indie", "BritPop", "Negerpunk", "Polsk Punk", "Beat",
        "Christian Gangsta Rap", "Heavy Metal", "Black Metal", "Crossover",
        "Contemporary Christian", "Christian Rock", "Merengue", "Salsa",
        "Thrash Metal", "Anime", "JPop", "Synthpop", "Unknown"
    };

    if (SeekFile(MP3Info->FileHandle, -128, FILE_END) == -1) {
        return FALSE;
    }

    ReadFile(MP3Info->FileHandle, Buffer, 128, &BytesRead, NULL);

    if (BytesRead != 128 || memcmp(Buffer, "TAG", 3) != 0) {
        return FALSE;
    }

    MP3CopyTag(Buffer+3,  MP3Tag->Title,  30);
    MP3CopyTag(Buffer+33, MP3Tag->Artist, 30);
    MP3CopyTag(Buffer+63, MP3Tag->Album,  30);

    MP3CopyTag(Buffer+93, Year, 4);
    MP3Tag->Year = atol(Year);

    if (Buffer[125] == '\0' && Buffer[126] != '\0') {
        MP3Tag->Version = (FLOAT)1.1;
        MP3Tag->Track = (LONG)Buffer[126];

        MP3CopyTag(Buffer+97, MP3Tag->Comment, 28);
    } else {
        MP3Tag->Version = (FLOAT)1.0;
        MP3Tag->Track = 0;

        MP3CopyTag(Buffer+97, MP3Tag->Comment, 30);
    }

    // Find the genre name.
    GenreIndex = Buffer[127];
    if (GenreIndex > 148) {
        GenreIndex = 148;
    }
    StringCchCopyA(MP3Tag->Genre, 31, Genres[GenreIndex]);

    return TRUE;
}

LONG MP3GetBitrate(MP3INFO *MP3Info)
{
    if (MP3Info->IsVbr) {
        FLOAT FrameSize;
        LONG Frames = MP3GetFrames(MP3Info);

        if (Frames < 1) {
            return 0;
        }

        // The average frame size is represented by: file size/frames
        FrameSize = (FLOAT)(MP3Info->FileSize / (LONGLONG)Frames);

        return (LONG)(
            (FrameSize * (FLOAT)MP3GetFrequency(MP3Info)) /
            (1000.0 * (MP3GetLayerIndex(MP3Info->FrameHeader) == 3 ? 12.0 : 144.0))
        );
    } else {
        return MP3GetFrameBitrate(MP3Info);
    }
}

LONG MP3GetFrames(MP3INFO *MP3Info)
{
    if (MP3Info->IsVbr) {
        return MP3Info->VbrFrames;
    } else {
        FLOAT FrameSize = (FLOAT)(
            (MP3GetLayerIndex(MP3Info->FrameHeader) == 3 ? 12 : 144) *
            (1000.0 * (FLOAT)MP3GetFrameBitrate(MP3Info) / (FLOAT)MP3GetFrequency(MP3Info))
        );

        return (LONG)((FLOAT)MP3Info->FileSize / FrameSize);
    }
}

LONG MP3GetFrequency(MP3INFO *MP3Info)
{
    const static LONG Table[4][3] = {
        {32000, 16000,  8000}, // MPEG 2.5
        {    0,     0,     0}, // Reserved
        {22050, 24000, 16000}, // MPEG 2
        {44100, 48000, 32000}  // MPEG 1
    };

    return Table[MP3GetVersionIndex(MP3Info->FrameHeader)]
                [MP3GetFrequencyIndex(MP3Info->FrameHeader)];
}

LONG MP3GetLayer(MP3INFO *MP3Info)
{
    return (4 - MP3GetLayerIndex(MP3Info->FrameHeader));
}

LONG MP3GetLength(MP3INFO *MP3Info)
{
    LONG Bitrate = MP3GetBitrate(MP3Info);

    if (Bitrate < 1) {
        return 0;
    }

    return (LONG)((8 * MP3Info->FileSize / 1000) / (LONGLONG)Bitrate);
}

PCHAR MP3GetMode(MP3INFO *MP3Info)
{
    switch (MP3GetModeIndex(MP3Info->FrameHeader)) {
        case 1:
            return "Joint Stereo";
        case 2:
            return "Dual Channel";
        case 3:
            return "Single Channel";
        default:
            return "Stereo";
    }
}

FLOAT MP3GetVersion(MP3INFO *MP3Info)
{
    const static FLOAT Table[4] = {2.5, 0.0, 2.0, 1.0};

    return Table[MP3GetVersionIndex(MP3Info->FrameHeader)];
}

#if 0
INT _tmain(INT argc, TCHAR *argv[])
{
    MP3INFO MP3Info;
    MP3TAG MP3Tag;

    if (argc < 2) {
        _tprintf(TEXT("Usage: %s <file>\n"), argv[0]);
        return -1;
    }

    if (MP3OpenFile(argv[1], &MP3Info)) {

        if (MP3LoadHeader(&MP3Info)) {
            //
            // Display MP3 information.
            //
            printf(" - MP3 Info ------------\n");
            printf(" - Bitrate = %ld Kbit\n", MP3GetBitrate(&MP3Info));
            printf(" - Frames  = %ld\n",      MP3GetFrames(&MP3Info));
            printf(" - Freq    = %ld hz\n",   MP3GetFrequency(&MP3Info));
            printf(" - Layer   = %ld\n",      MP3GetLayer(&MP3Info));
            printf(" - Length  = %ld secs\n", MP3GetLength(&MP3Info));
            printf(" - Mode    = %s\n",       MP3GetMode(&MP3Info));
            printf(" - Type    = %s\n",       MP3Info.IsVbr ? "VBR" : "CBR");
            printf(" - Version = %.1f\n",     MP3GetVersion(&MP3Info));

            //
            // Display ID3 information.
            //
            printf(" - ID3 Tags ------------\n");
            if (MP3GetTag(&MP3Info, &MP3Tag)) {
                printf(" - Title   = \"%s\"\n",   MP3Tag.Title);
                printf(" - Artist  = \"%s\"\n",   MP3Tag.Artist);
                printf(" - Album   = \"%s\"\n",   MP3Tag.Album);
                printf(" - Genre   = \"%s\"\n",   MP3Tag.Genre);
                printf(" - Comment = \"%s\"\n",   MP3Tag.Comment);
                printf(" - Year    = %ld\n",  MP3Tag.Year);
                printf(" - Track   = %ld\n",  MP3Tag.Track);
                printf(" - Version = %.1f\n", MP3Tag.Version);
            } else {
                printf(" - None\n");
            }

        } else {
            printf(" - Invalid MP3.\n");
        }

        MP3CloseFile(&MP3Info);
    } else {
        printf(" - Unable to open file (error: %lu).\n", GetLastError());
    }

    return 0;
}
#endif
