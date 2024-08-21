#ifndef __HCSERVER_HTTP_SESSION_H__
#define __HCSERVER_HTTP_SESSION_H__

#include "../streams/socket_stream.h"
#include "http.h"

namespace HCServer {
namespace http {

//  HttpSession封装
//解析HTTP请求协议，并转发响应协议
//accept后产生socket属于session
class HttpSession : public SocketStream {
public:
    /// 智能指针类型定义
    typedef shared_ptr<HttpSession> ptr;

    //sock Socket类型       owner 是否托管
    HttpSession(Socket::ptr sock, bool owner = true);

    //接收HTTP请求
    HttpRequest::ptr recvRequest();

    //发送HTTP响应
    int sendResponse(HttpResponse::ptr rsp);
};

}
}

#endif