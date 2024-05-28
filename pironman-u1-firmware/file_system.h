#pragma once

#include "FS.h"

#define ENABLE_FS_PRINT 0
#if ENABLE_FS_PRINT
#define FS_Serial Serial
#else
#define FS_Serial Serial0
#endif

void listDir(fs::FS &fs, const char *dirname, uint8_t depth);
void createDir(fs::FS &fs, const char *path);
void removeDir(fs::FS &fs, const char *path);
void readFile(fs::FS &fs, const char *path, String *txtBuf, size_t *returnSize);
void writeFile(fs::FS &fs, const char *path, const char *message);
void appendFile(fs::FS &fs, const char *path, const char *message);
void renameFile(fs::FS &fs, const char *path1, const char *path2);
void deleteFile(fs::FS &fs, const char *path);
void testFileIO(fs::FS &fs, const char *path);
uint16_t fileCount(fs::FS &fs, const char *dirname, uint8_t depth);
// void findOldestFile(fs::FS &fs, const char *dirname, char *oldestFile, size_t len);
uint16_t deleteNFiles(fs::FS &fs, const char *dirname, uint16_t n, uint8_t depth);