#include "file_system.h"

void listDir(fs::FS &fs, const char *dirname, uint8_t depth)
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
            if (depth)
            {
                listDir(fs, file.path(), depth - 1);
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

uint16_t fileCount(fs::FS &fs, const char *dirname, uint8_t depth)
{
    uint16_t count = 0;
    FS_Serial.printf("file Count: %s\n", dirname);

    File root = fs.open(dirname);
    if (!root)
    {
        FS_Serial.println("Failed to open directory");
        root.close();
        return 0;
    }
    if (!root.isDirectory())
    {
        FS_Serial.println("Not a directory");
        root.close();
        return 0;
    }

    File file = root.openNextFile();
    while (file)
    {
        if (!file.isDirectory())
        {
            // FS_Serial.println(file.name());
            count++;
        }
        else
        {
            if (depth)
            {
                count += fileCount(fs, file.path(), depth - 1);
            }
        }
        file = root.openNextFile();
    }
    FS_Serial.printf("%d files found in: %s\n", count, dirname);
    root.close();
    return count;
}

uint16_t deleteNFiles(fs::FS &fs, const char *dirname, uint16_t n, uint8_t depth)
{
    FS_Serial.printf("delete %d files in: %s(depth=%d)\n", n, dirname, depth);
    // uint16_t nameLen = 50;
    // char fname[nameLen];

    char fname[50];
    uint16_t count = 0;

    File root = fs.open(dirname);
    if (!root)
    {
        FS_Serial.println("Failed to open directory");
        root.close();
    }
    if (!root.isDirectory())
    {
        FS_Serial.println("Not a directory");
        root.close();
    }

    File file = root.openNextFile();
    while (file)
    {
        if (!file.isDirectory())
        {
            sprintf(fname, "%s/%s", dirname, file.name());
            FS_Serial.printf("delete file %s --- ", fname);
            if (fs.remove(fname))
            {
                n--;
                count++;
                FS_Serial.println("ok");
            }
            else
            {
                FS_Serial.println("fail");
            }
            if (!n)
            {

                break;
            }
        }
        else
        {
            if (depth)
            {
                count += deleteNFiles(fs, file.path(), n, depth - 1);
                n -= count;
            }
        }

        file = root.openNextFile();
    }

    root.close();

    FS_Serial.printf("%d files has been delete\n", count);
    return count;
}

// void findOldestFile(fs::FS &fs, const char *dirname, char *oldestFile, size_t len)
// {
//     FS_Serial.printf("find oldeset file : %s\n", dirname);

//     File root = fs.open(dirname);

//     File file = root.openNextFile();
//     while (file)
//     {
//         if (!file.isDirectory())
//         {
//             strlcpy(oldestFile, file.name(), len);
//             break;
//         }
//         file = root.openNextFile();
//     }

//     file = root.openNextFile();
//     while (file)
//     {
//         if (!file.isDirectory())
//         {
//             if (strcmp(oldestFile, file.name()) > 0)
//             {
//                 strlcpy(oldestFile, file.name(), len);
//             }
//         }
//         file = root.openNextFile();
//     }

//     FS_Serial.printf("oldestFile:%s\n", oldestFile);
//     root.close();
// }
