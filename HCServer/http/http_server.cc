#include "http_server.h"
#include "../log.h"
// #include "sylar/http/servlets/config_servlet.h"
// #include "sylar/http/servlets/status_servlet.h"

namespace HCServer {
namespace http {

static HCServer::Logger::ptr g_logger = HCSERVER_LOG_NAME("system");

HttpServer::HttpServer(bool keepalive,HCServer::IOManager* worker,HCServer::IOManager* io_worker,HCServer::IOManager* accept_worker)
    :TcpServer(worker, io_worker, accept_worker)
    ,m_isKeepalive(keepalive) {
    m_dispatch.reset(new ServletDispatch);

    //m_type = "http";
    //添加精准匹配servlet(普通) uri uri路径 slt serlvet
    //m_dispatch->addServlet("/_/status", Servlet::ptr(new StatusServlet));
    //m_dispatch->addServlet("/_/config", Servlet::ptr(new ConfigServlet));
}

void HttpServer::setName(const std::string& v) {
    TcpServer::setName(v);
    m_dispatch->setDefault(make_shared<NotFoundServlet>(v));
}

//处理新连接的Socket类(处理新连接的客户端数据传输)
//client :accept()后得到的客户端的socket
void HttpServer::handleClient(Socket::ptr client) {
    HCSERVER_LOG_DEBUG(g_logger) << "HttpServer handleClient " << *client;
    HttpSession::ptr session(new HttpSession(client));
    do {
        auto req = session->recvRequest();//接收请求信息
        if(!req) {
            HCSERVER_LOG_DEBUG(g_logger) << "recv http request fail, errno="
                << errno << " errstr=" << strerror(errno)
                << " cliet:" << *client << " keep_alive=" << m_isKeepalive;
            break;
        }

        //创建一个响应信息
        HttpResponse::ptr rsp(new HttpResponse(req->getVersion(),req->isClose() || !m_isKeepalive));
        rsp->setBody("hello zhangyanzhou");
        rsp->setHeader("Server", getName());
        m_dispatch->handle(req, rsp, session);
        session->sendResponse(rsp);

        if(!m_isKeepalive || req->isClose()) {
            break;
        }
    } while(true);
    session->close();
}

}
}