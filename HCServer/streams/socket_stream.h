#ifndef __HCSERVER_SOCKET_STREAM_H__
#define __HCSERVER_SOCKET_STREAM_H__

#include"../stream.h"
#include"../socket.h"
#include<memory>

namespace HCServer{

class SocketStream:public Stream{
public:
    typedef shared_ptr<SocketStream> ptr;

    //构造函数  sock Socket类   owner sock是否完全交给当时的类控制  
    SocketStream(Socket::ptr sock, bool owner = true);
    ~SocketStream();

    //读数据    buffer 存储接收到数据的内存 length 接收数据的内存大小    
    //return:   >0 返回接收到的数据的实际大小   =0 被关闭   <0 出现流错误 
    virtual int read(void* buffer, size_t length) override;

    //读数据    ba 存储接收到数据的ByteArray内存池  length 接收数据的内存大小
    //return: >0 返回接收到的数据的实际大小   =0 被关闭   <0 出现流错误
    virtual int read(HCServer::ByteArray::ptr ba, size_t length) override;

    //写数据    buffer 写数据的内存 length 写入数据的内存大小
    //return:>0 返回接收到的数据的实际大小  =0 被关闭   <0 出现流错误
    virtual int write(const void* buffer, size_t length) override;

    //写数据    ba 写数据的ByteArray    length 写入数据的内存大小
    //return:>0 返回接收到的数据的实际大小  =0 被关闭   <0 出现流错误
    virtual int write(ByteArray::ptr ba, size_t length) override;

    virtual void close() override;//关闭socket

    Socket::ptr getSocket() const { return m_socket;}   //返回Socket类

    bool isConnected() const;//返回是否连接

    Address::ptr getRemoteAddress();    //获取远端地址(本机：服务器——客户器地址     本机：客户器——服务器地址)
        
    Address::ptr getLocalAddress(); //获取本地地址(本机：服务器——服务器地址     本机：客户器——客户器地址)
        
    string getRemoteAddressString();   //返回远端地址的可读性字符串
        
    string getLocalAddressString();    //返回本地地址的可读性字符串

protected: 
    Socket::ptr m_socket;
    bool m_owner;
};



}


#endif