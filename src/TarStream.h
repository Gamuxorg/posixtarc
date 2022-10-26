#include "tar.h"
#include <stdio.h>
#include <stdbool.h>

#define BLOCKSIZE 1024
#define PATH_MAX 4096

typedef void (*OutputCallback)(char *data, size_t size);

// 解包相关函数
void open(const char* filename, size_t offset);
bool unpack(const char* outputDir, const char* filterPath);

// 打包相关函数
void addFile(const char* src, const char* dist);
void setOutputCallback(OutputCallback callback);

void getFileHeader(const char* filename, struct posix_header* header);

/*
Tar数据输出的回调函数，您需要通过设置此回调函数来获得
输出的Tar数据，其中data为指向此数据的指针，您需要使用
size变量得到输出的数据。
*/
OutputCallback m_outputStream;
FILE *m_file;
void (*m_endOfStream)(void);
size_t m_offset;
