//  URI封装类
//解析URI网址(http://www.sylar.top/test/uri?id=100&name=sylar#frg),并判断是否是对的
/*
        scheme://userinfo@host:port path?query#fragment

        http://user@HCServer.com:8042/over/there?name=ferret#nose
        \_/   \_____________________/\_________/ \_________/ \__/
         |              |                |           |        |
        scheme      authority           path       query    fragment
*/
#ifndef __HCSERVER_URI_H__
#define __HCSERVER_URI_H__

#include <memory>
#include <string>
#include <stdint.h>
#include "address.h"

namespace HCServer
{
//URI类
class Uri
{
public:
    typedef shared_ptr<Uri> ptr;   //智能指针类型定义

    //创建Uri对象，并解析uri(网址)  uri uri字符串
    static Uri::ptr Create(const string& uri);

    //构造函数
    Uri();

    const string& getScheme() const { return m_scheme;}    //返回scheme
    const string& getUserinfo() const { return m_userinfo;}    //返回用户信息
    const string& getHost() const { return m_host;}    //返回host
    const string& getPath() const; //返回路径
    const string& getQuery() const { return m_query;}  //返回查询条件
    const string& getFragment() const { return m_fragment;}    //返回fragment
    int32_t getPort() const;    //返回端口

    void setScheme(const string& v) { m_scheme = v;}   //设置scheme
    void setUserinfo(const string& v) { m_userinfo = v;}   //设置用户信息
    void setHost(const string& v) { m_host = v;}   //设置host信息
    void setPath(const string& v) { m_path = v;}   //设置路径
    void setQuery(const string& v) { m_query = v;} //设置查询条件
    void setFragment(const string& v) { m_fragment = v;}   //设置fragment
    void setPort(int32_t v) { m_port = v;}  //设置端口号

    ostream& dump(ostream& os) const; //序列化到输出流

    string toString() const;   //转成字符串

    Address::ptr createAddress() const; //根据uri类，获取Address

private:
    bool isDefaultPort() const; //是否默认端口(0、http 80、https 443)

private:
    /*
            scheme://userinfo@host:port path?query#fragment

            http://user@HCServer.com:8042/over/there?name=ferret#nose
            \_/   \_____________________/\_________/ \_________/ \__/
             |              |                 |           |        |
            scheme      authority           path       query    fragment
    */
    string m_scheme;   //schema
    //authority=m_userinfo+m_host+m_port
    string m_userinfo; //用户信息
    string m_host; //host
    string m_path; //路径
    string m_query;    //查询参数
    string m_fragment; //fragment
    int32_t m_port; //端口
};

}

#endif