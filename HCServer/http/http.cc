#include"http.h"
#include "../util.h"
using namespace std;



namespace HCServer{

namespace http{

HttpMethod StringToHttpMethod(const string& m) {//将字符串方法名转成HTTP请求方法枚举
#define XX(num, name, string) \
    if(strcmp(#string, m.c_str()) == 0) { \
        return HttpMethod::name; \
    }
    HTTP_METHOD_MAP(XX);
#undef XX
    return HttpMethod::INVALID_METHOD;
}

HttpMethod CharsToHttpMethod(const char* m) {//将字符串指针转换成HTTP请求方法枚举
#define XX(num, name, string) \
    if(strncmp(#string, m, strlen(#string)) == 0) { \
        return HttpMethod::name; \
    }
    HTTP_METHOD_MAP(XX);
#undef XX
    return HttpMethod::INVALID_METHOD;
}

static const char* s_method_string[] = {
#define XX(num, name, string) #string,
    HTTP_METHOD_MAP(XX)
#undef XX
};

const char* HttpMethodToString(const HttpMethod& m) {//将HTTP方法枚举转换成字符串
    uint32_t idx = (uint32_t)m;
    if(idx >= (sizeof(s_method_string) / sizeof(s_method_string[0]))) {
        return "<unknown>";
    }
    return s_method_string[idx];
}

const char* HttpStatusToString(const HttpStatus& s) {//将HTTP响应状态枚举转换成字符串
    switch(s) {
#define XX(code, name, msg) \
        case HttpStatus::name: \
            return #msg;
        HTTP_STATUS_MAP(XX);
#undef XX
        default:
            return "<unknown>";
    }
}

//忽略大小写比较是否相等仿函数
bool CaseInsensitiveLess::operator()(const string& lhs
                            ,const string& rhs) const {
    //strcasecmp()判断字符串是否相等，而且不分大小写，在比较之前全部转成小写
    return strcasecmp(lhs.c_str(), rhs.c_str()) < 0;
}


HttpRequest::HttpRequest(uint8_t version, bool close)
    :m_method(HttpMethod::GET)
    ,m_version(version)
    ,m_close(close)
    ,m_websocket(false)
    ,m_parserParamFlag(0)
    ,m_path("/") {
}

//获取HTTP请求的头部参数
string HttpRequest::getHeader(const string& key,const string& def) const {
    auto it = m_headers.find(key);
    return it == m_headers.end() ? def : it->second;
}

//
shared_ptr<HttpResponse> HttpRequest::createResponse() {
    HttpResponse::ptr rsp(new HttpResponse(getVersion(),isClose()));
    return rsp;
}

//获取HTTP请求的请求参数
string HttpRequest::getParam(const string& key,const string& def) {
    // initQueryParam();
    // initBodyParam();
    auto it = m_params.find(key);
    return it == m_params.end() ? def : it->second;
}

//获取HTTP请求的Cookie参数
string HttpRequest::getCookie(const string& key,const string& def) {
    //initCookies();
    auto it = m_cookies.find(key);
    return it == m_cookies.end() ? def : it->second;
}

//设置HTTP请求的头部参数 
void HttpRequest::setHeader(const string& key, const string& val) {
    m_headers[key] = val;
}

//设置HTTP请求的请求参数
void HttpRequest::setParam(const string& key, const string& val) {
    m_params[key] = val;
}

//设置HTTP请求的Cookie参数
void HttpRequest::setCookie(const string& key, const string& val) {
    m_cookies[key] = val;
}

// 删除HTTP请求的头部参数
void HttpRequest::delHeader(const string& key) {
    m_headers.erase(key);
}

//删除HTTP请求的请求参数
void HttpRequest::delParam(const string& key) {
    m_params.erase(key);
}

//删除HTTP请求的Cookie参数
void HttpRequest::delCookie(const string& key) {
    m_cookies.erase(key);
}

//判断HTTP请求的头部参数是否存在
bool HttpRequest::hasHeader(const string& key, string* val) {
    auto it = m_headers.find(key);
    if(it == m_headers.end()) {
        return false;
    }
    if(val) {
        *val = it->second;
    }
    return true;
}

//判断HTTP请求的请求参数是否存在
bool HttpRequest::hasParam(const string& key, string* val) {
    // initQueryParam();
    // initBodyParam();
    auto it = m_params.find(key);
    if(it == m_params.end()) {
        return false;
    }
    if(val) {
        *val = it->second;
    }
    return true;
}

//判断HTTP请求的Cookie参数是否存在
bool HttpRequest::hasCookie(const string& key, string* val) {
    //initCookies();
    auto it = m_cookies.find(key);
    if(it == m_cookies.end()) {
        return false;
    }
    if(val) {
        *val = it->second;
    }
    return true;
}

//转成字符串类型
string HttpRequest::toString() const {
    stringstream ss;
    dump(ss);
    return ss.str();
}

//序列化输出到流中
ostream& HttpRequest::dump(ostream& os) const {
    //GET /uri HTTP/1.1
    //Host: wwww.sylar.top
    //
    //
    os << HttpMethodToString(m_method) << " "
       << m_path
       << (m_query.empty() ? "" : "?")
       << m_query
       << (m_fragment.empty() ? "" : "#")
       << m_fragment
       << " HTTP/"
       << ((uint32_t)(m_version >> 4))
       << "."
       << ((uint32_t)(m_version & 0x0F))
       << "\r\n";
    if(!m_websocket) {
        os << "connection: " << (m_close ? "close" : "keep-alive") << "\r\n";
    }
    for(auto& i : m_headers) {
        if(!m_websocket && strcasecmp(i.first.c_str(), "connection") == 0) {
            continue;
        }
        os << i.first << ": " << i.second << "\r\n";
    }

    if(!m_body.empty()) {
        os << "content-length: " << m_body.size() << "\r\n\r\n"
           << m_body;
    } else {
        os << "\r\n";
    }
    return os;
}


void HttpRequest::init() {
    string conn = getHeader("connection");
    if(!conn.empty()) {
        if(strcasecmp(conn.c_str(), "keep-alive") == 0) {
            m_close = false;
        } else {
            m_close = true;
        }
    }
}

// void HttpRequest::initParam() {
//     initQueryParam();
//     initBodyParam();
//     initCookies();
// }

// void HttpRequest::initQueryParam() {
//     if(m_parserParamFlag & 0x1) {
//         return;
//     }

// #define PARSE_PARAM(str, m, flag, trim) \
//     size_t pos = 0; \
//     do { \
//         size_t last = pos; \
//         pos = str.find('=', pos); \
//         if(pos == string::npos) { \
//             break; \
//         } \
//         size_t key = pos; \
//         pos = str.find(flag, pos); \
//         m.insert(make_pair(trim(str.substr(last, key - last)), \
//                     sylar::StringUtil::UrlDecode(str.substr(key + 1, pos - key - 1)))); \
//         if(pos == string::npos) { \
//             break; \
//         } \
//         ++pos; \
//     } while(true);

//     PARSE_PARAM(m_query, m_params, '&',);
//     m_parserParamFlag |= 0x1;
// }

// void HttpRequest::initBodyParam() {
//     if(m_parserParamFlag & 0x2) {
//         return;
//     }
//     string content_type = getHeader("content-type");
//     if(strcasestr(content_type.c_str(), "application/x-www-form-urlencoded") == nullptr) {
//         m_parserParamFlag |= 0x2;
//         return;
//     }
//     PARSE_PARAM(m_body, m_params, '&',);
//     m_parserParamFlag |= 0x2;
// }

// void HttpRequest::initCookies() {
//     if(m_parserParamFlag & 0x4) {
//         return;
//     }
//     string cookie = getHeader("cookie");
//     if(cookie.empty()) {
//         m_parserParamFlag |= 0x4;
//         return;
//     }
//     PARSE_PARAM(cookie, m_cookies, ';', sylar::StringUtil::Trim);
//     m_parserParamFlag |= 0x4;
// }


HttpResponse::HttpResponse(uint8_t version, bool close)
    :m_status(HttpStatus::OK)
    ,m_version(version)
    ,m_close(close)
    ,m_websocket(false) {
}

// 获取响应头部参数     key 关键字      def 默认值
string HttpResponse::getHeader(const string& key, const string& def) const {
    auto it = m_headers.find(key);
    return it == m_headers.end() ? def : it->second;
}

//设置响应头部参数      key 关键字      val 值
void HttpResponse::setHeader(const string& key, const string& val) {
    m_headers[key] = val;
}

//删除响应头部参数      key 关键字  
void HttpResponse::delHeader(const string& key) {
    m_headers.erase(key);
}

//设置响应头部参数
void HttpResponse::setRedirect(const string& uri) {
    m_status = HttpStatus::FOUND;
    setHeader("Location", uri);
}

//设置响应Cookie vector(存储在用户主机中的文本文件)
// void HttpResponse::setCookie(const string& key, const string& val,
//                              time_t expired, const string& path,
//                              const string& domain, bool secure) {
//     stringstream ss;
//     ss << key << "=" << val;
//     if(expired > 0) {
//         ss << ";expires=" << HCServer::Time2Str(expired, "%a, %d %b %Y %H:%M:%S") << " GMT";
//     }
//     if(!domain.empty()) {
//         ss << ";domain=" << domain;
//     }
//     if(!path.empty()) {
//         ss << ";path=" << path;
//     }
//     if(secure) {
//         ss << ";secure";
//     }
//     m_cookies.push_back(ss.str());
// }


//转成字符串
string HttpResponse::toString() const {
    stringstream ss;
    dump(ss);
    return ss.str();
}

//序列化输出到流 
ostream& HttpResponse::dump(ostream& os) const {
    os << "HTTP/"
       << ((uint32_t)(m_version >> 4))
       << "."
       << ((uint32_t)(m_version & 0x0F))
       << " "
       << (uint32_t)m_status
       << " "
       << (m_reason.empty() ? HttpStatusToString(m_status) : m_reason)
       << "\r\n";

    for(auto& i : m_headers) {
        if(!m_websocket && strcasecmp(i.first.c_str(), "connection") == 0) {
            continue;
        }
        os << i.first << ": " << i.second << "\r\n";
    }
    for(auto& i : m_cookies) {
        os << "Set-Cookie: " << i << "\r\n";
    }
    if(!m_websocket) {
        os << "connection: " << (m_close ? "close" : "keep-alive") << "\r\n";
    }
    if(!m_body.empty()) {
        os << "content-length: " << m_body.size() << "\r\n\r\n"
           << m_body;
    }else {
        os << "\r\n";
    }
    return os;
}

//流式输出HttpRequest   os 输出流   req HTTP请求
ostream& operator<<(ostream& os, const HttpRequest& req) {
    return req.dump(os);
}

//流式输出HttpResponse  os 输出流   rsp HTTP响应
ostream& operator<<(ostream& os, const HttpResponse& rsp) {
    return rsp.dump(os);
}





}



}
