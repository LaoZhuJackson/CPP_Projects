#pragma once
#include <string>
#include <stdio.h>
#include <sys/types.h>//off_t
/**
 * @brief 文件工具类，用于处理文件的写入操作
 * 该类封装了对文件的基本操作，包括写入数据和刷新缓冲区
 */
class FileUtil{
public:
    FileUtil(std::string& file_name);
    ~FileUtil();

    void append(const char* data,size_t len);
    void flush();//刷新文件缓冲区,将缓冲区中的数据立即写入文件

    /**
     * @brief 获取已写入的字节数
     * @return 返回已写入文件的总字节数
     */
    off_t writtenBytes() const {return writtenBytes_;}
private:
    size_t write(const char* data, size_t len);  // 实际写入逻辑
    FILE* file_; // 文件指针，用于操作文件
    char buffer_[64*1024];       // 文件操作的缓冲区，大小为64KB，用于提高写入效率
    off_t writtenBytes_;        // 记录已写入文件的总字节数，off_t类型用于大文件支持
};