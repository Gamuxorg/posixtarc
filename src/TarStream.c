#include "TarStream.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

void setOutputCallback(OutputCallback callback){
    m_outputStream = callback;
}

void getFileHeader(const char* filename, struct posix_header* header){
  FILE *file = fopen(filename, "r");

    char *header_data = (char *)header;
    memset(header_data, 0, 512);
    // filename
    strcpy(header->name, filename);

    // mode
    struct stat* _stat;
    stat(filename, _stat);
    mode_t mode = _stat->st_mode;
    sprintf(header->mode, "%o", mode);

    // gid
    sprintf(header->gid, "%o",  _stat->st_gid);

    // uid
    sprintf(header->uid, "%o", _stat->st_uid);

    // mtime
    sprintf(header->mtime, "%ld", _stat->st_mtim.tv_sec);

    // typeflag
    char typeflag;
    if (S_ISREG(mode)){
        typeflag = REGTYPE;
    } else if (S_ISDIR(mode)){
        typeflag = DIRTYPE;
    } else if (S_ISLNK(mode)){
        typeflag = SYMTYPE;
    }
    header->typeflag = typeflag;

    // magic
    strcpy(header->magic, TMAGIC);

    // symlink
    if (typeflag == SYMTYPE){
        char target[FILENAME_MAX];
        ssize_t res = readlink(filename, header->linkname, FILENAME_MAX);
        if (res <= 0){
            printf("Error: Fail to add symlink!\n");
        }
    }

    // size
    fseek(file, 0, SEEK_END);
    sprintf(header->size, "%lo", ftell(file));
    fseek(file, 0, SEEK_SET);

    // chksum
    unsigned int chksum = 0;
    char *ptr = header_data;
    char buf_num[9];
    int num;
    memset(buf_num, 0, 9);
    for (int i = 0; i < 64; ++i) {
        memcpy(buf_num, ptr, 8);
        sscanf(buf_num, "%d", num);
        chksum += num;
    }
    sprintf(header->chksum, "%d", chksum);

}

void addFile(const char* src, const char* dist){
    FILE *file;
    struct posix_header* header;
    char buf[BLOCKSIZE];
    getFileHeader(src, header);
    memset(header->name, 0, 100);
    strcpy(header->name, dist);
    m_outputStream((char*)header, BLOCKSIZE);
    file = fopen(src, "r");
    printf("dest: %s", header->name);
    fseek(file, 0, SEEK_SET);
    while (true){
        memset(buf, 0, BLOCKSIZE);
        size_t read_size = fread(buf, 1, BLOCKSIZE, file);
        if (read_size <= 0){
            break;
        }
        m_outputStream(buf, BLOCKSIZE);
    }
}

void open(const char* filename, size_t offset){
    m_file = fopen(filename, "w+");
    m_offset = offset;
}

bool unpack(const char* outputDir, const char* filterPath){
    if (!m_file) return false;
    
    const int block_size = 512;
    unsigned char buf[block_size];
    struct posix_header* header = (struct posix_header*)buf;
    memset(buf, 0, block_size);
    char longFilename[PATH_MAX];
    char targetfn[PATH_MAX];

    size_t pos = (unsigned)m_offset;
    size_t now = 0;
    fseek(m_file, m_offset, SEEK_SET);
    while (true) {
        char filename[100];
        char prefix[155];
        size_t file_size = 0;
        mode_t mode;

        size_t read_size = fread(buf, block_size, 1, m_file);
        if (read_size != 1) break;
        if (strncmp(header->magic, TMAGIC, 5) != 0) break;
        pos += block_size;

        sscanf(header->size, "%lo", file_size);
        size_t file_block_count = (file_size + block_size - 1) / block_size;
        sscanf(header->mode, "%o", mode);
        memcpy(filename, header->name, 100);
        memcpy(prefix, header->prefix,155);
        memset(targetfn, 0, PATH_MAX);
        strcpy(targetfn,outputDir);
        strcat(targetfn,"/");

        char* fname;
        if (longFilename[0] != 0) {
            fname = longFilename;
            strcat(targetfn,&longFilename[strlen(filterPath)]);
            memset(longFilename, 0, PATH_MAX);
        } else {
            strcpy(fname, (const char*)prefix);
            strcat(fname, (const char*)filename);
            char name[PATH_MAX];
            memset(name,0,PATH_MAX);
            strcat(name,prefix);
            strcat(name,filename);
            strcat(targetfn,&name[strlen(filterPath)]);
        }

        FILE *outFile;
        char* content;
        char* deststr = "";
        switch (header->typeflag) {
            case REGTYPE: // intentionally dropping through
            case AREGTYPE:
                if (strncpy( deststr, fname, strlen(filterPath) ) != filterPath) break;
                // normal file
                now++;
                // 发射信号，传递进度信息
                outFile = fopen(targetfn , "w");
                if (feof(outFile) == 1) {
                    break;
                }
                content = (char*) malloc((int)file_size);
                fread(content, file_size,1,m_file);
                fwrite(content,file_size,1,outFile);
                free(content);
                fflush(outFile);
                fclose(outFile);
                chmod(targetfn, mode);
                break;
            case SYMTYPE:
                // symbolic link
                if (strncpy( deststr, fname, strlen(filterPath) ) != filterPath) break;
                symlink(header->linkname,targetfn);
                break;
            case DIRTYPE:
                if (strncpy( deststr, fname, strlen(filterPath) ) != filterPath) break;
                mkdir(targetfn, 0755);
                // directory
                break;
            case GNUTYPE_LONGNAME:
                fread(longFilename, file_block_count * block_size,1,m_file);
                break;
            default:
                printf("%s %s", targetfn, header->typeflag);
                break;
        }
        pos += file_block_count * block_size;
        fseek(m_file, pos, SEEK_SET);
    }

    fseek(m_file, m_offset, SEEK_SET);
    return true;
}