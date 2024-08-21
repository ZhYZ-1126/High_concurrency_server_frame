#include "http_connection.h"
#include "http_parser.h"
#include "../log.h"
//#include "../streams/zlib_stream.h"

namespace HCServer {
namespace http {

static HCServer::Logger::ptr g_logger = HCSERVER_LOG_NAME("system");

string HttpResult::toString() const {
    stringstream ss;
    ss << "[HttpResult result=" << result
       << " error=" << error
       << " response=" << (response ? response->toString() : "nullptr")
       << "]";
    return ss.str();
}

HttpConnection::HttpConnection(Socket::ptr sock, bool owner)
    :SocketStream(sock, owner) {
}

HttpConnection::~HttpConnection() {
    HCSERVER_LOG_DEBUG(g_logger) << "HttpConnection::~HttpConnection";
}

//接收HTTP响应，解析响应协议
HttpResponse::ptr HttpConnection::recvResponse() {
    HttpResponseParser::ptr parser(new HttpResponseParser);
    uint64_t buff_size = HttpResponseParser::GetHttpResponseBufferSize();
    //uint64_t buff_size = 100;
    shared_ptr<char> buffer(
            new char[buff_size + 1], [](char* ptr){
                delete[] ptr;
            });
    char* data = buffer.get();
    int offset = 0;//上一轮 已读取的数据中还未解析的长度
    do {
        int len = read(data + offset, buff_size - offset);//读取HTTP响应协议  (返回实际接收到的数据长度)
        if(len <= 0) {
            close();
            return nullptr;
        }
        len += offset; //现在 已读取的数据中还未解析的长度
        data[len] = '\0';
        size_t nparse = parser->execute(data, len, false);//解析HTTP响应协议
        if(parser->hasError()) {
            close();
            return nullptr;
        }
        offset = len - nparse;//这一轮之后 已读取的数据中还未解析的长度
        if(offset == (int)buff_size) {
            close();
            return nullptr;
        }
        if(parser->isFinished()) {
            break;
        }
    } while(true);

    auto& client_parser = parser->getParser();//返回httpclient_parser
    string body;
    if(client_parser.chunked) { //设置chunked(分块传输编码)————解析剩余的数据()
        int len = offset;//已读取的数据中还未解析的长度
        do {
            bool begin = true;
            do {
                if(!begin || len == 0) {
                    int rt = read(data + len, buff_size - len);
                    if(rt <= 0) {
                        close();
                        return nullptr;
                    }
                    len += rt;
                }
                data[len] = '\0';
                size_t nparse = parser->execute(data, len, true);//解析
                if(parser->hasError()) {
                    close();
                    return nullptr;
                }
                len -= nparse;
                if(len == (int)buff_size) {
                    close();
                    return nullptr;
                }
                begin = false;
            } while(!parser->isFinished());
            //len -= 2;
            
            HCSERVER_LOG_DEBUG(g_logger) << "content_len=" << client_parser.content_len;
            if(client_parser.content_len + 2 <= len) { //实际消息体 < 消息体
                body.append(data, client_parser.content_len);
                memmove(data, data + client_parser.content_len + 2
                        , len - client_parser.content_len - 2);
                len -= client_parser.content_len + 2;
            } else {//实际消息体 > 消息体
                body.append(data, len);
                int left = client_parser.content_len - len + 2;
                while(left > 0) {
                    int rt = read(data, left > (int)buff_size ? (int)buff_size : left);
                    if(rt <= 0) {
                        close();
                        return nullptr;
                    }
                    body.append(data, rt);
                    left -= rt;
                }
                body.resize(body.size() - 2);
                len = 0;
            }
        } while(!client_parser.chunks_done);
    } else {//不是chunked(分块传输编码)————消息体为响应协议
        int64_t length = parser->getContentLength();
        if(length > 0) {
            body.resize(length);

            int len = 0;
            if(length >= offset) {
                memcpy(&body[0], data, offset);
                len = offset;
            } else {
                memcpy(&body[0], data, length);
                len = length;
            }
            length -= offset;
            if(length > 0) {
                if(readFixSize(&body[len], length) <= 0) {
                    close();
                    return nullptr;
                }
            }
        }
    }
    if(!body.empty()) {
        auto content_encoding = parser->getData()->getHeader("content-encoding");
        HCSERVER_LOG_DEBUG(g_logger) << "content_encoding: " << content_encoding<< " size=" << body.size();
        if(strcasecmp(content_encoding.c_str(), "gzip") == 0) {
            // auto zs = ZlibStream::CreateGzip(false);
            // zs->write(body.c_str(), body.size());
            // zs->flush();
            // zs->getResult().swap(body);
        } else if(strcasecmp(content_encoding.c_str(), "deflate") == 0) {
            // auto zs = ZlibStream::CreateDeflate(false);
            // zs->write(body.c_str(), body.size());
            // zs->flush();
            // zs->getResult().swap(body);
        }
        parser->getData()->setBody(body);
    }
    return parser->getData();
}

//发送HTTP请求
int HttpConnection::sendRequest(HttpRequest::ptr req) {
    stringstream ss;
    ss << *req;
    string data = ss.str();
    //cout << ss.str() << endl;
    return writeFixSize(data.c_str(), data.size());
}

//发送HTTP的GET请求(字符串uri)
HttpResult::ptr HttpConnection::DoGet(const string& url, uint64_t timeout_ms
            , const map<string, string>& headers, const string& body) {
    Uri::ptr uri = Uri::Create(url);//创建一个uri对象并且解析网址url
    if(!uri) {
        return make_shared<HttpResult>((int)HttpResult::Error::INVALID_URL
                , nullptr, "invalid url: " + url);
    }
    return DoGet(uri, timeout_ms, headers, body);//使用参数为Uri结构体的DoGet函数
}
//发送HTTP的GET请求(结构体uri) 
HttpResult::ptr HttpConnection::DoGet(Uri::ptr uri, uint64_t timeout_ms
            , const map<string, string>& headers, const string& body) {
    return DoRequest(HttpMethod::GET, uri, timeout_ms, headers, body);//调用发送请求函数
}

//发送HTTP的POST请求(字符串uri)
HttpResult::ptr HttpConnection::DoPost(const string& url, uint64_t timeout_ms
            , const map<string, string>& headers, const string& body) {
    Uri::ptr uri = Uri::Create(url);
    if(!uri) {
        return make_shared<HttpResult>((int)HttpResult::Error::INVALID_URL
                , nullptr, "invalid url: " + url);
    }
    return DoPost(uri, timeout_ms, headers, body);
}
//发送HTTP的POST请求(结构体uri) 
HttpResult::ptr HttpConnection::DoPost(Uri::ptr uri, uint64_t timeout_ms
            , const map<string, string>& headers, const string& body) {
    return DoRequest(HttpMethod::POST, uri, timeout_ms, headers, body);
}
//发送HTTP请求(字符串uri)
HttpResult::ptr HttpConnection::DoRequest(HttpMethod method, const string& url, uint64_t timeout_ms
            , const map<string, string>& headers, const string& body) {
    Uri::ptr uri = Uri::Create(url);
    if(!uri) {
        return make_shared<HttpResult>((int)HttpResult::Error::INVALID_URL
                , nullptr, "invalid url: " + url);
    }
    return DoRequest(method, uri, timeout_ms, headers, body);
}
//发送HTTP请求(结构体uri)
HttpResult::ptr HttpConnection::DoRequest(HttpMethod method, Uri::ptr uri, uint64_t timeout_ms
            , const map<string, string>& headers, const string& body) {
    HttpRequest::ptr req = make_shared<HttpRequest>();//创建一个请求
    //设置请求的内容
    req->setPath(uri->getPath());
    req->setQuery(uri->getQuery());
    req->setFragment(uri->getFragment());
    req->setMethod(method);
    bool has_host = false;
    for(auto& i : headers) {
        //strcasecmp:忽略大小写，比较字符串是否相等
        if(strcasecmp(i.first.c_str(), "connection") == 0) {
            if(strcasecmp(i.second.c_str(), "keep-alive") == 0) {
                req->setClose(false);
            }
            continue;
        }

        if(!has_host && strcasecmp(i.first.c_str(), "host") == 0) {
            has_host = !i.second.empty();
        }

        req->setHeader(i.first, i.second);
    }
    if(!has_host) {
        req->setHeader("Host", uri->getHost());
    }
    req->setBody(body);
    return DoRequest(req, uri, timeout_ms);//将生成的请求发送出去
}
//发送HTTP请求(HttpRequest请求结构体、结构体uri)
HttpResult::ptr HttpConnection::DoRequest(HttpRequest::ptr req, Uri::ptr uri, uint64_t timeout_ms) {
    bool is_ssl = uri->getScheme() == "https";
    Address::ptr addr = uri->createAddress();
    if(!addr) {
        return make_shared<HttpResult>((int)HttpResult::Error::INVALID_HOST
                , nullptr, "invalid host: " + uri->getHost());
    }
    //Socket::ptr sock = is_ssl ? SSLSocket::CreateTCP(addr) : Socket::CreateTCP(addr);
    Socket::ptr sock=Socket::CreateTCP(addr);
    if(!sock) {//socket创建失败
        return make_shared<HttpResult>((int)HttpResult::Error::CREATE_SOCKET_ERROR
                , nullptr, "create socket fail: " + addr->toString()
                        + " errno=" + to_string(errno)
                        + " errstr=" + string(strerror(errno)));
    }
    if(!sock->connect(addr)) {//客户端连接服务器端
        return make_shared<HttpResult>((int)HttpResult::Error::CONNECT_FAIL
                , nullptr, "connect fail: " + addr->toString());
    }
    sock->setRecvTimeout(timeout_ms);//设置接收超时时间
    HttpConnection::ptr conn = make_shared<HttpConnection>(sock);//创建一个连接
    int rt = conn->sendRequest(req);//发送请求
    if(rt == 0) {
        return make_shared<HttpResult>((int)HttpResult::Error::SEND_CLOSE_BY_PEER
                , nullptr, "send request closed by peer: " + addr->toString());
    }
    if(rt < 0) {
        return make_shared<HttpResult>((int)HttpResult::Error::SEND_SOCKET_ERROR
                    , nullptr, "send request socket error errno=" + to_string(errno)
                    + " errstr=" + string(strerror(errno)));
    }
    auto rsp = conn->recvResponse();//接收响应
    if(!rsp) {
        return make_shared<HttpResult>((int)HttpResult::Error::TIMEOUT
                    , nullptr, "recv response timeout: " + addr->toString()
                    + " timeout_ms:" + to_string(timeout_ms));
    }
    return make_shared<HttpResult>((int)HttpResult::Error::OK, rsp, "ok");
}


//HttpConnectionPool的方法
//创建HttpConnectionPool(HTTP客户端连接池)
HttpConnectionPool::ptr HttpConnectionPool::Create(const string& uri,const string& vhost
                ,uint32_t max_size,uint32_t max_alive_time,uint32_t max_request) {
    Uri::ptr turi = Uri::Create(uri);
    if(!turi) {
        HCSERVER_LOG_ERROR(g_logger) << "invalid uri=" << uri;
    }
    return make_shared<HttpConnectionPool>(turi->getHost()
            , vhost, turi->getPort(), turi->getScheme() == "https"
            , max_size, max_alive_time, max_request);
}
//构造函数
HttpConnectionPool::HttpConnectionPool(const string& host,const string& vhost,uint32_t port
        ,bool is_https,uint32_t max_size,uint32_t max_alive_time,uint32_t max_request)
    :m_host(host)
    ,m_vhost(vhost)
    ,m_port(port ? port : (is_https ? 443 : 80))
    ,m_maxSize(max_size)
    ,m_maxAliveTime(max_alive_time)
    ,m_maxRequest(max_request)
    ,m_isHttps(is_https) {
}
//从连接池取出链接，没有，则自动根据host创建链接
HttpConnection::ptr HttpConnectionPool::getConnection() {
    uint64_t now_ms = HCServer::GetcurrentMS();
    vector<HttpConnection*> invalid_conns;//无效连接
    HttpConnection* ptr = nullptr;
    MutexType::Lock lock(m_mutex);
    while(!m_conns.empty()) {//遍历整个连接池
        auto conn = *m_conns.begin();
        m_conns.pop_front();
        //两个if实现对当前拿到的连接进行判断，判断是否是有效的连接，有效则使用该连接
        if(!conn->isConnected()) {//该连接无效
            invalid_conns.push_back(conn);
            continue;
        }
        if((conn->m_createTime + m_maxAliveTime) > now_ms) {//连接的时间超时
            invalid_conns.push_back(conn);
            continue;
        }
        ptr = conn;//拿到连接
        break;
    }
    lock.unlock();
    for(auto i : invalid_conns) {//释放掉无效的连接
        delete i;
    }
    m_total -= invalid_conns.size();

    if(!ptr) {//如果一个连接都没有就直接创建一个连接
        //通过主机名创建一个地址
        IPAddress::ptr addr = Address::LookupAnyIPAddress(m_host);
        if(!addr) {
            HCSERVER_LOG_ERROR(g_logger) << "get addr fail: " << m_host;
            return nullptr;
        }
        addr->setPort(m_port);//设置端口
        //Socket::ptr sock = m_isHttps ? SSLSocket::CreateTCP(addr) : Socket::CreateTCP(addr);
        Socket::ptr sock=Socket::CreateTCP(addr);//通过地址创建一个用于TCP的socket
        if(!sock) {
            HCSERVER_LOG_ERROR(g_logger) << "create sock fail: " << *addr;
            return nullptr;
        }
        if(!sock->connect(addr)) {//客户端连接服务器
            HCSERVER_LOG_ERROR(g_logger) << "sock connect fail: " << *addr;
            return nullptr;
        }

        ptr = new HttpConnection(sock);
        ++m_total;
    }
    return HttpConnection::ptr(ptr, bind(&HttpConnectionPool::ReleasePtr, placeholders::_1, this));
}
//释放连接池指针()
void HttpConnectionPool::ReleasePtr(HttpConnection* ptr, HttpConnectionPool* pool) {
    ++ptr->m_request;
    //满足条件就释放当前连接
    if(!ptr->isConnected()|| ((ptr->m_createTime + pool->m_maxAliveTime) >= HCServer::GetcurrentMS()
            || (ptr->m_request >= pool->m_maxRequest))) {
        delete ptr;
        --pool->m_total;
        return;
    }
    MutexType::Lock lock(pool->m_mutex);
    pool->m_conns.push_back(ptr);
}
//发送HTTP的GET请求(字符串uri)
HttpResult::ptr HttpConnectionPool::doGet(const string& url, uint64_t timeout_ms
            , const map<string, string>& headers, const string& body) {
    return doRequest(HttpMethod::GET, url, timeout_ms, headers, body);
}
//发送HTTP的GET请求(结构体uri)
HttpResult::ptr HttpConnectionPool::doGet(Uri::ptr uri, uint64_t timeout_ms
            , const map<string, string>& headers, const string& body) {
    stringstream ss;
    ss << uri->getPath()
       << (uri->getQuery().empty() ? "" : "?")
       << uri->getQuery()
       << (uri->getFragment().empty() ? "" : "#")
       << uri->getFragment();
    return doGet(ss.str(), timeout_ms, headers, body);
}
//发送HTTP的POST请求(字符串uri)
HttpResult::ptr HttpConnectionPool::doPost(const string& url, uint64_t timeout_ms
            , const map<string, string>& headers, const string& body) {
    return doRequest(HttpMethod::POST, url, timeout_ms, headers, body);
}
//发送HTTP的POST请求(结构体uri)
HttpResult::ptr HttpConnectionPool::doPost(Uri::ptr uri, uint64_t timeout_ms
            , const map<string, string>& headers, const string& body) {
    stringstream ss;
    ss << uri->getPath()
       << (uri->getQuery().empty() ? "" : "?")
       << uri->getQuery()
       << (uri->getFragment().empty() ? "" : "#")
       << uri->getFragment();
    return doPost(ss.str(), timeout_ms, headers, body);
}
//发送HTTP请求(字符串uri) 
HttpResult::ptr HttpConnectionPool::doRequest(HttpMethod method, const string& url
            , uint64_t timeout_ms, const map<string, string>& headers, const string& body) {
    HttpRequest::ptr req = make_shared<HttpRequest>();
    req->setPath(url);
    req->setMethod(method);
    req->setClose(false);
    bool has_host = false;
    for(auto& i : headers) {
        if(strcasecmp(i.first.c_str(), "connection") == 0) {
            if(strcasecmp(i.second.c_str(), "keep-alive") == 0) {
                req->setClose(false);
            }
            continue;
        }

        if(!has_host && strcasecmp(i.first.c_str(), "host") == 0) {
            has_host = !i.second.empty();
        }

        req->setHeader(i.first, i.second);
    }
    if(!has_host) {
        if(m_vhost.empty()) {
            req->setHeader("Host", m_host);
        } else {
            req->setHeader("Host", m_vhost);
        }
    }
    req->setBody(body);
    return doRequest(req, timeout_ms);
}
 //发送HTTP请求(结构体uri)
HttpResult::ptr HttpConnectionPool::doRequest(HttpMethod method, Uri::ptr uri
            , uint64_t timeout_ms, const map<string, string>& headers, const string& body) {
    stringstream ss;
    ss << uri->getPath()
       << (uri->getQuery().empty() ? "" : "?")
       << uri->getQuery()
       << (uri->getFragment().empty() ? "" : "#")
       << uri->getFragment();
    return doRequest(method, ss.str(), timeout_ms, headers, body);
}
//发送HTTP请求
HttpResult::ptr HttpConnectionPool::doRequest(HttpRequest::ptr req, uint64_t timeout_ms) {
    auto conn = getConnection();//拿到连接
    if(!conn) {
        return make_shared<HttpResult>((int)HttpResult::Error::POOL_GET_CONNECTION
                , nullptr, "pool host:" + m_host + " port:" + to_string(m_port));
    }
    auto sock = conn->getSocket();//返回Socket类
    if(!sock) {
        return make_shared<HttpResult>((int)HttpResult::Error::POOL_INVALID_CONNECTION
                , nullptr, "pool host:" + m_host + " port:" + to_string(m_port));
    }
    sock->setRecvTimeout(timeout_ms);//设置读取超时时间
    int rt = conn->sendRequest(req);//发送请求
    if(rt == 0) {
        return make_shared<HttpResult>((int)HttpResult::Error::SEND_CLOSE_BY_PEER
                , nullptr, "send request closed by peer: " + sock->getRemoteAddress()->toString());
    }
    if(rt < 0) {
        return make_shared<HttpResult>((int)HttpResult::Error::SEND_SOCKET_ERROR
                    , nullptr, "send request socket error errno=" + to_string(errno)
                    + " errstr=" + string(strerror(errno)));
    }
    auto rsp = conn->recvResponse();//读取响应
    if(!rsp) {
        return make_shared<HttpResult>((int)HttpResult::Error::TIMEOUT
                    , nullptr, "recv response timeout: " + sock->getRemoteAddress()->toString()
                    + " timeout_ms:" + to_string(timeout_ms));
    }
    return make_shared<HttpResult>((int)HttpResult::Error::OK, rsp, "ok");
}

}
}