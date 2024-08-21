#include "tcp_server.h"
#include "config.h"
#include "log.h"
using namespace std;


namespace HCServer {

static HCServer::ConfigVar<uint64_t>::ptr g_tcp_server_read_timeout =
    HCServer::Config::Lookup("tcp_server.read_timeout", (uint64_t)(60 * 1000 * 2),"tcp server read timeout");

static HCServer::Logger::ptr g_logger = HCSERVER_LOG_NAME("system");

TcpServer::TcpServer(HCServer::IOManager* worker,HCServer::IOManager* io_worker,HCServer::IOManager* accept_worker)
    :m_worker(worker)
    ,m_ioWorker(io_worker)
    ,m_acceptWorker(accept_worker)
    ,m_recvTimeout(g_tcp_server_read_timeout->getValue())
    ,m_name("HCServer/1.0.0")
    ,m_isStop(true) {
}

TcpServer::~TcpServer() {
    for(auto& i : m_socks) {
        i->close();
    }
    m_socks.clear();
}

// //设置TCP服务器配置结构体
// void TcpServer::setConf(const TcpServerConf& v) {
//     m_conf.reset(new TcpServerConf(v));
// }

//绑定地址数组(并开启监听)  
bool TcpServer::bind(HCServer::Address::ptr addr, bool ssl) {
    vector<Address::ptr> addrs;
    vector<Address::ptr> fails;
    addrs.push_back(addr);
    return bind(addrs, fails, ssl);
}

//绑定地址数组(并开启监听)   fails 绑定失败的地址
bool TcpServer::bind(const vector<Address::ptr>& addrs,vector<Address::ptr>& fails,bool ssl) {
    m_ssl = ssl;
    for(auto& addr : addrs) {
        //创建对应类型的tcp socket
        //Socket::ptr sock = ssl ? SSLSocket::CreateTCP(addr) : Socket::CreateTCP(addr);
        Socket::ptr sock =Socket::CreateTCP(addr);
        //绑定地址
        if(!sock->bind(addr)) {
            HCSERVER_LOG_ERROR(g_logger) << "bind fail errno="
                << errno << " errstr=" << strerror(errno)
                << " addr=[" << addr->toString() << "]";
            fails.push_back(addr);
            continue;
        }
        //监听socket
        if(!sock->listen()) {
            HCSERVER_LOG_ERROR(g_logger) << "listen fail errno="
                << errno << " errstr=" << strerror(errno)
                << " addr=[" << addr->toString() << "]";
            fails.push_back(addr);
            continue;
        }
        m_socks.push_back(sock);//如果两个都成功了则添加到监听的socket数组中
    }

    if(!fails.empty()) {
        m_socks.clear();
        return false;
    }

    for(auto& i : m_socks) {
        HCSERVER_LOG_INFO(g_logger) << "type=" << m_type
            << " name=" << m_name
            << " ssl=" << m_ssl
            << " server bind success: " << *i;
    }
    return true;
}

void TcpServer::startAccept(Socket::ptr sock) {
    while(!m_isStop) {
        Socket::ptr client = sock->accept();
        if(client) {
            client->setRecvTimeout(m_recvTimeout);
            m_ioWorker->schedule(std::bind(&TcpServer::handleClient,shared_from_this(), client));//添加协程任务
        } else {
            HCSERVER_LOG_ERROR(g_logger) << "accept errno=" << errno
                << " errstr=" << strerror(errno);
        }
    }
}

bool TcpServer::start() {
    if(!m_isStop) {
        return true;
    }
    m_isStop = false;
    for(auto& sock : m_socks) {
        m_acceptWorker->schedule(std::bind(&TcpServer::startAccept,shared_from_this(), sock));//添加协程任务
    }
    return true;
}

void TcpServer::stop() {
    m_isStop = true;
    auto self = shared_from_this();
    m_acceptWorker->schedule([this, self]() {
        for(auto& sock : m_socks) {
            sock->cancelAll();
            sock->close();
        }
        m_socks.clear();
    });
}

void TcpServer::handleClient(Socket::ptr client) {
    HCSERVER_LOG_INFO(g_logger) << "handleClient: " << *client;
}

// bool TcpServer::loadCertificates(const string& cert_file, const string& key_file) {
//     for(auto& i : m_socks) {
//         auto ssl_socket = dynamic_pointer_cast<SSLSocket>(i);
//         if(ssl_socket) {
//             if(!ssl_socket->loadCertificates(cert_file, key_file)) {
//                 return false;
//             }
//         }
//     }
//     return true;
// }

// string TcpServer::toString(const string& prefix) {
//     stringstream ss;
//     ss << prefix << "[type=" << m_type
//        << " name=" << m_name << " ssl=" << m_ssl
//        << " worker=" << (m_worker ? m_worker->getname() : "")
//        << " accept=" << (m_acceptWorker ? m_acceptWorker->getname() : "")
//        << " recv_timeout=" << m_recvTimeout << "]" << endl;
//     string pfx = prefix.empty() ? "    " : prefix;
//     for(auto& i : m_socks) {
//         ss << pfx << pfx << *i << endl;
//     }
//     return ss.str();
// }



}