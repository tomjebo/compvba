# compvba

A C++ reference implementation of the VBA source code compression and decompression algorithms defined in the [MS-OVBA] Open Specification: *Office VBA File Format Structure*.

---

## Overview

Microsoft Office documents that contain VBA macros store the macro source code in a compressed format inside a Compound File Binary (CFB) container. The compression scheme is a proprietary LZ77-variant described in section **2.4.1** of [[MS-OVBA]](https://learn.microsoft.com/en-us/openspecs/office_file_formats/ms-ovba/575462ba-bf67-4190-9fac-c275523c75fc).

This project implements both the **compression** and **decompression** paths of that algorithm in a single self-contained C++ program, closely following the pseudocode published in the specification. It is intended as a learning aid, a debugging tool, and a correctness reference when working with VBA binary streams extracted from Office files (`.xlsm`, `.docm`, `.pptm`, etc.).

---

## How It Works

### Data Structures — [MS-OVBA] 2.4.1.1

`structures.h` declares the low-level types that mirror the on-disk layout:

| Type | Spec section | Description |
|------|-------------|-------------|
| `CompressedChunkHeader` | §2.4.1.1.5 | 2-byte header carrying the chunk size (12 bits), signature (3 bits), and compressed flag (1 bit) |
| `CompressedChunk` | §2.4.1.1.4 | One compressed chunk: header + up to 4 096 bytes of compressed data |
| `CompressedContainer` | §2.4.1.1.1 | Top-level container: signature byte `0x01` followed by one or more `CompressedChunk`s |
| `DecompressedChunk` / `DecompressedBuffer` | §2.4.1.2 | Working buffers used during compression and decompression |

### State Variables — [MS-OVBA] 2.4.1.2

A set of global pointer variables tracks the current read/write positions in both the compressed and decompressed workspaces:

- `pCompressedContainer`, `pCompressedCurrent`, `pCompressedChunkStart`, `pCompressedRecordEnd`
- `pDecompressedBuffer`, `pDecompressedCurrent`, `pDecompressedChunkStart`, `pDecompressedBufferEnd`

### Compression — [MS-OVBA] 2.4.1.3.6 – 2.4.1.3.19

| Function | Spec section | Description |
|----------|-------------|-------------|
| `CompressingADecompressedChunk()` | §2.4.1.3.7 | Outer loop: processes one 4 096-byte decompressed chunk, writes header |
| `CompressingATokenSequence()` | §2.4.1.3.8 | Processes up to 8 tokens, building the flag byte |
| `CompressingAToken()` | §2.4.1.3.9 | Decides whether to emit a literal byte or a copy token |
| `CompressingARawChunk()` | §2.4.1.3.10 | Emits an uncompressed (raw) chunk when compression would expand the data |
| `Matching()` | §2.4.1.3.19.4 | Searches backward in the decompressed window for the best match |
| `PackCopyToken()` | §2.4.1.3.19.3 | Encodes an `(Offset, Length)` pair into a 2-byte copy token |
| `PackCompressedChunkSize()` | §2.4.1.3.13 | Sets the size field in the chunk header |
| `PackCompressedChunkFlag()` | §2.4.1.3.16 | Sets the compressed/raw flag bit in the chunk header |
| `PackCompressedChunkSignature()` | §2.4.1.3.14 | Sets the 3-bit signature field (`0b011`) in the chunk header |
| `CopyTokenHelp()` | §2.4.1.3.19.1 | Computes `LengthMask`, `OffsetMask`, `BitCount`, and `MaximumLength` |
| `SetFlagBit()` | §2.4.1.3.18 | Sets a single bit in the token flag byte |

### Decompression — [MS-OVBA] 2.4.1.3.1 – 2.4.1.3.5

| Function | Spec section | Description |
|----------|-------------|-------------|
| `DecompressingACompressedChunk()` | §2.4.1.3.2 | Reads the chunk header, dispatches to raw or token-sequence path |
| `DecompressingARawChunk()` | §2.4.1.3.3 | Copies 4 096 bytes verbatim from the compressed stream |
| `DecompressingATokenSequence()` | §2.4.1.3.4 | Reads one flag byte, then processes each of the 8 tokens it describes |
| `DecompressingAToken()` | §2.4.1.3.5 | Emits a literal byte or performs a back-reference copy |
| `UnpackCopyToken()` | §2.4.1.3.19.2 | Decodes a 2-byte copy token into `(Offset, Length)` |
| `ExtractCompressedChunkSize()` | §2.4.1.3.12 | Reads the 12-bit size field from the chunk header |
| `ExtractCompressedChunkFlag()` | §2.4.1.3.15 | Reads the compressed/raw flag from the chunk header |
| `ExtractFlagBit()` | §2.4.1.3.17 | Reads a single token flag bit |
| `ByteCopy()` | §2.4.1.3.11 | Byte-by-byte copy (handles overlapping back-references correctly) |

---

## Test Data

`structures.h` embeds the example data from the specification as well as real-world streams extracted from an Excel VBA project (`vbaProject.bin`):

| Variable | Source | Description |
|----------|--------|-------------|
| `pDecompressedInput_3_2_1` / `testCompressedData_3_2_1` | [MS-OVBA] §3.2.1 | No-compression example (`abcdefghijklmnopqrstuv.`) |
| `pDecompressedInput_3_2_2` / `testCompressedData_3_2_2` | [MS-OVBA] §3.2.2 | Normal compression example |
| `pDecompressedInput_3_2_3` / `testCompressedData_3_2_3` | [MS-OVBA] §3.2.3 | Maximum compression example (all identical bytes) |
| `vba_dir_Stream_Compressed` | Real-world | Compressed `dir` stream from an Excel VBA project (offset 0x3400, 1 492 bytes) |
| `Alerts_Stream_Compressed` | Real-world | Compressed module stream from the same VBA project (offset 0xE200, 8 272 bytes) |
| `Module1_stream` | Real-world | Compressed module stream used as the default decompression test input |

The `main()` function runs the **compression** algorithm on `pDecompressedInput_3_2_2` (§3.2.2) and then runs the **decompression** algorithm on `Module1_stream`, exercising both paths end-to-end.

---

## Building

The project ships a Visual Studio solution (`compvba.sln`) and project file (`compvba.vcxproj`) targeting Windows. Open the solution in Visual Studio 2019 or later and build normally (Debug or Release, x64 or x86).

```
compvba.sln
compvba.vcxproj
compvba.cpp       ← algorithm implementation + main()
structures.h      ← data structures, state variables, and test data
```

---

## References

- [[MS-OVBA]: Office VBA File Format Structure](https://learn.microsoft.com/en-us/openspecs/office_file_formats/ms-ovba/575462ba-bf67-4190-9fac-c275523c75fc) — Microsoft Open Specification
  - §2.4.1 — Compression and Decompression
  - §3.2 — Compression Examples

---

## Reporting Issues

Bug reports and questions are welcome. To help diagnose a problem quickly, please include **all** of the following in your issue:

### 1. Environment
- Operating system and version (e.g. Windows 11 22H2)
- Compiler and version (e.g. Visual Studio 2022 17.9, MSVC 19.39)
- Build configuration (Debug / Release, x86 / x64)

### 2. Complete reproduction steps
Provide the **exact, numbered steps** needed to reproduce the problem from a clean build:

```
1. Open compvba.sln in Visual Studio 2022.
2. Build the solution in Debug x64.
3. Run compvba.exe from the command line with no arguments.
4. Observe: ...
```

Vague descriptions such as *"it crashes"* or *"the output is wrong"* are very hard to act on. Please include:
- The exact command or code path exercised
- The **actual** output or error message (paste the full text, do not paraphrase)
- The **expected** output, referencing the relevant [MS-OVBA] section if applicable

### 3. Sample document (if applicable)
If the issue involves a specific Office file (`.xlsm`, `.docm`, `.pptm`, etc.) or a raw VBA binary stream:

- **Attach the file** to the GitHub issue (drag-and-drop or use the file picker).
- If the file contains sensitive information, please sanitize it first — for example, remove personally identifiable data or proprietary macro code while keeping the binary structure intact. Tools such as [oletools](https://github.com/decalage2/oletools) can help extract the raw VBA streams.
- Specify the **exact stream name and byte offset** within the file where the problematic compressed data begins (e.g. `xl/vbaProject.bin`, stream `dir`, offset `0x3400`).
- If possible, also provide the **expected decompressed output** so the correct behavior can be verified against [MS-OVBA] §3.2.

> **Note:** Issues filed without reproduction steps or a sample document may be closed as *needs more information* until the required details are provided.
