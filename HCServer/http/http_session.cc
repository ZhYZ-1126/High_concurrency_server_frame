#include "http_session.h"
#include "http_parser.h"

namespace HCServer {
namespace http {

HttpSession::HttpSession(Socket::ptr sock, bool owner)
    :SocketStream(sock, owner) {
}

//接收HTTP请求
HttpRequest::ptr HttpSession::recvRequest() {
    HttpRequestParser::ptr parser(new HttpRequestParser);
    //返回HttpRequest协议解析的缓冲区大小
    uint64_t buff_size = HttpRequestParser::GetHttpRequestBufferSize();
    //uint64_t buff_size = 100;
    shared_ptr<char> buffer(
            new char[buff_size], [](char* ptr){
                delete[] ptr;
            });
    char* data = buffer.get();
    int offset = 0;
    do {
        int len = read(data + offset, buff_size - offset);
        if(len <= 0) {
            close();
            return nullptr;
        }
        //加上之前已经接收但是还未解析的长度为offset的请求信息，才是真正的已经读取到的信息
        len += offset;
        size_t nparse = parser->execute(data, len);//返回实际解析的长度
        if(parser->hasError()) {
            close();
            return nullptr;
        }
        //从buff读到的http请求长度-实际解析的长度，得到一个已经接收但是还未解析的长度，需要重新读取那段长度的请求消息
        offset = len - nparse;
        if(offset == (int)buff_size) {
            close();
            return nullptr;
        }
        if(parser->isFinished()) {
            break;
        }
    } while(true);
    
    int64_t length = parser->getContentLength();
    if(length > 0) {
        string body;
        body.resize(length);

        int len = 0;
        if(length >= offset) {//消息体长度 > 已读取的数据中还未解析的长度(消息体)
            memcpy(&body[0], data, offset);
            len = offset;
        } else {
            memcpy(&body[0], data, length);
            len = length;
        }
        length -= offset;//还未读取的消息体数据
        if(length > 0) {
            if(readFixSize(&body[len], length) <= 0) {//读取剩下的消息体数据
                close();
                return nullptr;
            }
        }
        parser->getData()->setBody(body);//读取剩下的消息体数据
    }

    parser->getData()->init();
    return parser->getData();//返回HttpRequest结构(HTTP请求结构体)
}
//发送HTTP响应
int HttpSession::sendResponse(HttpResponse::ptr rsp) {
    stringstream ss;
    ss << *rsp;
    string data = ss.str();
    return writeFixSize(data.c_str(), data.size());
}

}
}