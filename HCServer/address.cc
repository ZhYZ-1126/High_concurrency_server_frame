#include"address.h"
#include"log.h"
#include"endian.h"
#include <sys/socket.h>   
#include<string.h>
#include <netinet/in.h>   
#include <arpa/inet.h> 
#include<sstream>
#include <stddef.h>
#include<netdb.h>
 #include <sys/types.h>
#include <ifaddrs.h>
#include<iostream>
using namespace std;

namespace HCServer
{

static HCServer::Logger::ptr g_logger=HCSERVER_LOG_ROOT();

template<class T>
static T CreateMask(uint32_t bits) { //子网掩码反码
    return (1 << (sizeof(T) * 8 - bits)) - 1;
}

template<class T>
static uint32_t CountBytes(T value) {//求ip地址中1的数量
    uint32_t result = 0;
    for(; value; ++result) {
        value &= value - 1;
    }
    return result;
}

//通过传入的addr创建一个Address类对象
Address::ptr Address::Create(const sockaddr* addr, socklen_t addrlen)
{
    if(addr == nullptr) {
        return nullptr;
    }

    Address::ptr result;
    switch(addr->sa_family) {
        case AF_INET:
            result.reset(new IPv4Address(*(const sockaddr_in*)addr));
            break;
        case AF_INET6:
            result.reset(new IPv6Address(*(const sockaddr_in6*)addr));
            break;
        default:
            result.reset(new UnknownAddress(*addr));
            break;
    }
    return result;
}

//通过host地址返回对应条件的所有Address,并插入容器中(DNS域名服务器作用，可以解析网上的网站--要联网 http:80)
//result 保存满足条件的Address(容器) , host 域名,服务器名等,family 协议族(AF_INT, AF_INT6, AF_UNIX),
//type socket类型SOCK_STREAM、SOCK_DGRAM 等, protocol 协议,IPPROTO_TCP、IPPROTO_UDP 等
bool Address::Lookup(vector<Address::ptr> & result,const string & host, int family,int type,int protocol)
{
    addrinfo hints, *results, *next;
    hints.ai_flags = 0;
    hints.ai_family = family;
    hints.ai_socktype = type;
    hints.ai_protocol = protocol;
    hints.ai_addrlen = 0;
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    string node;//主机名，也可以是数字地址（IPV4的10进制，或是IPV6的16进制）
    const char* service = NULL;//端口号的位置

    //检查 ipv6address serivce
    //IPv6的域名/服务器名形式：[xx:xx:xx:xx:xx:xx:xx:xx]:端口号
    if(!host.empty() && host[0] == '[') {
        //在参数 host.c_str() + 1 所指向的字符串的前 host.size() - 1 个字节中搜索第一次出现字符 ] 的位置
        const char* endipv6 = (const char*)memchr(host.c_str() + 1, ']', host.size() - 1);
        if(endipv6) {
            if(*(endipv6 + 1) == ':') {
                service = endipv6 + 2;//得到端口号的位置的起始位置
            }
            node = host.substr(1, endipv6 - host.c_str() - 1);
        }
    }

    //检查 node serivce
    if(node.empty()) {
        service = (const char*)memchr(host.c_str(), ':', host.size());
        if(service) {
            if(!memchr(service + 1, ':', host.c_str() + host.size() - service - 1)) {
                node = host.substr(0, service - host.c_str());
                ++service;
            }
        }
    }

    if(node.empty()) {
        node = host;
    }
    
    //getaddrinfo:用于将主机名和服务名解析为套接字地址结构,
    //返回一个指向（由其中的ai_next成员）串联起来的addrinfo结构链表的指针（result）
    int error = getaddrinfo(node.c_str(), service, &hints, &results);
    if(error) {
        HCSERVER_LOG_DEBUG(g_logger) << "Address::Lookup getaddress(" << host << ", "
            << family << ", " << type << ") err=" << error << " errstr="
            << gai_strerror(error);
        return false;
    }

    next = results;//将得到的addrinfo结构体链表赋值给next，方便遍历
    while(next) { //循环遍历，将拿到的地址创建一个Address对象，然后插入到result容器中
        result.push_back(Create(next->ai_addr, (socklen_t)next->ai_addrlen));
        next = next->ai_next;//得到指向下一个addrinfo结构体的指针
    }

    freeaddrinfo(results);
    return !result.empty();
}

//通过host地址返回对应条件的任意Address
// host 域名,服务器名等,family 协议族(AF_INT, AF_INT6, AF_UNIX),
//type socket类型SOCK_STREAM、SOCK_DGRAM 等, protocol 协议,IPPROTO_TCP、IPPROTO_UDP 等
Address::ptr Address::LookupAny(const string& host,int family , int type , int protocol )
{
    vector<Address::ptr> result;
    if(Lookup(result, host, family, type, protocol)) {
        return result[0];
    }
    return nullptr;
}

//通过host地址返回对应条件的任意IPAddress
shared_ptr<IPAddress> Address::LookupAnyIPAddress(const string& host,int family , int type , int protocol )
{
    vector<Address::ptr> result;
    if(Lookup(result, host, family, type, protocol)) {
        for(auto& i : result) {
            IPAddress::ptr v = dynamic_pointer_cast<IPAddress>(i);
            if(v) {
                return v;
            }
        }
    }
    return nullptr;
}

//返回本机所有网卡的<网卡名, 地址, 子网掩码位数>到result
bool Address::GetInterfaceAddresses(multimap<string,pair<Address::ptr, uint32_t> >& result,int family)
{
    struct ifaddrs *next, *results;
    //getifaddrs:获取当前系统上所有的网络接口信息，并返回一个ifaddrs结构体的链表，results为该链表的头指针
    if(getifaddrs(&results) != 0) {//获取信息失败
        HCSERVER_LOG_DEBUG(g_logger) << "Address::GetInterfaceAddresses getifaddrs "
            " err=" << errno << " errstr=" << strerror(errno);
        return false;       
    }

    try {
        //循环遍历ifaddrs链表
        for(next = results; next; next = next->ifa_next) {
            Address::ptr addr;
            uint32_t prefix_len = ~0u;
            //当前位置的family不满足条件，直接continue
            if(family != AF_UNSPEC && family != next->ifa_addr->sa_family) {
                continue;
            }
            switch(next->ifa_addr->sa_family) {
                case AF_INET:
                    {
                        addr = Create(next->ifa_addr, sizeof(sockaddr_in));
                        uint32_t netmask = ((sockaddr_in*)next->ifa_netmask)->sin_addr.s_addr;
                        prefix_len = CountBytes(netmask);//得到子网掩码
                    }
                    break;
                case AF_INET6:
                    {
                        addr = Create(next->ifa_addr, sizeof(sockaddr_in6));
                        in6_addr& netmask = ((sockaddr_in6*)next->ifa_netmask)->sin6_addr;
                        prefix_len = 0;
                        for(int i = 0; i < 16; ++i) {
                            prefix_len += CountBytes(netmask.s6_addr[i]);//得到子网掩码
                        }
                    }
                    break;
                default:
                    break;
            }

            if(addr) {
                result.insert(make_pair(next->ifa_name,make_pair(addr, prefix_len)));
            }
        }
    } catch (...) {
        HCSERVER_LOG_ERROR(g_logger) << "Address::GetInterfaceAddresses exception";
        freeifaddrs(results);
        return false;
    }
    //释放ifaddrs
    freeifaddrs(results);
    return !result.empty();
}

//获取指定网卡iface的地址和子网掩码位数到result
bool Address::GetInterfaceAddresses(vector<pair<Address::ptr, uint32_t> >&result,const string& iface, int family )
{
    if(iface.empty() || iface == "*") {
        if(family == AF_INET || family == AF_UNSPEC) {
            result.push_back(make_pair(Address::ptr(new IPv4Address()), 0u));
        }
        if(family == AF_INET6 || family == AF_UNSPEC) {
            result.push_back(make_pair(Address::ptr(new IPv6Address()), 0u));
        }
        return true;
    }

    multimap<string,pair<Address::ptr, uint32_t> > results;

    if(!GetInterfaceAddresses(results, family)) {//返回本机所有网卡的<网卡名, 地址, 子网掩码位数>到results
        return false;
    }

    //equal_range(iface):在已排序的[first,last)中寻找iface，它返回一对迭代器i和j
    //[i,j)内的每个元素都等同于iface，而且[i,j)是[first,last)之中符合此一性质的最大子区间
    //由于multimap容器会根据键值（string）进行排序，因此可以使用equal_range，不用担心顺序问题
    auto its = results.equal_range(iface);
    for(; its.first != its.second; ++its.first) {
        result.push_back(its.first->second);
    }
    return !result.empty();
}


int Address::getFamily() const
{
    return getAddr()->sa_family;
}

string Address::toString() const
{
    stringstream ss;
    insert(ss);
    return ss.str();
}

bool Address::operator<(const Address & rhs) const
{
    socklen_t minlen=min(getAddrlen(),rhs.getAddrlen());
    //比较内存区域getAddr()和rhs.getAddr()的前minlen个字节。
    
    int result=memcmp(getAddr(),rhs.getAddr(),minlen);
    if(result<0){//当getAddr()<rhs.getAddr()时，返回值result<0
        return true;
    }else if(result>0){//当getAddr()>rhs.getAddr()时，返回值result>0
        return false;
    }else if(getAddrlen()<rhs.getAddrlen()){
        return true;
    }
    return false;
}
bool Address::operator==(const Address & rhs) const
{
    return getAddrlen()==rhs.getAddrlen() &&memcmp(getAddr(),rhs.getAddr(),getAddrlen())==0;
}
bool Address::operator!=(const Address & rhs) const
{
    return !(*this==rhs);
}





//通过域名,IP地址,服务器名创建IPAddress(不管是ipv4的地址还是ipv6的地址),返回对应的ipv4指针或者ipv6指针
IPAddress::ptr IPAddress::Create(const char* address, uint16_t port)
{
    addrinfo hints, *results;
    memset(&hints, 0, sizeof(addrinfo));

    hints.ai_flags = AI_NUMERICHOST;
    hints.ai_family = AF_UNSPEC;
    //int getaddrinfo( const char *hostname, const char *service, const struct addrinfo *hints, struct addrinfo **result );
    //hostname:主机名或者数字地址（ipv4/ipv6）
    //service：包含十进制数的端口号或服务名如（ftp,http）
    //hints:是一个空指针或指向一个addrinfo结构的指针，由调用者填写关于它所想返回的信息类型的线索。
    //res:存放返回addrinfo结构链表的指针
    //getaddrinfo:用于将主机名和服务名解析为套接字地址结构,
    //返回一个指向（由其中的ai_next成员）串联起来的addrinfo结构链表的指针（result）
    int error = getaddrinfo(address, NULL, &hints, &results);
    if(error) {
        HCSERVER_LOG_DEBUG(g_logger) << "IPAddress::Create(" << address
            << ", " << port << ") error=" << error
            << " errno=" << errno << " errstr=" << strerror(errno);
        return nullptr;
    }

    try {
        //将Address类型的对象转化成IPAddress对象
        IPAddress::ptr result = dynamic_pointer_cast<IPAddress>(Address::Create(results->ai_addr, (socklen_t)results->ai_addrlen));
        if(result) {
            result->setPort(port);
        }
        freeaddrinfo(results);
        return result;
    } catch (...) {
        freeaddrinfo(results);
        return nullptr;
    }
}



//ipv4的方法

//使用字符串信息的地址（如1.1.1.1）创建一个ipv4类对象
IPv4Address::ptr IPv4Address::Create(const char * address,uint32_t port)
{
    IPv4Address::ptr rt(new IPv4Address);
    rt->m_addr.sin_port = byteswapOnLittleEndian(port);
    //将字符串信息的地址address转化成网络ipv4形式的地址
    int result = inet_pton(AF_INET, address, &rt->m_addr.sin_addr);
    if(result <= 0) {
        HCSERVER_LOG_DEBUG(g_logger) << "IPv4Address::Create(" << address << ", "
                << port << ") rt=" << result << " errno=" << errno
                << " errstr=" << strerror(errno);
        return nullptr;
    }
    return rt;
}
IPv4Address::IPv4Address(const sockaddr_in& address)
{
    m_addr = address;
}

IPv4Address::IPv4Address(uint32_t address,uint32_t port)
{
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin_family = AF_INET;
    m_addr.sin_port = byteswapOnLittleEndian(port);
    m_addr.sin_addr.s_addr = byteswapOnLittleEndian(address);
}

const sockaddr* IPv4Address::getAddr() const
{
    return (sockaddr*)&m_addr;
}

sockaddr* IPv4Address::getAddr()
{
    return (sockaddr*)&m_addr;
}

socklen_t IPv4Address::getAddrlen() const
{
    return sizeof(m_addr);
}

ostream& IPv4Address::insert(ostream& os) const
{
    uint32_t addr = byteswapOnLittleEndian(m_addr.sin_addr.s_addr);
    os << ((addr >> 24) & 0xff) << "."
       << ((addr >> 16) & 0xff) << "."
       << ((addr >> 8) & 0xff) << "."
       << (addr & 0xff);
    os << ":" << byteswapOnLittleEndian(m_addr.sin_port);
    return os;
}
//网关地址
IPAddress::ptr IPv4Address::broadcastAddress(uint32_t prefix_len) 
{
    if(prefix_len > 32) {
        return nullptr;
    }

    sockaddr_in baddr(m_addr);
    baddr.sin_addr.s_addr |= byteswapOnLittleEndian(CreateMask<uint32_t>(prefix_len));
    return IPv4Address::ptr(new IPv4Address(baddr));
}
//网络地址
IPAddress::ptr IPv4Address::networkAddress(uint32_t prefix_len)
{
    if(prefix_len > 32) {
        return nullptr;
    }

    sockaddr_in baddr(m_addr);
    baddr.sin_addr.s_addr &= byteswapOnLittleEndian(CreateMask<uint32_t>(prefix_len));
    return IPv4Address::ptr(new IPv4Address(baddr));
}
//子网掩码
IPAddress::ptr IPv4Address::subnetMask(uint32_t prefix_len) 
{
    sockaddr_in subnet;
    memset(&subnet, 0, sizeof(subnet));
    subnet.sin_family = AF_INET;
    subnet.sin_addr.s_addr = ~byteswapOnLittleEndian(CreateMask<uint32_t>(prefix_len));
    return IPv4Address::ptr(new IPv4Address(subnet));
}
//获得端口号
uint32_t IPv4Address::getPort() const  
{
    return byteswapOnLittleEndian(m_addr.sin_port);
}
//设置端口号
void IPv4Address::setPort(uint16_t v) 
{
    m_addr.sin_port=byteswapOnLittleEndian(v);
}


//ipv6的方法

//使用字符串信息的地址（如1.1.1.1）创建一个ipv6类对象
IPv6Address::ptr IPv6Address::Create(const char* address, uint32_t port)
{
    IPv6Address::ptr rt(new IPv6Address);
    rt->m_addr.sin6_port = byteswapOnLittleEndian(port);
    //将字符串信息的地址address转化成ipv6形式的地址
    int result = inet_pton(AF_INET6, address, &rt->m_addr.sin6_addr);
    if(result <= 0) {
        HCSERVER_LOG_DEBUG(g_logger) << "IPv6Address::Create(" << address << ", "
                << port << ") rt=" << result << " errno=" << errno
                << " errstr=" << strerror(errno);
        return nullptr;
    }
    return rt;
}
IPv6Address::IPv6Address()
{
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin6_family = AF_INET6;
}
IPv6Address::IPv6Address(const sockaddr_in6& address) {
    m_addr = address;
}
IPv6Address::IPv6Address(const uint8_t address[16], uint16_t port)
{
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin6_family = AF_INET6;
    m_addr.sin6_port = byteswapOnLittleEndian(port);
    memcpy(&m_addr.sin6_addr.s6_addr, address, 16);
}

const sockaddr* IPv6Address::getAddr() const
{
    return (sockaddr*)&m_addr;
}

sockaddr* IPv6Address::getAddr()
{
    return (sockaddr*)&m_addr;
}

socklen_t IPv6Address::getAddrlen() const
{
    return sizeof(m_addr);
}

ostream& IPv6Address::insert(ostream& os) const
{
    os << "[";
    uint16_t* addr = (uint16_t*)m_addr.sin6_addr.s6_addr;
    bool used_zeros = false;
    for(size_t i = 0; i < 8; ++i) {
        if(addr[i] == 0 && !used_zeros) {
            continue;
        }
        if(i && addr[i - 1] == 0 && !used_zeros) {
            os << ":";
            used_zeros = true;
        }
        if(i) {
            os << ":";
        }
        os << hex << (int)byteswapOnLittleEndian(addr[i]) << dec;
    }

    if(!used_zeros && addr[7] == 0) {
        os << "::";
    }

    os << "]:" << byteswapOnLittleEndian(m_addr.sin6_port);
    return os;
}
//网关地址
IPAddress::ptr IPv6Address::broadcastAddress(uint32_t prefix_len) 
{
    sockaddr_in6 baddr(m_addr);
    baddr.sin6_addr.s6_addr[prefix_len / 8] |=CreateMask<uint8_t>(prefix_len % 8);
    for(int i = prefix_len / 8 + 1; i < 16; ++i) {
        baddr.sin6_addr.s6_addr[i] = 0xff;
    }
    return IPv6Address::ptr(new IPv6Address(baddr));
}
//网络地址
IPAddress::ptr IPv6Address::networkAddress(uint32_t prefix_len)
{
    sockaddr_in6 baddr(m_addr);
    baddr.sin6_addr.s6_addr[prefix_len / 8] &=CreateMask<uint8_t>(prefix_len % 8);
    for(int i = prefix_len / 8 + 1; i < 16; ++i) {
        baddr.sin6_addr.s6_addr[i] = 0x00;
    }
    return IPv6Address::ptr(new IPv6Address(baddr));
}
//子网掩码
IPAddress::ptr IPv6Address::subnetMask(uint32_t prefix_len) 
{
    sockaddr_in6 subnet;
    memset(&subnet, 0, sizeof(subnet));
    subnet.sin6_family = AF_INET6;
    subnet.sin6_addr.s6_addr[prefix_len /8] =~CreateMask<uint8_t>(prefix_len % 8);

    for(uint32_t i = 0; i < prefix_len / 8; ++i) {
        subnet.sin6_addr.s6_addr[i] = 0xff;
    }
    return IPv6Address::ptr(new IPv6Address(subnet));   
}

uint32_t IPv6Address::getPort() const  //获得端口号
{
    return byteswapOnLittleEndian(m_addr.sin6_port);
}

void IPv6Address::setPort(uint16_t v) //设置端口号
{
    m_addr.sin6_port = byteswapOnLittleEndian(v);
}



//unix地址

static const size_t MAX_PATH_LEN=sizeof(((sockaddr_un*)0)->sun_path)-1;

UnixAddress::UnixAddress()
{
    memset(&m_addr,0,sizeof(m_addr));
    m_addr.sun_family=AF_UNIX;
    m_length=offsetof(sockaddr_un,sun_path)+MAX_PATH_LEN;
}
UnixAddress::UnixAddress(const string & path)
{
    memset(&m_addr,0,sizeof(m_addr));
    m_addr.sun_family=AF_UNIX;
    m_length=path.size()+1;
    if(!path.empty()&& path[0]=='\0'){
        --m_length;
    }

    if(m_length > sizeof(m_addr.sun_path)) {
        throw  logic_error("path too long");
    }
    memcpy(m_addr.sun_path, path.c_str(), m_length);
    m_length += offsetof(sockaddr_un, sun_path);

}
const sockaddr* UnixAddress::getAddr() const
{
    return (sockaddr*)&m_addr;
}

sockaddr* UnixAddress::getAddr()
{
    return (sockaddr*)&m_addr;
}

socklen_t UnixAddress::getAddrlen() const 
{
    return m_length;
}
ostream& UnixAddress::insert(ostream& os) const
{
    if(m_length > offsetof(sockaddr_un, sun_path)
            && m_addr.sun_path[0] == '\0') {
        return os << "\\0" << string(m_addr.sun_path + 1,
                m_length - offsetof(sockaddr_un, sun_path) - 1);
    }
    return os << m_addr.sun_path;
}

// 设置unix地址的长度
void UnixAddress::setAddrlen(uint32_t len)
{
    m_length=len;
}



//未知地址

UnknownAddress::UnknownAddress(int family)
{
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sa_family = family;
}
UnknownAddress::UnknownAddress(const sockaddr& addr)
{
    m_addr=addr;
}

const sockaddr* UnknownAddress::getAddr() const 
{
    return (sockaddr*)&m_addr;
}

sockaddr* UnknownAddress::getAddr()
{
    return (sockaddr*)&m_addr;
}

socklen_t UnknownAddress::getAddrlen() const
{
    return sizeof(m_addr);
}

ostream& UnknownAddress::insert(ostream& os) const
{
    os << "[UnknownAddress family=" << m_addr.sa_family << "]";
    return os;
}

//流式输出addr   os 输出流   addr Address
ostream& operator<<(ostream& os, const Address& addr)
{
    return addr.insert(os);
}


}