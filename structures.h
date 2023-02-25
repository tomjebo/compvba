#pragma once
// From [MS-OVBA] 2.4.1.1 Structures
// These are data structures used by the compression algorithm.

typedef unsigned char BYTE;
#define MAX_INPUT_SIZE (100)
#define MAX_COMPRESSED_CHUNK_SIZE (4096)
#define MAX_DECOMPRESSED_CHUNK_SIZE (4096)
#define MAX_COMPRESSED_CHUNKS (5)
#define MAX_DECOMPRESSED_CHUNKS (5)
#define CompressedHeaderSize sizeof(CompressedHeader)

#define MIN(left, right) (left < right)? left : right

// 2.4.1.1.5 CompressedChunkHeader

typedef struct {
	short CompressedChunkSize:12;
	short A:3;
	short B:1;
} CompressedChunkHeader;

// 2.4.1.1.4 CompressedChunk

typedef struct  {
	//CompressedChunkHeader CompressedHeader;
	unsigned char CompressedData[sizeof(CompressedChunkHeader) + MAX_COMPRESSED_CHUNK_SIZE];
} CompressedChunk;

// 2.4.1.1.1 CompressedContainer

typedef struct CompressedContainer {
	BYTE SignatureByte = 0x01;
	CompressedChunk Chunks[MAX_COMPRESSED_CHUNKS];
} CompressedContainer;

typedef struct DecompressedChunk {
	BYTE Data[MAX_DECOMPRESSED_CHUNK_SIZE];
} DecompressedChunk;

typedef struct DecompressedBuffer {
	DecompressedChunk Chunk[MAX_DECOMPRESSED_CHUNKS];
} DecompressedBuffer;

// 2.4.1.2 State Variables 

// compressed workspace variables
CompressedContainer* pCompressedContainer;
unsigned char* pCompressedCurrent;
unsigned char* pCompressedRecordEnd;
unsigned char* pCompressedChunkStart;

// decompressed workspace variables 
DecompressedBuffer* pDecompressedBuffer;
unsigned char* pDecompressedCurrent;
unsigned char* pDecompressedBufferEnd;
unsigned char* pDecompressedChunkStart;
