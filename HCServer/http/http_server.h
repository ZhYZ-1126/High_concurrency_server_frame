// HTTP服务器封装
// 继承了TcpServer，重载handleClient方法
// 将accept后得到的客户端套接字封装成HttpSession结构，以便于接收和发送HTTP消息。
// (处理HTTP服务器与客户端的请求与响应)

#ifndef __HCSERVER_HTTP_HTTP_SERVER_H__
#define __HCSERVER_HTTP_HTTP_SERVER_H__

#include "../tcp_server.h"
#include "http_session.h"
#include "servlet.h"

namespace HCServer
{
namespace http   
{

//HTTP服务器类
class HttpServer : public TcpServer
{
public:
    typedef shared_ptr<HttpServer> ptr;    //智能指针类型

    //构造函数(利用子类构造来初始化父类的变量)
    //keepalive 是否长连接  worker 工作调度器
    //io_worker 新连接的Socket(客户端)工作的调度器(处理传输数据)
    //accept_worker 接收连接调度器
    HttpServer(bool keepalive = false,HCServer::IOManager* worker = HCServer::IOManager::Getthis()
                    ,HCServer::IOManager* io_worker = HCServer::IOManager::Getthis()
                    ,HCServer::IOManager* accept_worker = HCServer::IOManager::Getthis());

    ServletDispatch::ptr getServletDispatch() const { return m_dispatch;}   //获取ServletDispatch(管理类) 

    void setServletDispatch(ServletDispatch::ptr v) { m_dispatch = v;}  //设置ServletDispatch(管理类) 

    virtual void setName(const string& v) override;    //设置服务器名称

protected:

    virtual void handleClient(Socket::ptr client) override; //处理新连接的Socket类(处理新连接的客户端数据传输)

private:

    bool m_isKeepalive; //是否支持长连接(一条链接支持多个请求和响应的发送)
    ServletDispatch::ptr m_dispatch;    //Servlet分发器(管理类)     匹配servlet

};

}
}

#endif