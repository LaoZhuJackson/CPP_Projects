#include <cstring>
#include "FileUtil.h"
//以追加模式打开一个文件，并且设置其执行时关闭
FileUtil::FileUtil(std::string& file_name):file_(::fopen(file_name.c_str(),"ae")), writtenBytes_(0){
    // 使用 setbuffer 为该文件流设置一个更大的空间缓冲区（默认的只有几KB），减少系统io的次数
    ::setbuffer(file_,buffer_,sizeof(buffer_));
}
FileUtil::~FileUtil(){
    if(file_) ::fclose(file_);
}

//向文件写入数据
void FileUtil::append(const char* data,size_t len){
    size_t writen = 0;
    while(writen != len){
        size_t remain = len - writen; //剩余需要写入的长度
        size_t n = write(data+writen,remain);//尝试从第一个参数指定的地址开始，写入第二个参数指定长度的数据
        // 如果剩余空间不足直接写入剩余字节
        if (n != remain){
            //错误判断
            int err = ferror(file_);//ferror(FILE*) 是C标准库函数，用于检查文件流 (file_) 是否设置了错误标志
            if(err){
                fprintf(stderr,"AppendFile::append() failed %s\n", strerror(err));//strerror(err): 将错误码转换为可读的字符串
                clearerr(file_);//  清除文件指针的错误标志
                break;
            }else if (errno == EINTR) { // 如果是被信号中断
                continue; // 不要break，继续尝试写
            }
        }
        writen += n;
    }
    writtenBytes_ += writen;
}

void FileUtil::flush(){
    ::fflush(file_);
}
//write逻辑
size_t FileUtil::write(const char* data,size_t len){
    // 没有选择线程安全的fwrite()为性能考虑。
   return  ::fwrite_unlocked(data, 1, len, file_);//每个数据项的大小（以字节为单位）。这里设置为1字节，是最常见的用法
}