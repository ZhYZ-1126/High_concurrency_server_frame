#include "servlet.h"
#include <fnmatch.h>
using namespace std;

namespace HCServer {
namespace http {

FunctionServlet::FunctionServlet(callback cb)
    :Servlet("FunctionServlet")
    ,m_cb(cb) {
}

//处理请求: request HTTP请求、 response HTTP响应、 session HTTP连接
int32_t FunctionServlet::handle(HCServer::http::HttpRequest::ptr request
               , HCServer::http::HttpResponse::ptr response
               , HCServer::http::HttpSession::ptr session) {
    return m_cb(request, response, session);
}


ServletDispatch::ServletDispatch()
    :Servlet("ServletDispatch") {
    m_default.reset(new NotFoundServlet("HCServer/1.0"));
}

//处理请求: request HTTP请求、 response HTTP响应、 session HTTP连接
int32_t ServletDispatch::handle(HCServer::http::HttpRequest::ptr request
               , HCServer::http::HttpResponse::ptr response
               , HCServer::http::HttpSession::ptr session) {
    auto slt = getMatchedServlet(request->getPath());
    if(slt) {
        slt->handle(request, response, session);
    }
    return 0;
}

//添加精准匹配servlet(普通) uri uri路径 slt serlvet
void ServletDispatch::addServlet(const string& uri, Servlet::ptr slt) {
    RWMutexType::Writelock lock(m_mutex);
    m_datas[uri] = make_shared<HoldServletCreator>(slt);
}
//添加精准匹配servlet(回调函数)    uri uri路径     cb FunctionServlet回调函数
void ServletDispatch::addServlet(const string& uri,FunctionServlet::callback cb) {
    RWMutexType::Writelock lock(m_mutex);
    m_datas[uri] = make_shared<HoldServletCreator>(make_shared<FunctionServlet>(cb));
}
//添加精准匹配servlet(Servlet指针封装)  uri uri路径     creator IServletCreator
void ServletDispatch::addServletCreator(const string& uri, IServletCreator::ptr creator) {
    RWMutexType::Writelock lock(m_mutex);
    m_datas[uri] = creator;
}


//添加模糊匹配servlet(普通) uri uri路径 slt serlvet
void ServletDispatch::addGlobServlet(const string& uri,Servlet::ptr slt) {
    RWMutexType::Writelock lock(m_mutex);
    for(auto it = m_globs.begin();it != m_globs.end(); ++it) {
        if(it->first == uri) {
            m_globs.erase(it);
            break;
        }
    }
    m_globs.push_back(make_pair(uri, make_shared<HoldServletCreator>(slt)));
}
//添加模糊匹配servlet(回调函数) uri uri路径     cb FunctionServlet回调函数
void ServletDispatch::addGlobServlet(const string& uri,FunctionServlet::callback cb) {
    return addGlobServlet(uri, make_shared<FunctionServlet>(cb));
}
//添加精准匹配servlet(Servlet指针封装)  uri uri路径     creator IServletCreator
void ServletDispatch::addGlobServletCreator(const string& uri, IServletCreator::ptr creator) {
    RWMutexType::Writelock lock(m_mutex);
    for(auto it = m_globs.begin();
            it != m_globs.end(); ++it) {
        if(it->first == uri) {
            m_globs.erase(it);
            break;
        }
    }
    m_globs.push_back(make_pair(uri, creator));
}


//删除精准匹配servlet   uri uri路径
void ServletDispatch::delServlet(const string& uri) {
    RWMutexType::Writelock lock(m_mutex);
    m_datas.erase(uri);
}
//删除模糊匹配servlet   uri uri路径
void ServletDispatch::delGlobServlet(const string& uri) {
    RWMutexType::Writelock lock(m_mutex);
    for(auto it = m_globs.begin();it != m_globs.end(); ++it) {
        if(it->first == uri) {
            m_globs.erase(it);
            break;
        }
    }
}

//通过uri获取精准匹配servlet（直接访问m_datas）    uri uri路径
Servlet::ptr ServletDispatch::getServlet(const string& uri) {
    RWMutexType::Readlock lock(m_mutex);
    auto it = m_datas.find(uri);
    return it == m_datas.end() ? nullptr : it->second->get();
}
//通过uri获取模糊匹配servlet（直接访问m_globs）    uri uri路径
Servlet::ptr ServletDispatch::getGlobServlet(const string& uri) {
    RWMutexType::Readlock lock(m_mutex);
    for(auto it = m_globs.begin();it != m_globs.end(); ++it) {
        if(it->first == uri) {
            return it->second->get();
        }
    }
    return nullptr;
}

//通过uri获取servlet    uri uri路径
// return:优先精准匹配,其次模糊匹配,最后返回默认
Servlet::ptr ServletDispatch::getMatchedServlet(const string& uri) {
    RWMutexType::Readlock lock(m_mutex);
    auto mit = m_datas.find(uri);
    if(mit != m_datas.end()) {//优先精准匹配
        return mit->second->get();
    }
    for(auto it = m_globs.begin();it != m_globs.end(); ++it) {//其次模糊匹配
        //fnmatch就是判断字符串uri.c_str()是不是符合it->first.c_str()所指的结构
        //fnmatch能够实现模糊匹配，返回0则uri.c_str()匹配it->first.c_str()
        if(!fnmatch(it->first.c_str(), uri.c_str(), 0)) {
            return it->second->get();
        }
    }
    return m_default;//最后返回默认
}

//复制精准匹配servlet MAP
void ServletDispatch::listAllServletCreator(map<string, IServletCreator::ptr>& infos) {
    RWMutexType::Readlock lock(m_mutex);
    for(auto& i : m_datas) {
        infos[i.first] = i.second;
    }
}
//复制模糊匹配servlet 数组
void ServletDispatch::listAllGlobServletCreator(map<string, IServletCreator::ptr>& infos) {
    RWMutexType::Readlock lock(m_mutex);
    for(auto& i : m_globs) {
        infos[i.first] = i.second;
    }
}

NotFoundServlet::NotFoundServlet(const string& name)
    :Servlet("NotFoundServlet")
    ,m_name(name) {
    m_content = "<html><head><title>404 Not Found"
        "</title></head><body><center><h1>404 Not Found</h1></center>"
        "<hr><center>" + name + "</center></body></html>";

}

//处理请求: request HTTP请求、 response HTTP响应、 session HTTP连接
int32_t NotFoundServlet::handle(HCServer::http::HttpRequest::ptr request
                   , HCServer::http::HttpResponse::ptr response
                   , HCServer::http::HttpSession::ptr session) {
    response->setStatus(HCServer::http::HttpStatus::NOT_FOUND);
    response->setHeader("Server", "HCServer/1.0.0");
    response->setHeader("Content-Type", "text/html");
    response->setBody(m_content);
    return 0;
}


}
}