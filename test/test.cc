#include<iostream>


using namespace std;
#include "../HCServer/log.h"
#include "../HCServer/util.h"
int main()
{
    //声明一个event对象
    HCServer::Logger::ptr logger(new HCServer::Logger("zhangyanzhou"));


    //添加一个输出到控制台的日志输出器
    logger->AddAppender(HCServer::LogAppender::ptr(new HCServer::StdOutLogAppender));
    //声明一个event对象，并且初始化对象内的内容:文件名、行号、程序从启动到现在的毫秒数、线程id、携程id、时间
    HCServer::LogEvent::ptr event(new HCServer::LogEvent(logger,HCServer::LogLevel::DEBUG,__FILE__,__LINE__,0,HCServer::GetthreadId(),HCServer::GetFiberId(),time(0),"zhangyanzhou"));
    //进行日志信息的输出
    logger->log(event->getLevel(),event);

    // //使用宏，进行日志事件的创建
    HCSERVER_LOG_DEBUG(logger)<<"test 1";

    //添加一个将日志信息输出到文件名为log.txt的日志输出器
    HCServer::FileLogAppender::ptr file_appender(new HCServer::FileLogAppender("log.txt"));
    logger->AddAppender(file_appender);
    HCSERVER_LOG_FMT_DEBUG(logger,"test fmt error %s","aa");//使用

    HCServer::LogFormatter::ptr fmt(new HCServer::LogFormatter("%d%T%p%T%m%n"));//创建一种新的输出格式
    file_appender->SetLogFormatter(fmt);//修改输出格式
    file_appender->SetLevel(HCServer::LogLevel::ERROR);//修改日志器的等级

    HCSERVER_LOG_ERROR(logger)<<"test error std";
    HCSERVER_LOG_FMT_DEBUG(logger,"test debug fmt %s", "aa");

    //使用单例模式来创建日志器
    auto l=HCServer::m_LoggerMger::Getinstance()->getLogger("xxxx");
    HCSERVER_LOG_ERROR(l)<<"xxx";
    HCSERVER_LOG_FMT_ERROR(l,"test LoggerManger error %s","xxx");

    return 0;
}