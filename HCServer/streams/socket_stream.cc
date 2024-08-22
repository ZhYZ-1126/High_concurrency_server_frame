#include"socket_stream.h"

namespace HCServer{

//构造函数  sock Socket类   owner sock是否完全交给当时的类控制  
SocketStream::SocketStream(Socket::ptr sock, bool owner)
:m_socket(sock),m_owner(owner)
{}
SocketStream::~SocketStream()
{
    if(m_owner && m_socket) {
        m_socket->close();
    }
}
//读数据    buffer 存储接收到数据的内存 length 接收数据的内存大小   
int SocketStream::read(void* buffer, size_t length)
{
    if(!isConnected()) {
        return -1;
    }
    return m_socket->recv(buffer, length);
}
//读数据    ba 存储接收到数据的ByteArray内存池  length 接收数据的内存大小
int SocketStream::read(HCServer::ByteArray::ptr ba, size_t length)
{
    if(!isConnected()) {
        return -1;
    }
    vector<iovec> iovs;
    ba->getWriteBuffers(iovs, length);
    int rt = m_socket->recv(&iovs[0], iovs.size());
    if(rt > 0) {
        ba->setPosition(ba->getPosition() + rt);
    }
    return rt;
}
//写数据    buffer 写数据的内存 length 写入数据的内存大小
int SocketStream::write(const void* buffer, size_t length)
{
    if(!isConnected()) {
        return -1;
    }
    return m_socket->send(buffer, length);
}
//写数据    ba 写数据的ByteArray    length 写入数据的内存大小
int SocketStream::write(ByteArray::ptr ba, size_t length)
{
    if(!isConnected()) {
        return -1;
    }
    vector<iovec> iovs;
    ba->getReadBuffers(iovs, length);
    int rt = m_socket->send(&iovs[0], iovs.size());
    if(rt > 0) {
        ba->setPosition(ba->getPosition() + rt);
    }
    return rt;
}
//关闭socket
void SocketStream::close()
{
    if(m_socket) {
        m_socket->close();
    }
}
//返回是否连接
bool SocketStream::isConnected() const {
    return m_socket && m_socket->isConnected();
}
//获取远端地址
Address::ptr SocketStream::getRemoteAddress() {
    if(m_socket) {
        return m_socket->getRemoteAddress();
    }
    return nullptr;
}
 //获取本地地址
Address::ptr SocketStream::getLocalAddress() {
    if(m_socket) {
        return m_socket->getLocalAddress();
    }
    return nullptr;
}
//返回远端地址的可读性字符串
string SocketStream::getRemoteAddressString() {
    auto addr = getRemoteAddress();
    if(addr) {
        return addr->toString();
    }
    return "";
}
//返回本地地址的可读性字符串
string SocketStream::getLocalAddressString() {
    auto addr = getLocalAddress();
    if(addr) {
        return addr->toString();
    }
    return "";
}

}