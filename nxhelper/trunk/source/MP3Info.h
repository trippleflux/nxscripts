#ifndef __MP3INFO_H__
#define __MP3INFO_H__

// Size of read buffer in bytes (must be at least 12).
#define MP3_BUFFER    256

// VBR header flags.
#define MP3_FLAG_VBR_FRAMES     0x0001
#define MP3_FLAG_VBR_BYTES      0x0002
#define MP3_FLAG_VBR_TOC        0x0004
#define MP3_FLAG_VBR_SCALE      0x0008

typedef struct {
    HANDLE FileHandle;  // A handle to the MP3 file, with read rights
    LONGLONG FileSize;  // Size of MP3 file in bytes
    ULONG FrameHeader;  // The MP3 file's frame header
    BOOL  IsVbr;        // Indicates if the file has a variable bitrate
    LONG  VbrFrames;    // Frame count from the VBR header, -1 if none
} MP3INFO;

typedef struct {
    CHAR  Title[31];   // Song or recording title
    CHAR  Artist[31];  // Artist or author's name
    CHAR  Album[31];   // Title of the recording
    CHAR  Genre[31];   // Genre or type of music
    LONG  Year;        // Release year
    CHAR  Comment[31]; // File comment
    LONG  Track;       // Track number (1.1 only)
    FLOAT Version;     // ID3 version (only 1.0 and 1.1 are supported)
} MP3TAG;

// Frame header bit manipulation macros
#define MP3GetFrameSync(FrameHeader)     ((FrameHeader >> 21) & 2047)
#define MP3GetVersionIndex(FrameHeader)  ((FrameHeader >> 19) & 3 )
#define MP3GetLayerIndex(FrameHeader)    ((FrameHeader >> 17) & 3 )
#define MP3GetProtectionBit(FrameHeader) ((FrameHeader >> 16) & 1 )
#define MP3GetBitrateIndex(FrameHeader)  ((FrameHeader >> 12) & 15)
#define MP3GetFrequencyIndex(FrameHeader)((FrameHeader >> 10) & 3 )
#define MP3GetPaddingBit(FrameHeader)    ((FrameHeader >>  9) & 1 )
#define MP3GetPrivateBit(FrameHeader)    ((FrameHeader >>  8) & 1 )
#define MP3GetModeIndex(FrameHeader)     ((FrameHeader >>  6) & 3 )
#define MP3GetModeExtIndex(FrameHeader)  ((FrameHeader >>  4) & 3 )
#define MP3GetCopyrightBit(FrameHeader)  ((FrameHeader >>  3) & 1 )
#define MP3GetOrginalBit(FrameHeader)    ((FrameHeader >>  2) & 1 )
#define MP3GetEmphasisIndex(FrameHeader) ((FrameHeader      ) & 3 )

// User functions
BOOL MP3OpenFile(PTCHAR FilePath, MP3INFO *MP3Info);
BOOL MP3CloseFile(MP3INFO *MP3Info);
BOOL MP3LoadHeader(MP3INFO *MP3Info);
BOOL MP3GetTag(MP3INFO *MP3Info, MP3TAG *MP3Tag);
LONG MP3GetBitrate(MP3INFO *MP3Info);
LONG MP3GetFrames(MP3INFO *MP3Info);
LONG MP3GetFrequency(MP3INFO *MP3Info);
LONG MP3GetLayer(MP3INFO *MP3Info);
LONG MP3GetLength(MP3INFO *MP3Info);
PCHAR MP3GetMode(MP3INFO *MP3Info);
FLOAT MP3GetVersion(MP3INFO *MP3Info);

#endif // __MP3INFO_H__
