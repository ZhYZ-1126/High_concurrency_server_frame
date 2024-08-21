#ifndef _HCSERVER_ADDRESS_H__
#define _HCSERVER_ADDRESS_H__

#include<memory>
#include<vector>
#include<map>
#include<string>
#include<sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include<sys/types.h>
#include<iostream>
#include<unistd.h>
using namespace std;

namespace HCServer
{
class IPAddress;//声明


//地址类：有ip地址子类、unix地址子类和未知地址子类
//ip地址子类：有ipv4地址子类和ipv6地址子类


//地址抽象类
class Address{
public:
    typedef shared_ptr<Address> ptr;

    //通过addr创建一个Address类对象
    static Address::ptr Create(const sockaddr* addr, socklen_t addrlen);

    //通过host地址返回对应条件的所有Address,并插入容器中(DNS域名服务器作用，可以解析网上的网站--要联网 http:80)
    static bool Lookup(vector<Address::ptr> & result,const string & host,int family=AF_INET,int type=0,int protocol=0);

    //通过host地址返回对应条件的任意Address(DNS域名服务器作用，可以解析网上的网站--要联网 http:80)
    static Address::ptr LookupAny(const string& host,int family = AF_INET, int type = 0, int protocol = 0);

    //通过host地址返回对应条件的任意IPAddress
    static shared_ptr<IPAddress> LookupAnyIPAddress(const string& host,int family = AF_INET, int type = 0, int protocol = 0);

    //返回本机所有网卡的<网卡名, 地址, 子网掩码位数>到result
    static bool GetInterfaceAddresses(multimap<string,pair<Address::ptr, uint32_t> >& result,int family = AF_INET);

    //获取指定网卡的地址和子网掩码位数到result
    static bool GetInterfaceAddresses(vector<pair<Address::ptr, uint32_t> >&result,const string & iface, int family = AF_INET);


    virtual ~Address() {}

    int getFamily() const; //获得当前地址的协议族（ipv4，ipv6，unix）

    virtual const sockaddr* getAddr() const=0;
    virtual sockaddr* getAddr() = 0;
    virtual socklen_t getAddrlen() const =0;

    virtual ostream& insert(ostream& os) const=0;
    string toString() const ;

    bool operator<(const Address & rhs) const;
    bool operator==(const Address & rhs) const;
    bool operator!=(const Address & rhs) const;

};

//ip地址类
class IPAddress:public Address
{
public:
    typedef shared_ptr<IPAddress> ptr;

    //通过域名,IP地址,服务器名创建IPAddress(不管是ipv4的地址还是ipv6的地址)
    static IPAddress::ptr Create(const char* address, uint16_t port = 0);

    virtual IPAddress::ptr broadcastAddress(uint32_t prefix_len)=0;//网关地址
    virtual IPAddress::ptr networkAddress(uint32_t prefix_len)=0;//网络地址
    virtual IPAddress::ptr subnetMask(uint32_t prefix_len)=0;//子网掩码

    virtual uint32_t getPort() const =0;//获得端口号
    virtual void setPort(uint16_t v)=0;//设置端口号
};

//ipv4地址类
class IPv4Address:public IPAddress
{
public:
    typedef shared_ptr<IPv4Address> ptr;

    //使用字符串信息的地址（如1.1.1.1）创建一个ipv4类对象
    static IPv4Address::ptr Create(const char *address,uint32_t port=0);

    IPv4Address(const sockaddr_in& address);
    IPv4Address(uint32_t address=INADDR_ANY,uint32_t port=0);

    const sockaddr* getAddr() const override;
    sockaddr* getAddr() override;
    socklen_t getAddrlen() const override;
    ostream& insert(ostream& os) const override;

    IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;//网关地址
    IPAddress::ptr networkAddress(uint32_t prefix_len) override;//网络地址
    IPAddress::ptr subnetMask(uint32_t prefix_len) override;//子网掩码

    uint32_t getPort() const override;//获得端口号
    void setPort(uint16_t v) override;//设置端口号

private:
    sockaddr_in m_addr;
};

//ipv6地址类
class IPv6Address:public IPAddress
{
public:
    typedef shared_ptr<IPv6Address> ptr;
    //使用字符串信息的地址（如1.1.1.1）创建一个ipv6类对象
    static IPv6Address::ptr Create(const char* address, uint32_t port=0);

    IPv6Address();
    IPv6Address(const sockaddr_in6& address);
    IPv6Address(const uint8_t address[16], uint16_t port=0);

    const sockaddr* getAddr() const override;
    sockaddr* getAddr() override;
    socklen_t getAddrlen() const override;
    ostream& insert(ostream& os) const override;

    IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;//网关地址
    IPAddress::ptr networkAddress(uint32_t prefix_len) override;//网络地址
    IPAddress::ptr subnetMask(uint32_t prefix_len) override;//子网掩码

    uint32_t getPort() const  override;//获得端口号
    void setPort(uint16_t v) override;//设置端口号

private:
    sockaddr_in6 m_addr;

};

//unix地址类
class UnixAddress:public Address
{
public:
    typedef shared_ptr<UnixAddress> ptr;
    UnixAddress();
    UnixAddress(const string & path);

    const sockaddr* getAddr() const override;
    sockaddr* getAddr() override;
    socklen_t getAddrlen() const override;
    ostream& insert(ostream& os) const override;
    void setAddrlen(uint32_t len);

private:
    sockaddr_un m_addr;//unix的地址
    socklen_t m_length;//地址长度
};

//未知地址类
class UnknownAddress:public Address
{
public:
    typedef shared_ptr<UnknownAddress> ptr;
    UnknownAddress(int family);
    UnknownAddress(const sockaddr& addr);
    const sockaddr* getAddr() const override;
    sockaddr* getAddr() override;
    socklen_t getAddrlen() const override;
    ostream& insert(ostream& os) const override;
    

private:
    sockaddr m_addr;
};

//流式输出addr   os 输出流   addr Address
ostream& operator<<(ostream& os, const Address& addr);

}


#endif