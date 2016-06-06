#include "fs.h"
#include "draw.h"
#include "hashfile.h"

#define HBUFFER_ADDRESS  ((u8*) 0x21800000)
#define HBUFFER_MAX_SIZE ((u32) (8 * 1024 * 1024))

void sha_init(u32 mode)
{
    while(*REG_SHACNT & 1);
    *REG_SHACNT = mode | SHA_CNT_OUTPUT_ENDIAN | SHA_NORMAL_ROUND;
}

void sha_update(const void* src, u32 size)
{    
    const u32* src32 = (const u32*)src;
    
    while(size >= 0x40) {
        while(*REG_SHACNT & 1);
        for(u32 i = 0; i < 4; i++) {
            *REG_SHAINFIFO = *src32++;
            *REG_SHAINFIFO = *src32++;
            *REG_SHAINFIFO = *src32++;
            *REG_SHAINFIFO = *src32++;
        }
        size -= 0x40;
    }
    while(*REG_SHACNT & 1);
    memcpy((void*)REG_SHAINFIFO, src32, size);
}

void sha_get(void* res) {
    *REG_SHACNT = (*REG_SHACNT & ~SHA_NORMAL_ROUND) | SHA_FINAL_ROUND;
    while(*REG_SHACNT & SHA_FINAL_ROUND);
    while(*REG_SHACNT & 1);
    memcpy(res, (void*)REG_SHAHASH, (256 / 8));
}

void sha_quick(void* res, const void* src, u32 size, u32 mode) {
    sha_init(mode);
    sha_update(src, size);
    sha_get(res);
}

u32 GetHashFromFile(const char* filename, u32 offset, u32 size, u8* hash)
{
    // uses the standard buffer, so be careful
    u8* buffer = HBUFFER_ADDRESS;
    
    if (!FileOpen(filename))
        return 1;
    if (!size) {
        size = FileGetSize();
        if (offset >= size)
            return 1;
        size -= offset;
    }
    sha_init(SHA256_MODE);
    for (u32 i = 0; i < size; i += HBUFFER_MAX_SIZE) {
        u32 read_bytes = min(HBUFFER_MAX_SIZE, (size - i));
        if (size >= 0x100000) ShowProgress(i, size);
        if(!FileRead(buffer, read_bytes, offset + i)) {
            FileClose();
            return 1;
        }
        sha_update(buffer, read_bytes);
    }
    sha_get(hash);
    ShowProgress(0, 0);
    FileClose();
    
    return 0;
}

u32 CheckHashFromFile(const char* filename, u32 offset, u32 size, const u8* hash)
{
    u8 digest[32];
    
    if (GetHashFromFile(filename, offset, size, digest) != 0)
        return 1;
    
    return (memcmp(hash, digest, 32) == 0) ? HASH_VERIFIED : HASH_FAILED; 
}

u32 HashVerifyFile(const char* filename)
{
    char hashname[64];
    u8 hash[32];
    
    snprintf(hashname, 64, "%s.sha", filename);
    if (FileGetData(hashname, hash, 32, 0) != 32)
        return HASH_NOT_FOUND;
    
    return CheckHashFromFile(filename, 0, 0, hash);
}
