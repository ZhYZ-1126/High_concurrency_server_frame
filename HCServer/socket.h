/* 
*socket模块
*创建一个socket，需要绑定某种类型（ipv4，ipv6等）的地址
*实现了和socket相关的操作，如bind，listen，accept，connect等
*/


#ifndef __HCSERVER_SOCKET_H__
#define __HCSERVER_SOCKET_H__

#include<memory>
#include "address.h"
#include"noncopyable.h"
#include<iostream>
using namespace std;

namespace HCServer{

class Socket:public enable_shared_from_this<Socket>,Noncopyable
{
public:
    typedef shared_ptr<Socket> ptr;
    typedef weak_ptr<Socket> Weak_ptr;//借助weak_ptr类型指针可以获取shared_ptr指针的一些状态信息(比如引用计数)

    Socket(int family, int type, int protocol = 0);
    virtual ~Socket();

    //socket的类型
    enum Type
    {
        TCP = SOCK_STREAM,  //TCP类型
        UDP = SOCK_DGRAM    //UDP类型
    };
    //socket协议族
    enum Family
    {
        IPv4 = AF_INET,     //IPv4 socket
        IPv6 = AF_INET6,    //IPv6 socket
        UNIX = AF_UNIX,     //Unix socket
    };

    
    static Socket::ptr CreateTCP(HCServer::Address::ptr address);// 创建TCP Socket(满足地址类型)
    
    static Socket::ptr CreateUDP(HCServer::Address::ptr address);// 创建UDP Socket(满足地址类型)

    static Socket::ptr CreateTCPSocket();   //创建IPv4的TCP Socket

    static Socket::ptr CreateUDPSocket();   //创建IPv4的UDP Socket

    static Socket::ptr CreateTCPSocket6();  //创建IPv6的TCP Socket

    static Socket::ptr CreateUDPSocket6();  //创建IPv6的UDP Socket

    static Socket::ptr CreateUnixTCPSocket();   //创建Unix的TCP Socket

    static Socket::ptr CreateUnixUDPSocket();   //创建Unix的UDP Socket


    int64_t getSendTimeout();   //获取发送超时时间(毫秒)

    void setSendTimeout(int64_t v); //设置发送超时时间(毫秒)    v选项的值

    int64_t getRecvTimeout();   //获取接受超时时间(毫秒)

    void setRecvTimeout(int64_t v); //设置接受超时时间(毫秒)    v选项的值

    bool getOption(int level,int option,void * result,socklen_t* len);//获取socket fd的信息

    //获取socket fd的信息的模板
    template<class T>
    bool getOption(int level, int option, T& result)
    {
        socklen_t length = sizeof(T);
        return getOption(level, option, &result, &length);
    }

    //设置fd的属性
    bool setOption(int level, int option, const void* result, socklen_t len);

    //设置fd的属性的模板
    template<class T>
    bool setOption(int level, int option, const T& value)
    {
        return setOption(level, option, &value, sizeof(T));
    }

    virtual Socket::ptr accept();//服务器接收connect(客户端)链接

    virtual bool bind(const Address::ptr addr);//服务器绑定IP地址

    virtual bool listen(int backlog = SOMAXCONN);//监听socket客户端连接

    virtual bool connect(const Address::ptr addr, uint64_t timeout_ms = -1);//客户端连接服务器地址

    virtual bool reconnect(uint64_t timeout_ms = -1);//客户端重新连接服务器地址

    virtual bool close();//关闭socket

    //发送数据(TCP)
    virtual int send(const void* buffer, size_t length, int flags = 0);
    virtual int send(const iovec* buffers, size_t length, int flags = 0);

    //发送数据(UDP)
    virtual int sendTo(const void* buffer, size_t length, const Address::ptr to, int flags = 0);
    virtual int sendTo(const iovec* buffers, size_t length, const Address::ptr to, int flags = 0);

    //接收数据（TCP）
    virtual int recv(void* buffer, size_t length, int flags = 0);
    virtual int recv(iovec* buffers, size_t length, int flags = 0);

    //接收数据（UDP）
    virtual int recvFrom(void* buffer, size_t length, Address::ptr from, int flags = 0);
    virtual int recvFrom(iovec* buffers, size_t length, Address::ptr from, int flags = 0);
        
    Address::ptr getRemoteAddress();    //获取远端地址(本机：服务器——客户器地址     本机：客户器——服务器地址)

    Address::ptr getLocalAddress(); //获取本地地址(本机：服务器——服务器地址     本机：客户器——客户器地址)

    int getSocket() const { return m_sock;} //返回socket句柄

    int getFamily() const { return m_family; }  //获取协议簇(IP协议)

    int getType() const { return m_type; }  //获取socket类型

    int getProtocol() const { return m_protocol; }  //获取协议

    bool isConnected() const { return m_isConnected; }  //返回是否连接成功

    bool isValid() const;   //m_sock是否有效(m_sock != -1)

    int getError(); //返回Socket的错误

    virtual ostream& dump(ostream& os) const; //输出socket基本信息到流中

    virtual string toString() const;   //socket基本信息字符串化

    bool cancelRead();  //取消读(唤醒执行)

    bool cancelWrite(); //取消写(唤醒执行)

    bool cancelAccept();    //取消accept(唤醒执行)

    bool cancelAll();   //取消所有事件(唤醒执行)

protected:
    void initSock();    //初始化socket(m_sock)

    void newSock(); //创建新的socket(赋予到m_sock)

    virtual bool init(int sock);//初始化sock(sock赋予到m_sock)



protected:
    int m_sock; //socket句柄
    int m_family;   //协议簇(IP协议)
    int m_type; //socket类型    流格式套接字TCP（SOCK_STREAM） 数据报格式套接字UDP（SOCK_DGRAM）
    int m_protocol; //协议
    bool m_isConnected; //是否连接成功
    
    Address::ptr m_localAddress;    //本地地址(本机：服务器——服务器地址)     本机：客户器——客户器地址)
    Address::ptr m_remoteAddress;   //远端地址(本机：服务器——客户器地址)    本机：客户器——服务器地址)

};

ostream& operator<<(ostream& os, const Socket& sock);


}



#endif // 