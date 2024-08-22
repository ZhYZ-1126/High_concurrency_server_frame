/*
*Tcp服务器模块
*支持同时绑定多个地址进行监听，只需要在绑定时传入地址数组即可
*还可以分别指定接收客户端和处理客户端的协程调度器
*处理客户端的 协程调度器 的协程执行队列中的任务（handleClient（）） 可以写个子类继承该方法并且具体的实现
*/


#ifndef __HCSERVER_TCP_SERVER_H__
#define __HCSERVER_TCP_SERVER_H__

#include<memory>
#include<functional>
#include"address.h"
#include"iomanager.h"
#include"socket.h"
#include"noncopyable.h"
#include"config.h"

namespace HCServer{

// //TCP服务器配置结构体
// struct TcpServerConf
// {
//     typedef shared_ptr<TcpServerConf> ptr;

//     vector<string> address;   //服务器绑定地址
//     int keepalive = 0;  //是否长连接
//     int timeout = 1000 * 2 * 60;    //连接超时时间
//     int ssl = 0;    //是否使用https所有的证书
//     string id; //服务器id
//     // 服务器类型，http, ws, rock
//     string type = "http";  //服务器类型
//     string name;   //服务器名称
//     string cert_file;
//     string key_file;
//     string accept_worker;  //服务器使用的IO协程调度器(连接使用的协程调度器)
//     string io_worker;  //新连接的Socket(客户端)工作的调度器(处理传输数据)
//     string process_worker; //工作过程中需要新的IO协程调度器
//     map<string, string> args;

//     bool isValid() const    //返回地址数组是否为空
//     {
//         return !address.empty();
//     }

//     bool operator==(const TcpServerConf& oth) const //判断自定义类型是否相等
//     {
//         return address == oth.address
//                 && keepalive == oth.keepalive
//                 && timeout == oth.timeout
//                 && name == oth.name
//                 && ssl == oth.ssl
//                 && cert_file == oth.cert_file
//                 && key_file == oth.key_file
//                 && accept_worker == oth.accept_worker
//                 && io_worker == oth.io_worker
//                 && process_worker == oth.process_worker
//                 && args == oth.args
//                 && id == oth.id
//                 && type == oth.type;
//     }
// };

// //类型转换模板类片特化(YAML String 转换成 TcpServerConf)
// template<>
// class LexicalCast<string, TcpServerConf>
// {
// public:
//     TcpServerConf operator()(const string& v)
//     {
//         YAML::Node node = YAML::Load(v);
//         TcpServerConf conf;
//         conf.id = node["id"].as<string>(conf.id);  //conf.id默认值
//         conf.type = node["type"].as<string>(conf.type);
//         conf.keepalive = node["keepalive"].as<int>(conf.keepalive);
//         conf.timeout = node["timeout"].as<int>(conf.timeout);
//         conf.name = node["name"].as<string>(conf.name);
//         conf.ssl = node["ssl"].as<int>(conf.ssl);
//         conf.cert_file = node["cert_file"].as<string>(conf.cert_file);
//         conf.key_file = node["key_file"].as<string>(conf.key_file);
//         conf.accept_worker = node["accept_worker"].as<string>();
//         conf.io_worker = node["io_worker"].as<string>();
//         conf.process_worker = node["process_worker"].as<string>();
//         conf.args = LexicalCast<string,map<string, string> >()(node["args"].as<string>(""));
//         if(node["address"].IsDefined()) //插入服务器的绑定地址
//         {
//             for(size_t i = 0; i < node["address"].size(); ++i)
//             {
//                 conf.address.push_back(node["address"][i].as<string>());
//             }
//         }
//         return conf;
//     }
// };

// //类型转换模板类片特化(TcpServerConf 转换成 YAML String)
// template<>
// class LexicalCast<TcpServerConf, string>
// {
// public:
//     string operator()(const TcpServerConf& conf)
//     {
//         YAML::Node node;
//         node["id"] = conf.id;
//         node["type"] = conf.type;
//         node["name"] = conf.name;
//         node["keepalive"] = conf.keepalive;
//         node["timeout"] = conf.timeout;
//         node["ssl"] = conf.ssl;
//         node["cert_file"] = conf.cert_file;
//         node["key_file"] = conf.key_file;
//         node["accept_worker"] = conf.accept_worker;
//         node["io_worker"] = conf.io_worker;
//         node["process_worker"] = conf.process_worker;
//         node["args"] = YAML::Load(LexicalCast<map<string, string>, string>()(conf.args));
//         for(auto& i : conf.address)
//         {
//             node["address"].push_back(i);
//         }
//         stringstream ss;
//         ss << node;
//         return ss.str();
//     }
// };

//TCP服务器封装
class TcpServer : public enable_shared_from_this<TcpServer>, Noncopyable//enable_shared_from_this<>成员函数可以获取自身类的智能指针,Time只能以智能指针形式存在
{
public:
    typedef shared_ptr<TcpServer> ptr;
        
    //构造函数
    //worker socket客户端工作的协程调度器(连接使用的协程调度器)
    //io_worker 新连接的Socket(客户端)工作的调度器(处理传输数据)
    //accept_worker 服务器socket执行接收socket连接的协程调度器(服务器使用的IO协程调度器)   
    TcpServer(HCServer::IOManager* worker = HCServer::IOManager::Getthis(),HCServer::IOManager* io_worker = HCServer::IOManager::Getthis()
                ,HCServer::IOManager* accept_worker = HCServer::IOManager::Getthis());

    virtual ~TcpServer();

    //绑定地址(并开启监听)  addrs 需要绑定的地址数组   ssl 是否使用https所有的证书
    virtual bool bind(HCServer::Address::ptr addr, bool ssl = false);

    //绑定地址数组(并开启监听)      addrs 需要绑定的地址数组
    //  fails 绑定失败的地址        ssl 是否使用https所有的证书
    virtual bool bind(const vector<Address::ptr>& addrs,vector<Address::ptr>& fails,bool ssl = false);

    //启动服务，并安排线程处理接受连接客户端连接
    virtual bool start();

    //停止服务
    virtual void stop();

    uint64_t getRecvTimeout() const { return m_recvTimeout;}    //返回服务器读取超时时间(毫秒)
    void setRecvTimeout(uint64_t v) { m_recvTimeout = v;}   //设置服务器读取超时时间(毫秒)
    string getName() const { return m_name;}   //返回服务器名称
    bool isStop() const { return m_isStop;} //返回服务器是否停止
    //TcpServerConf::ptr getConf() const { return m_conf;}    //返回TCP服务器配置结构体
    //void setConf(TcpServerConf::ptr v) { m_conf = v;}   //设置TCP服务器配置结构体
    vector<Socket::ptr> getSocks() const { return m_socks;}    //返回监听Socket数组(正在监听客户端的socket)

    //void setConf(const TcpServerConf& v);   //设置TCP服务器配置结构体

    //bool loadCertificates(const string& cert_file, const string& key_file);

    virtual void setName(const string& v) { m_name = v;}   //设置服务器名称

    //virtual string toString(const string& prefix = "");   //以字符串形式返回

protected:

    virtual void handleClient(Socket::ptr client);  //处理新连接的Socket类(处理新连接的客户端数据传输)

    //开始接受连接客户端连接，并安排线程处理        sock 服务器socket
    virtual void startAccept(Socket::ptr sock);

private:
    vector<Socket::ptr> m_socks;   //监听Socket数组(正在监听客户端的socket)
    IOManager* m_worker;    //正在Socket工作的调度器(类似线程池,存放已经accept后的socket fd,方便后续处理这些socket的任务)
    IOManager* m_ioWorker;  //新连接的Socket(客户端)工作的调度器(处理传输数据)
    IOManager* m_acceptWorker;  //服务器Socket接收连接的调度器(接收连接)

    uint64_t m_recvTimeout; //服务器接收超时时间(毫秒)————接收数据之类的
    string m_name; //服务器名称
    string m_type = "tcp"; //服务器类型(tcp、udp)
    bool m_isStop;  //服务器是否停止

    bool m_ssl = false; //是否使用https所有的证书

    //TcpServerConf::ptr m_conf;  //TCP服务器配置结构体
};




}



#endif