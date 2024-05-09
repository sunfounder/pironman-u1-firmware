#include "file_system.h"

#define ENABLE_PRINT 0
#if ENABLE_PRINT
#define FS_Serial Serial
#else
#define FS_Serial Serial0
#endif

void listDir(fs::FS &fs, const char *dirname, uint8_t levels)
{
    FS_Serial.printf("Listing directory: %s\n", dirname);

    File root = fs.open(dirname);
    if (!root)
    {
        FS_Serial.println("Failed to open directory");
        return;
    }
    if (!root.isDirectory())
    {
        FS_Serial.println("Not a directory");
        return;
    }

    File file = root.openNextFile();
    while (file)
    {
        if (file.isDirectory())
        {
            FS_Serial.print("  DIR : ");
            FS_Serial.println(file.name());
            if (levels)
            {
                listDir(fs, file.path(), levels - 1);
            }
        }
        else
        {
            FS_Serial.print("  FILE: ");
            FS_Serial.print(file.name());
            FS_Serial.print("  SIZE: ");
            FS_Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

void createDir(fs::FS &fs, const char *path)
{
    FS_Serial.printf("Creating Dir: %s\n", path);
    if (fs.mkdir(path))
    {
        FS_Serial.println("Dir created");
    }
    else
    {
        FS_Serial.println("mkdir failed");
    }
}

void removeDir(fs::FS &fs, const char *path)
{
    FS_Serial.printf("Removing Dir: %s\n", path);
    if (fs.rmdir(path))
    {
        FS_Serial.println("Dir removed");
    }
    else
    {
        FS_Serial.println("rmdir failed");
    }
}

void readFile(fs::FS &fs, const char *path, String *txtBuf, size_t *returnSize)
{
    char ch;
    size_t size;
    *txtBuf = ""; // clear String buffer

    FS_Serial.printf("Reading %s\n", path);

    File file = fs.open(path);
    if (!file)
    {
        FS_Serial.println("Failed to open file for reading");
        return;
    }
    size = file.size();
    *returnSize = size;
    FS_Serial.printf("size(%d):\n", size);

    while (file.available())
    {
        // FS_Serial.write(file.read());

        ch = char(file.read());
        FS_Serial.write(ch);
        *txtBuf = *txtBuf + ch;
    }
    file.close();
    FS_Serial.println(""); // new line
}

void writeFile(fs::FS &fs, const char *path, const char *message)
{
    FS_Serial.printf("Writing file: %s\n", path);

    File file = fs.open(path, FILE_WRITE);
    if (!file)
    {
        FS_Serial.println("Failed to open file for writing");
        return;
    }
    if (file.print(message))
    {
        FS_Serial.println("File written");
    }
    else
    {
        FS_Serial.println("Write failed");
    }
    file.close();
}

void appendFile(fs::FS &fs, const char *path, const char *message)
{
    FS_Serial.printf("Appending to file: %s\n", path);

    File file = fs.open(path, FILE_APPEND);
    if (!file)
    {
        FS_Serial.println("Failed to open file for appending");
        return;
    }
    if (file.print(message))
    {
        FS_Serial.println("Message appended");
    }
    else
    {
        FS_Serial.println("Append failed");
    }
    file.close();
}

void renameFile(fs::FS &fs, const char *path1, const char *path2)
{
    FS_Serial.printf("Renaming file %s to %s\n", path1, path2);
    if (fs.rename(path1, path2))
    {
        FS_Serial.println("File renamed");
    }
    else
    {
        FS_Serial.println("Rename failed");
    }
}

void deleteFile(fs::FS &fs, const char *path)
{
    FS_Serial.printf("Deleting file: %s\n", path);
    if (fs.remove(path))
    {
        FS_Serial.println("File deleted");
    }
    else
    {
        FS_Serial.println("Delete failed");
    }
}

void testFileIO(fs::FS &fs, const char *path)
{
    File file = fs.open(path);
    static uint8_t buf[512];
    size_t len = 0;
    uint32_t start = millis();
    uint32_t end = start;
    if (file)
    {
        len = file.size();
        size_t flen = len;
        start = millis();
        while (len)
        {
            size_t toRead = len;
            if (toRead > 512)
            {
                toRead = 512;
            }
            file.read(buf, toRead);
            len -= toRead;
        }
        end = millis() - start;
        FS_Serial.printf("%u bytes read for %u ms\n", flen, end);
        file.close();
    }
    else
    {
        FS_Serial.println("Failed to open file for reading");
    }

    file = fs.open(path, FILE_WRITE);
    if (!file)
    {
        FS_Serial.println("Failed to open file for writing");
        return;
    }

    size_t i;
    start = millis();
    for (i = 0; i < 2048; i++)
    {
        file.write(buf, 512);
    }
    end = millis() - start;
    FS_Serial.printf("%u bytes written for %u ms\n", 2048 * 512, end);
    file.close();
}