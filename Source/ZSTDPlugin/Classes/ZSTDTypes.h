////////////////////////////////////////////////////////////////////////////////
//
// MIT License
// 
// Copyright (c) 2018-2019 Nuraga Wiswakarma
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
////////////////////////////////////////////////////////////////////////////////
//

#pragma once

#include "zstd.h"
#include "UnrealMemory.h"
#include "SharedPointer.h"

typedef TSharedPtr<struct FZSTDBufferData> TPSZSTDBufferData;
typedef TSharedPtr<struct FZSTDCCtx> TPSZSTDCCtx;
typedef TSharedPtr<struct FZSTDDCtx> TPSZSTDDCtx;
typedef unsigned long long FZSTDDecSize;

struct FZSTDBufferData
{
    void* Buffer;
    SIZE_T BufferSize;

    ~FZSTDBufferData()
    {
        Reset();
    }

    FORCEINLINE void Reset()
    {
        if (Buffer)
        {
            FMemory::Free(Buffer);
            Buffer = nullptr;
        }
        BufferSize = 0;
    }

    void Move(TArray<uint8>& OutData)
    {
        check(Buffer);
        // Copy buffer
        OutData.SetNumZeroed(BufferSize);
        FMemory::Memcpy(OutData.GetData(), Buffer, BufferSize);
        // Reset buffer
        Reset();
    }

private:
    FZSTDBufferData() = default;
    FZSTDBufferData& operator=(FZSTDBufferData& rhs) = default;

    FZSTDBufferData(void* b, SIZE_T s)
        : Buffer(b)
        , BufferSize(s)
    {
    }

    friend class FZSTDUtils;
};

struct FZSTDCCtx
{
    ~FZSTDCCtx()
    {
        if (CCtx)
            ZSTD_freeCCtx(CCtx);
    }

    ZSTD_CCtx& operator*()
    {
        return *CCtx;
    }

private:

    ZSTD_CCtx* CCtx = nullptr;

    FZSTDCCtx() = default;
    FZSTDCCtx& operator=(FZSTDCCtx& rhs) = default;

    FZSTDCCtx(ZSTD_CCtx* cctx) : CCtx( cctx )
    {
    }

    friend class FZSTDUtils;
};

struct FZSTDDCtx
{
    ~FZSTDDCtx()
    {
        if (DCtx)
            ZSTD_freeDCtx(DCtx);
    }

    ZSTD_DCtx& operator*()
    {
        return *DCtx;
    }

private:

    ZSTD_DCtx* DCtx = nullptr;

    FZSTDDCtx() = default;
    FZSTDDCtx& operator=(FZSTDDCtx& rhs) = default;

    FZSTDDCtx(ZSTD_DCtx* cctx) : DCtx( cctx )
    {
    }

    friend class FZSTDUtils;
};

class FZSTDUtils
{

    FORCEINLINE static void* alloc_fn(void* memctx, size_t size)
    {
        (void) memctx;
        return FMemory::Malloc(size);
    }

    FORCEINLINE static void free_fn(void* memctx, void* p)
    {
        (void) memctx;
        FMemory::Free(p);
    }

public:

    static const int32 DEFAULT_COMPRESSION_LEVEL = 3;

    static int32 GetMaxCompressionLevel()
    {
        return ZSTD_maxCLevel();
    }

    FORCEINLINE static TPSZSTDCCtx CreateCCtx()
    {
        return MakeShareable( new FZSTDCCtx( ZSTD_createCCtx_advanced({ alloc_fn, free_fn, nullptr }) ) );
    }

    FORCEINLINE static TPSZSTDDCtx CreateDCtx()
    {
        return MakeShareable( new FZSTDDCtx( ZSTD_createDCtx_advanced({ alloc_fn, free_fn, nullptr }) ) );
    }

    FORCEINLINE static TPSZSTDBufferData CompressData(const void* SrcBuff, const SIZE_T SrcBuffSize, int32 CompressionLevel = DEFAULT_COMPRESSION_LEVEL)
    {
        // Return compressed data if data source is valid
        return SrcBuff
            ? CompressData(MoveTemp(CreateCCtx()), SrcBuff, SrcBuffSize, CompressionLevel)
            : TPSZSTDBufferData();
    }

    static TPSZSTDBufferData CompressData(TPSZSTDCCtx CCtx, const void* SrcBuff, const SIZE_T SrcBuffSize, int32 CompressionLevel = DEFAULT_COMPRESSION_LEVEL)
    {
        if (CCtx.IsValid() && SrcBuff)
        {
            // Calculate compression bound size and allocate compressed data buffer
            SIZE_T dstBuffSize = ZSTD_compressBound(SrcBuffSize);
            void* dstBuff = FMemory::Malloc(dstBuffSize);

            // Compress data
            SIZE_T compressedSize = CompressData(CCtx, SrcBuff, SrcBuffSize, dstBuff, dstBuffSize, CompressionLevel, true);

            // Store compression buffer result to a shared pointer
            TPSZSTDBufferData bufferData = MakeShareable( new FZSTDBufferData(dstBuff, compressedSize) );

            return MoveTemp( bufferData );
        }

        return TPSZSTDBufferData();
    }

    FORCEINLINE static SIZE_T CompressData(TPSZSTDCCtx CCtx, const void* SrcBuff, const SIZE_T SrcBuffSize, void*& DstBuff, const SIZE_T DstBuffSize, int32 CompressionLevel = DEFAULT_COMPRESSION_LEVEL, bool bRealloc = false)
    {
        check(SrcBuff);
        check(DstBuff);
        check(CCtx.IsValid());
        SIZE_T compressedSize = CompressData(**CCtx.Get(), SrcBuff, SrcBuffSize, DstBuff, DstBuffSize, CompressionLevel);
        if (bRealloc)
            DstBuff = FMemory::Realloc(DstBuff, compressedSize);
        return compressedSize;
    }

    FORCEINLINE static TPSZSTDBufferData DecompressData(const void* SrcBuff, const SIZE_T SrcBuffSize)
    {
        return SrcBuff
            ? DecompressData(MoveTemp(CreateDCtx()), SrcBuff, SrcBuffSize)
            : TPSZSTDBufferData();
    }

    static TPSZSTDBufferData DecompressData(TPSZSTDDCtx DCtx, const void* SrcBuff, const SIZE_T SrcBuffSize)
    {
        if (DCtx.IsValid() && SrcBuff)
        {
            // Calculates decompression size, may return error code if supplied with invalid parameter
            const FZSTDDecSize typeSize = GetDecompressionSize(SrcBuff, SrcBuffSize);

            if (IsValidDecompressionSize(typeSize))
            {
                // Allocate decompression buffer
                SIZE_T dstSize = static_cast<SIZE_T>(typeSize);
                void* dstBuff = FMemory::Malloc(dstSize);

                // Decompress data
                SIZE_T decompressedSize = DecompressData(DCtx, SrcBuff, SrcBuffSize, dstBuff, dstSize, true);

                // Store decompression buffer result to a shared pointer
                TPSZSTDBufferData bufferData = MakeShareable( new FZSTDBufferData(dstBuff, decompressedSize) );

                // Store decompression buffer result to a shared pointer
                return MoveTemp( bufferData );
            }
        }

        return TPSZSTDBufferData();
    }

    FORCEINLINE static SIZE_T DecompressData(TPSZSTDDCtx DCtx, const void* SrcBuff, const SIZE_T SrcBuffSize, void*& DstBuff, const SIZE_T DstBuffSize, bool bRealloc = false)
    {
        check(SrcBuff);
        check(DstBuff);
        check(DCtx.IsValid());
        check(IsValidDecompressionSize(DstBuffSize));
        SIZE_T compressedSize = DecompressData(**DCtx.Get(), SrcBuff, SrcBuffSize, DstBuff, DstBuffSize);
        if (bRealloc && DstBuffSize != compressedSize)
            DstBuff = FMemory::Realloc(DstBuff, compressedSize);
        return compressedSize;
    }

    FORCEINLINE static SIZE_T GetCompressionSize(const SIZE_T SrcBuffSize)
    {
        return ZSTD_compressBound(SrcBuffSize);
    }

    FORCEINLINE static FZSTDDecSize GetDecompressionSize(const void* SrcBuff, const SIZE_T SrcBuffSize)
    {
        return ZSTD_findDecompressedSize(SrcBuff, SrcBuffSize);
    }

    FORCEINLINE static bool IsValidDecompressionSize(FZSTDDecSize SrcBuffSize)
    {
        return SrcBuffSize != ZSTD_CONTENTSIZE_ERROR && SrcBuffSize != ZSTD_CONTENTSIZE_UNKNOWN;
    }

private:

    FORCEINLINE static SIZE_T CompressData(ZSTD_CCtx& CCtx, const void* SrcBuff, const SIZE_T SrcBuffSize, void* DstBuff, const SIZE_T DstBuffSize, int32 CompressionLevel)
    {
        // Ensure configuration parameter validity
        check(SrcBuff);
        check(DstBuff);
        return ZSTD_compressCCtx(&CCtx, DstBuff, DstBuffSize, SrcBuff, SrcBuffSize, FMath::Clamp(CompressionLevel, 1, GetMaxCompressionLevel()));
    }

    FORCEINLINE static SIZE_T DecompressData(ZSTD_DCtx& DCtx, const void* SrcBuff, const SIZE_T SrcBuffSize, void* DstBuff, SIZE_T DstBuffSize)
    {
        // Ensure configuration parameter validity
        check(SrcBuff);
        check(DstBuff);
        check(IsValidDecompressionSize(DstBuffSize));
        return ZSTD_decompressDCtx(&DCtx, DstBuff, DstBuffSize, SrcBuff, SrcBuffSize);
    }

};
