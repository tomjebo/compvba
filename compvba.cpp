// compvba.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <cmath>
#include "structures.h"


// clean up memory.
void Finalize() {
	free(pCompressedContainer);
	free(pDecompressedBuffer);
}

// 2.4.1.3.19.1 CopyToken Help
void CopyTokenHelp(unsigned short* LengthMask, unsigned short* OffsetMask, unsigned short* BitCount, unsigned short* MaximumLength) {

	unsigned short difference = pDecompressedCurrent - pDecompressedChunkStart;

	// log2 of difference, i.e. num of bits needed to encode difference in binary.
	//*BitCount = 1; // preload the first bit.
	//while (difference >>= 1)
	//	(*BitCount)++;

	*BitCount = ceil(log2(difference));

	if (*BitCount < 4)
		*BitCount = 4;

	*LengthMask = 0xFFFF >> *BitCount;

	*OffsetMask = ~(*LengthMask);

	*MaximumLength = (0xFFFF >> *BitCount) + 3;

}

// 2.4.1.3.19.4 Matching
void Matching(unsigned char* pDecompressedEnd, unsigned short* Offset, unsigned short* Length) {


	unsigned char* pCandidate = pDecompressedCurrent - 1;

	unsigned short BestLength = 0;
	unsigned short MaximumLength = 0;
	unsigned short BitCount = 0;
	unsigned short LengthMask = 0;
	unsigned short OffsetMask = 0;
	unsigned char* BestCandidate = 0;

	while (pCandidate >= pDecompressedChunkStart) {
		unsigned char* pC = pCandidate;
		unsigned char* pD = pDecompressedCurrent;
		int Len = 0;

		while ((pD < pDecompressedEnd) && (pD[0] == pC[0])) {
			Len++;
			pC++;
			pD++;
		}

		if (Len > BestLength) {
			BestLength = Len;
			BestCandidate = pCandidate;
		}

		pCandidate--;
	} // end while

	if (BestLength >= 3) {

		CopyTokenHelp(&LengthMask, &OffsetMask, &BitCount, &MaximumLength);

		*Length = MIN(BestLength, MaximumLength);

		*Offset = pDecompressedCurrent - BestCandidate;
	}
	else {
		*Length = 0;
		*Offset = 0;
	}
}

void PackCopyToken(unsigned short Offset, unsigned short Length, unsigned short* Token) {

	// 2.4.1.3.19.3 Pack CopyToken

	unsigned short MaximumLength = 0;
	unsigned short BitCount = 0;
	unsigned short LengthMask = 0;
	unsigned short OffsetMask = 0;

	CopyTokenHelp(&LengthMask, &OffsetMask, &BitCount, &MaximumLength);

	unsigned short temp1 = Offset - 1;
	unsigned short temp2 = 16 - BitCount;
	unsigned short temp3 = Length - 3;

	*Token = (temp1 << temp2) | temp3;
}

//  2.4.1.3.18 Set FlagBit
void SetFlagBit(int index, int Flag, BYTE* Flags) {
	unsigned short temp1 = Flag << index;
	unsigned short temp2 = *Flags & (~temp1);
	*Flags = temp2 | temp1;
}

// 2.4.1.3.9 Compressing a Token
void CompressingAToken(unsigned char* pCompressedEnd, unsigned char* pDecompressedEnd, int index, BYTE* Flags) {

	unsigned short Offset = 0;
	unsigned short Length = 0;
	unsigned short Token = 0;

	Matching(pDecompressedEnd, &Offset, &Length);

	if (Offset) {
		if ((pCompressedCurrent + 1) < pCompressedEnd) {
			PackCopyToken(Offset, Length, &Token);

			// copy Token to current compressed stream, little endian.
			pCompressedCurrent[0] = (BYTE)(Token & 0x00FF);
			pCompressedCurrent[1] = (BYTE)(Token >> 8);

			SetFlagBit(index, 1, Flags);

			pCompressedCurrent += 2;
			pDecompressedCurrent += Length;
		}
		else {
			pCompressedCurrent = pCompressedEnd;
		}
	} // end if
	else {
		if (pCompressedCurrent < pCompressedEnd) {
			pCompressedCurrent[0] = pDecompressedCurrent[0];
			pCompressedCurrent++;
			pDecompressedCurrent++;
		}
		else {
			pCompressedCurrent = pCompressedEnd;
		}
	}
}

// 2.4.1.3.8 Compressing a TokenSequence
void CompressingATokenSequence(unsigned char* pCompressedEnd, unsigned char* pDecompressedEnd) {


	unsigned char* pFlagByteIndex = pCompressedCurrent;

	BYTE TokenFlags = 0b00000000;

	pCompressedCurrent++;

	for (int index = 0; index <= 7; index++) {
		if ((pDecompressedCurrent < pDecompressedEnd) && (pCompressedCurrent < pCompressedEnd)) {
			CompressingAToken(pCompressedEnd, pDecompressedEnd, index, &TokenFlags);
		}
	} // end for

	pFlagByteIndex[0] = TokenFlags;

}

// 2.4.1.3.10 Compressing a RawChunk
void CompressingARawChunk(unsigned char* LastByte) {

	pCompressedCurrent = pCompressedChunkStart + 2;
	pDecompressedCurrent = pDecompressedChunkStart;
	unsigned short PadCount = 4096;

	while (pDecompressedCurrent <= LastByte) {
		pCompressedCurrent[0] = pDecompressedCurrent[0];
		pCompressedCurrent++;
		pDecompressedCurrent++;
		PadCount--;
	} // end while 

	for (unsigned short counter = 1; counter <= PadCount; counter++) {
		pCompressedCurrent[0] = 0x00;
		pCompressedCurrent++;
	} // end for

}


// 2.4.1.3.13 Pack CompressedChunkSize
void PackCompressedChunkSize(unsigned short Size, unsigned short* Header) {
	unsigned short temp1 = *Header & 0xF000;
	unsigned short temp2 = Size - 3;
	*Header = temp1 | temp2;
}

// 2.4.1.3.16 Pack CompressedChunkFlag
void PackCompressedChunkFlag(int CompressedFlag, unsigned short* Header) {
	unsigned short temp1 = *Header & 0x7FFF;
	unsigned short temp2 = CompressedFlag << 15;
	*Header = temp1 | temp2;
}

// 2.4.1.3.14 Pack CompressedChunkSignature
void PackCompressedChunkSignature(unsigned short* Header) {
	unsigned short temp = *Header & 0x8FFF;
	*Header = temp | 0x3000;
}

// 2.4.1.3.7 Compressing a DecompressedChunk
void CompressingADecompressedChunk() {

	unsigned char* pCompressedEnd = pCompressedChunkStart + 4098; // two bytes for the header + 4096
	pCompressedCurrent = pCompressedChunkStart + 2; // skip the header

	unsigned char* pDecompressedEnd = 0;
	int CompressedFlag = 0;
	unsigned short Size = 0;
	unsigned short Header = 0x0000;

	if ((pDecompressedChunkStart + 4096) < pDecompressedBufferEnd) {
		pDecompressedEnd = pDecompressedChunkStart + 4096;
	}
	else
		pDecompressedEnd = pDecompressedBufferEnd;

	while ((pDecompressedCurrent < pDecompressedEnd) && (pCompressedCurrent < pCompressedEnd)) {
		CompressingATokenSequence(pCompressedEnd, pDecompressedEnd);
	}

	if (pDecompressedCurrent < pDecompressedEnd) {
		CompressingARawChunk(pDecompressedEnd - 1);
		CompressedFlag = 0;
	}
	else
		CompressedFlag = 1;

	Size = pCompressedCurrent - pCompressedChunkStart;
	Header = 0x0000;

	PackCompressedChunkSize(Size, &Header);

	PackCompressedChunkFlag(CompressedFlag, &Header);

	PackCompressedChunkSignature(&Header);

	pCompressedChunkStart[1] = (BYTE)(Header >> 8);
	pCompressedChunkStart[0] = (BYTE)(Header & 0x00FF);
}

void Initialize(const char* pData) {

	// 2.4.1.2 State Variables
	// 
	// anchors for compressed and decompressed workspaces.
	pCompressedContainer = (CompressedContainer*)calloc(sizeof(CompressedContainer), 1);
	pDecompressedBuffer = (DecompressedBuffer*)calloc(sizeof(DecompressedBuffer), 1);

	if (pCompressedContainer == NULL || pDecompressedBuffer == NULL) {
		printf_s("Failed to allocate memory for workspace buffers!\n");
		exit(1);
	}

	pDecompressedCurrent = (unsigned char*)pDecompressedBuffer;
	pCompressedCurrent = (unsigned char*)pCompressedContainer;

	size_t cData = strnlen_s(pData, (size_t)MAX_INPUT_SIZE + 1);

	if (cData > MAX_INPUT_SIZE) {
		printf_s("Input buffer is longer than max allowed!\n");
		exit(1);
	}

	int cChunk = 0;
	int cCurrent = 0;

	// break up and copy the initial data into the decompress chunks.
	for (cChunk = 0; (cChunk < MAX_DECOMPRESSED_CHUNKS) && (cData > (cChunk * MAX_DECOMPRESSED_CHUNK_SIZE) + cCurrent); cChunk++) {
		for (cCurrent = 0; ((cData > (cChunk * MAX_DECOMPRESSED_CHUNK_SIZE) + cCurrent)
			&& (cCurrent < (MAX_DECOMPRESSED_CHUNK_SIZE))); cCurrent++) {
			pDecompressedBuffer->Chunk[cChunk].Data[cCurrent] = 
			//pDecompressedCurrent[cCurrent + (MAX_DECOMPRESSED_CHUNK_SIZE * cChunk)] =
				pData[cCurrent + (MAX_DECOMPRESSED_CHUNK_SIZE * cChunk)];
		}
	}

	pDecompressedBufferEnd = (pDecompressedBuffer->Chunk[cChunk-1].Data) + ((cChunk-1) * MAX_DECOMPRESSED_CHUNK_SIZE) + cCurrent;
	//pDecompressedBufferEnd = pDecompressedCurrent + cCurrent + (MAX_DECOMPRESSED_CHUNK_SIZE * cChunk) + 1;
	pDecompressedChunkStart = (unsigned char*)pDecompressedBuffer->Chunk;
};

int main(int argc, char* argv[]) {

	const char* pInput = "#aaabcdefaaaaghijaaaaaklaaamnopqaaaaaaaaaaaarstuvwxyzaaa";

	Initialize(pInput);

	// 2.4.1.3.6 Compression algorithm

	pCompressedContainer->SignatureByte = 0x01;

	pCompressedCurrent++;

	while (pDecompressedCurrent < pDecompressedBufferEnd) {
		pCompressedChunkStart = pCompressedCurrent;
		pDecompressedChunkStart = pDecompressedCurrent;

		CompressingADecompressedChunk();
	}


	Finalize();
};