#include"../HCServer/uri.h"
#include"../HCServer/HCServer.h"
#include<iostream>
using namespace std;

int main(int argc,char ** argv)
{
    //HCServer::Uri::ptr uri=HCServer::Uri::Create("http://www.sylar.top/test/uri?id=100&name=sylar#frg");
    HCServer::Uri::ptr uri=HCServer::Uri::Create("http://admin@www.sylar.top/test/中文uri?id=100中文&name=sylar中文#frg中文");
    cout<<uri->toString()<<endl;
    auto addr=uri->createAddress();
    cout<<*addr;
    return 0;
}