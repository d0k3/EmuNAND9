#pragma once

#include "common.h"

#define WORK_DIRS   "/files9", "/EmuNAND9"

bool InitFS();
void DeinitFS();
bool CheckFS();

/** Work directory handling **/
const char* GetWorkDir();

/** Checks if there is enough space free on the SD card **/
bool DebugCheckFreeSpace(size_t required);

/** Opens existing files */
bool FileOpen(const char* path);
bool DebugFileOpen(const char* path);

/** Opens new files (and creates them if they don't already exist) */
bool FileCreate(const char* path, bool truncate);
bool DebugFileCreate(const char* path, bool truncate);

/** Reads contents of the opened file */
size_t FileRead(void* buf, size_t size, size_t foffset);
bool DebugFileRead(void* buf, size_t size, size_t foffset);

/** Writes to the opened file */
size_t FileWrite(void* buf, size_t size, size_t foffset);
bool DebugFileWrite(void* buf, size_t size, size_t foffset);

/** Gets the size of the opened file */
size_t FileGetSize();

/** Creates a directory */
bool DirMake(const char* path);
bool DebugDirMake(const char* path);

/** Opens an existing directory */
bool DirOpen(const char* path);
bool DebugDirOpen(const char* path);

/** Reads next file name to fname from opened directory,
    returns false if all files in directory are processed.
    fname needs to be allocated to fsize bytes minimum. */
bool DirRead(char* fname, int fsize);

/** Get list of files under a given path **/
bool GetFileList(const char* path, char* list, int lsize, bool recursive);

/** Quickly opens a secondary file, gets some data, and closes it again **/
size_t FileGetData(const char* path, void* buf, size_t size, size_t foffset);

/** Quickly opens a secondary file, dumps some data, and closes it again **/
size_t FileDumpData(const char* path, void* buf, size_t size);

/** Formats the first (and only) partition using label as name **/
bool PartitionFormat(const char* label);

/** Gets remaining space on SD card in bytes */
uint64_t RemainingStorageSpace();

/** Gets total space on SD card in bytes */
uint64_t TotalStorageSpace();

void FileClose();
void DirClose();
