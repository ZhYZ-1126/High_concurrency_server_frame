#include"socket.h"
#include"HCServer.h"
#include"fd_manager.h"
#include<sys/socket.h>
#include<stdint.h>
#include<sys/types.h>
#include<sstream>
#include <netinet/tcp.h>

namespace HCServer{

static HCServer::Logger::ptr g_logger = HCSERVER_LOG_NAME("system");

Socket::Socket(int family, int type, int protocol)
    :m_sock(-1)
    ,m_family(family)
    ,m_type(type)
    ,m_protocol(protocol)
    ,m_isConnected(false) {
}

Socket::~Socket() {
    close();
}

// 创建TCP Socket(满足地址类型)
Socket::ptr Socket::CreateTCP(HCServer::Address::ptr address) {
    Socket::ptr sock(new Socket(address->getFamily(), TCP, 0));
    return sock;
}

// 创建UDP Socket(满足地址类型)
Socket::ptr Socket::CreateUDP(HCServer::Address::ptr address) {
    Socket::ptr sock(new Socket(address->getFamily(), UDP, 0));
    sock->newSock();
    sock->m_isConnected = true;//UDP是面向无连接的，m_isConnected直接置true
    return sock;
}

//创建IPv4的TCP Socket
Socket::ptr Socket::CreateTCPSocket() {
    Socket::ptr sock(new Socket(IPv4, TCP, 0));
    return sock;
}

//创建IPv4的UDP Socket
Socket::ptr Socket::CreateUDPSocket() {
    Socket::ptr sock(new Socket(IPv4, UDP, 0));
    sock->newSock();
    sock->m_isConnected = true;
    return sock;
}

//创建IPv6的TCP Socket
Socket::ptr Socket::CreateTCPSocket6() {
    Socket::ptr sock(new Socket(IPv6, TCP, 0));
    return sock;
}

//创建IPv6的UDP Socket
Socket::ptr Socket::CreateUDPSocket6() {
    Socket::ptr sock(new Socket(IPv6, UDP, 0));
    sock->newSock();
    sock->m_isConnected = true;
    return sock;
}

//创建Unix的TCP Socket
Socket::ptr Socket::CreateUnixTCPSocket() {
    Socket::ptr sock(new Socket(UNIX, TCP, 0));
    return sock;
}

//创建Unix的UDP Socket
Socket::ptr Socket::CreateUnixUDPSocket() {
    Socket::ptr sock(new Socket(UNIX, UDP, 0));
    return sock;
}

//获取发送超时时间(毫秒)
int64_t Socket::getSendTimeout() {
    HCServer::FdCtx::ptr ctx = FdMgr::Getinstance()->get(m_sock);
    if(ctx) {
        return ctx->getTimeout(SO_SNDTIMEO);
    }
    return -1;
}

//设置发送超时时间(毫秒) 
void Socket::setSendTimeout(int64_t v) {
    struct timeval tv{int(v / 1000), int(v % 1000 * 1000)};
    setOption(SOL_SOCKET, SO_SNDTIMEO, tv);
}

//获取接收超时时间(毫秒)
int64_t Socket::getRecvTimeout() {
    FdCtx::ptr ctx = FdMgr::Getinstance()->get(m_sock);
    if(ctx) {
        return ctx->getTimeout(SO_RCVTIMEO);
    }
    return -1;
}

//设置接收超时时间(毫秒)
void Socket::setRecvTimeout(int64_t v) {
    struct timeval tv{int(v / 1000), int(v % 1000 * 1000)};
    setOption(SOL_SOCKET, SO_RCVTIMEO, tv);
}

//获取socket fd的信息,存放与result中
bool Socket::getOption(int level, int option, void* result, socklen_t* len) {
    int rt = getsockopt(m_sock, level, option, result, (socklen_t*)len);
    if(rt) {
        HCSERVER_LOG_DEBUG(g_logger) << "getOption sock=" << m_sock
            << " level=" << level << " option=" << option
            << " errno=" << errno << " errstr=" << strerror(errno);
        return false;
    }
    return true;
}

//设置fd的属性
bool Socket::setOption(int level, int option, const void* result, socklen_t len) {
    if(setsockopt(m_sock, level, option, result, (socklen_t)len)) {
        HCSERVER_LOG_DEBUG(g_logger) << "setOption sock=" << m_sock
            << " level=" << level << " option=" << option
            << " errno=" << errno << " errstr=" << strerror(errno);
        return false;
    }
    return true;
}



//服务器绑定IP地址
bool Socket::bind(const Address::ptr addr) {
    if(!isValid()) {
        newSock();
        if(HCSERVER_UNLIKELY(!isValid())) {
            return false;
        }
    }

    if(HCSERVER_UNLIKELY(addr->getFamily() != m_family)) {
        HCSERVER_LOG_ERROR(g_logger) << "bind sock.family("
            << m_family << ") addr.family(" << addr->getFamily()
            << ") not equal, addr=" << addr->toString();
        return false;
    }

    // UnixAddress::ptr uaddr = dynamic_pointer_cast<UnixAddress>(addr);
    // if(uaddr) {
    //     Socket::ptr sock = Socket::CreateUnixTCPSocket();
    //     if(sock->connect(uaddr)) {
    //         return false;
    //     } else {
    //         HCSERVER::FSUtil::Unlink(uaddr->getPath(), true);
    //     }
    // }

    if(::bind(m_sock, addr->getAddr(), addr->getAddrlen())) {
        HCSERVER_LOG_ERROR(g_logger) << "bind error errrno=" << errno
            << " errstr=" << strerror(errno);
        return false;
    }
    getLocalAddress();
    return true;
}

//监听socket客户端连接  backlog:监听上限
bool Socket::listen(int backlog) {
    if(!isValid()) {
        HCSERVER_LOG_ERROR(g_logger) << "listen error sock=-1";
        return false;
    }
    if(::listen(m_sock, backlog)) {
        HCSERVER_LOG_ERROR(g_logger) << "listen error errno=" << errno
            << " errstr=" << strerror(errno);
        return false;
    }
    return true;
}

//服务器接收connect(客户端)链接
Socket::ptr Socket::accept() {
    Socket::ptr sock(new Socket(m_family, m_type, m_protocol));
    int newsock = ::accept(m_sock, nullptr, nullptr);
    if(newsock == -1) {
        HCSERVER_LOG_ERROR(g_logger) << "accept(" << m_sock << ") errno="
            << errno << " errstr=" << strerror(errno);
        return nullptr;
    }
    if(sock->init(newsock)) {
        return sock;
    }
    return nullptr;
}

//客户端重新连接服务器地址
bool Socket::connect(const Address::ptr addr, uint64_t timeout_ms) {
    m_remoteAddress = addr;
    if(!isValid()) {
        newSock();
        if(HCSERVER_UNLIKELY(!isValid())) {
            return false;
        }
    }

    if(HCSERVER_UNLIKELY(addr->getFamily() != m_family)) {
        HCSERVER_LOG_ERROR(g_logger) << "connect sock.family("
            << m_family << ") addr.family(" << addr->getFamily()
            << ") not equal, addr=" << addr->toString();
        return false;
    }

    if(timeout_ms == (uint64_t)-1) {
        if(::connect(m_sock, addr->getAddr(), addr->getAddrlen())) {
            HCSERVER_LOG_ERROR(g_logger) << "sock=" << m_sock << " connect(" << addr->toString()
                << ") error errno=" << errno << " errstr=" << strerror(errno);
            close();
            return false;
        }
    } else {
        if(::connect_with_timeout(m_sock, addr->getAddr(), addr->getAddrlen(), timeout_ms)) {
            HCSERVER_LOG_ERROR(g_logger) << "sock=" << m_sock << " connect(" << addr->toString()
                << ") timeout=" << timeout_ms << " error errno="
                << errno << " errstr=" << strerror(errno);
            close();
            return false;
        }
    }
    m_isConnected = true;
    getRemoteAddress();
    getLocalAddress();
    return true;
}

//客户端连接服务器地址
bool Socket::reconnect(uint64_t timeout_ms) {
    if(!m_remoteAddress) {
        HCSERVER_LOG_ERROR(g_logger) << "reconnect m_remoteAddress is null";
        return false;
    }
    m_localAddress.reset();
    return connect(m_remoteAddress, timeout_ms);
}

//关闭socket
bool Socket::close() {
    if(!m_isConnected && m_sock == -1) {
        return true;
    }
    m_isConnected = false;
    if(m_sock != -1) {
        ::close(m_sock);
        m_sock = -1;
    }
    return false;
}





//发送数据(TCP)
int Socket::send(const void* buffer, size_t length, int flags) {
    if(isConnected()) {
        return ::send(m_sock, buffer, length, flags);
    }
    return -1;
}
//iovec
int Socket::send(const iovec* buffers, size_t length, int flags) {
    if(isConnected()) {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (iovec*)buffers;
        msg.msg_iovlen = length;
        return ::sendmsg(m_sock, &msg, flags);
    }
    return -1;
}

//发送数据(UDP)
int Socket::sendTo(const void* buffer, size_t length, const Address::ptr to, int flags) {
    if(isConnected()) {
        return ::sendto(m_sock, buffer, length, flags, to->getAddr(), to->getAddrlen());
    }
    return -1;
}
//iovec
int Socket::sendTo(const iovec* buffers, size_t length, const Address::ptr to, int flags) {
    if(isConnected()) {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (iovec*)buffers;
        msg.msg_iovlen = length;
        msg.msg_name = to->getAddr();
        msg.msg_namelen = to->getAddrlen();
        return ::sendmsg(m_sock, &msg, flags);
    }
    return -1;
}

//接收数据（TCP）
int Socket::recv(void* buffer, size_t length, int flags) {
    if(isConnected()) {
        return ::recv(m_sock, buffer, length, flags);
    }
    return -1;
}
//iovec
int Socket::recv(iovec* buffers, size_t length, int flags) {
    if(isConnected()) {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (iovec*)buffers;
        msg.msg_iovlen = length;
        return ::recvmsg(m_sock, &msg, flags);
    }
    return -1;
}

//接收数据（UDP）
int Socket::recvFrom(void* buffer, size_t length, Address::ptr from, int flags) {
    if(isConnected()) {
        socklen_t len = from->getAddrlen();
        return ::recvfrom(m_sock, buffer, length, flags,from->getAddr(), &len);
    }
    return -1;
}
//iovec
int Socket::recvFrom(iovec* buffers, size_t length, Address::ptr from, int flags) {
    if(isConnected()) {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (iovec*)buffers;
        msg.msg_iovlen = length;
        msg.msg_name = from->getAddr();
        msg.msg_namelen = from->getAddrlen();
        return ::recvmsg(m_sock, &msg, flags);
    }
    return -1;
}

//获取远端地址(本机：服务器——客户器地址     本机：客户器——服务器地址)
Address::ptr Socket::getRemoteAddress() {
    if(m_remoteAddress) {
        return m_remoteAddress;
    }

    Address::ptr result;
    switch(m_family) {
        case AF_INET:
            result.reset(new IPv4Address());
            break;
        case AF_INET6:
            result.reset(new IPv6Address());
            break;
        case AF_UNIX:
            result.reset(new UnixAddress());
            break;
        default:
            result.reset(new UnknownAddress(m_family));
            break;
    }
    socklen_t addrlen = result->getAddrlen();
    //getpeername函数用于获取与某个套接字(m_sock)关联的外地协议地址,存放到result中
    if(getpeername(m_sock, (sockaddr*)result->getAddr(), &addrlen)) {
        HCSERVER_LOG_ERROR(g_logger) << "getpeername error sock=" << m_sock
           << " errno=" << errno << " errstr=" << strerror(errno);
        return Address::ptr(new UnknownAddress(m_family));
    }
    if(m_family == AF_UNIX) {//如果是Unix得重新设置地址长度
        UnixAddress::ptr addr = dynamic_pointer_cast<UnixAddress>(result);
        addr->setAddrlen(addrlen);
    }
    m_remoteAddress = result;
    return m_remoteAddress;
}

//获取本地地址(本机：服务器——服务器地址     本机：客户器——客户器地址)
Address::ptr Socket::getLocalAddress() {
    if(m_localAddress) {
        return m_localAddress;
    }

    Address::ptr result;
    switch(m_family) {
        case AF_INET:
            result.reset(new IPv4Address());
            break;
        case AF_INET6:
            result.reset(new IPv6Address());
            break;
        case AF_UNIX:
            result.reset(new UnixAddress());
            break;
        default:
            result.reset(new UnknownAddress(m_family));
            break;
    }
    socklen_t addrlen = result->getAddrlen();
    //getsockname函数在addr指向的缓冲区中返回套接字m_sock绑定到的当前地址
    if(getsockname(m_sock, (sockaddr*)result->getAddr(), &addrlen)) {
        HCSERVER_LOG_ERROR(g_logger) << "getsockname error sock=" << m_sock
            << " errno=" << errno << " errstr=" << strerror(errno);
        return Address::ptr(new UnknownAddress(m_family));
    }
    if(m_family == AF_UNIX) {//如果是Unix得重新设置地址长度
        UnixAddress::ptr addr = dynamic_pointer_cast<UnixAddress>(result);
        addr->setAddrlen(addrlen);
    }
    m_localAddress = result;
    return m_localAddress;
}

//m_sock是否有效(m_sock != -1)
bool Socket::isValid() const {
    return m_sock != -1;
}

//返回Socket的错误
int Socket::getError() {
    int error = 0;
    socklen_t len = sizeof(error);
    if(!getOption(SOL_SOCKET, SO_ERROR, &error, &len)) {
        error = errno;
    }
    return error;
}

//输出socket基本信息到流中
ostream& Socket::dump (ostream& os) const {
    os << "[Socket sock=" << m_sock
       << " is_connected=" << m_isConnected
       << " family=" << m_family
       << " type=" << m_type
       << " protocol=" << m_protocol;
    if(m_localAddress) {
        os << " local_address=" << m_localAddress->toString();
    }
    if(m_remoteAddress) {
        os << " remote_address=" << m_remoteAddress->toString();
    }
    os << "]";
    return os;
}

//socket基本信息字符串化
string Socket::toString() const {
    stringstream ss;
    dump(ss);
    return ss.str();
}

//取消读(唤醒执行)
bool Socket::cancelRead() {
    return IOManager::Getthis()->cancelEvent(m_sock, HCServer::IOManager::READ);
}

//取消写(唤醒执行)
bool Socket::cancelWrite() {
    return IOManager::Getthis()->cancelEvent(m_sock, HCServer::IOManager::WRITE);
}

//取消accept(唤醒执行)
bool Socket::cancelAccept() {
    return IOManager::Getthis()->cancelEvent(m_sock, HCServer::IOManager::READ);
}

//取消所有事件(唤醒执行)
bool Socket::cancelAll() {
    return IOManager::Getthis()->cancelAll(m_sock);
}

//初始化socket(m_sock)
void Socket::initSock() {
    int val = 1;
    setOption(SOL_SOCKET, SO_REUSEADDR, val);
    if(m_type == SOCK_STREAM) {
        setOption(IPPROTO_TCP, TCP_NODELAY, val);
    }
}

//创建新的socket(赋予到m_sock)
void Socket::newSock() {
    m_sock = socket(m_family, m_type, m_protocol);
    if(HCSERVER_LIKELY(m_sock != -1)) {
        initSock();
    } else {
        HCSERVER_LOG_ERROR(g_logger) << "socket(" << m_family
            << ", " << m_type << ", " << m_protocol << ") errno="
            << errno << " errstr=" << strerror(errno);
    }
}

//初始化sock(sock赋予到m_sock)
bool Socket::init(int sock) {
    FdCtx::ptr ctx = FdMgr::Getinstance()->get(sock);
    if(ctx && ctx->isSocket() && !ctx->isClose()) {
        m_sock = sock;
        m_isConnected = true;
        initSock();//初始化m_sock的信息，设置fd的信息
        getLocalAddress();
        getRemoteAddress();
        return true;
    }
    return false;
}

ostream& operator<<(ostream& os, const Socket& sock)
{
    return sock.dump(os);
}

}