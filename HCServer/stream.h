/*
*Stream流模块
*提供read、wirte、readFixSize和writeFixSize；
*缓冲区可以是buffer或者内存池结构
*还有关闭流的功能
*/


#ifndef __HCSERVER_STREAM_H__
#define __HCSERVER_STREAM_H__

#include <memory>
#include "bytearray.h"

namespace HCServer
{

//流结构
class Stream
{
public:
    typedef std::shared_ptr<Stream> ptr;

    virtual ~Stream() {}

    //读数据    buffer 存储接收到数据的内存 length 接收数据的内存大小    
    //return:   >0 返回接收到的数据的实际大小   =0 被关闭   <0 出现流错误    
    virtual int read(void* buffer, size_t length) = 0;

    //读数据    ba 存储接收到数据的ByteArray内存池  length 接收数据的内存大小
    //return: >0 返回接收到的数据的实际大小   =0 被关闭   <0 出现流错误
    virtual int read(HCServer::ByteArray::ptr ba, size_t length) = 0;

    //读固定长度的数据  buffer 存储接收到数据的内存 length 接收数据的内存大小
    //return:>0 返回接收到的数据的实际大小  =0 被关闭   <0 出现流错误
    virtual int readFixSize(void* buffer, size_t length);

    //读固定长度的数据  ba 存储接收到数据的ByteArray    length 接收数据的内存大小
    //return:>0 返回接收到的数据的实际大小  =0 被关闭   <0 出现流错误
    virtual int readFixSize(ByteArray::ptr ba, size_t length);

    //写数据    buffer 写数据的内存 length 写入数据的内存大小
    //return:>0 返回接收到的数据的实际大小  =0 被关闭   <0 出现流错误
    virtual int write(const void* buffer, size_t length) = 0;

    //写数据    ba 写数据的ByteArray    length 写入数据的内存大小
    //return:>0 返回接收到的数据的实际大小  =0 被关闭   <0 出现流错误
    virtual int write(ByteArray::ptr ba, size_t length) = 0;

    //写固定长度的数据  buffer 写数据的内存 length 写入数据的内存大小
    //return:>0 返回接收到的数据的实际大小  =0 被关闭   <0 出现流错误
    virtual int writeFixSize(const void* buffer, size_t length);

    //写固定长度的数据  ba 写数据的ByteArray    length 写入数据的内存大小
    // return:>0 返回接收到的数据的实际大小  =0 被关闭   <0 出现流错误
    virtual int writeFixSize(ByteArray::ptr ba, size_t length);

    //关闭流
    virtual void close() = 0;
};

}

#endif