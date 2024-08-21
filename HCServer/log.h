#ifndef __HCSERVER_LOG_H
#define __HCSERVER_LOG_H

#include<string>
#include<stdint.h>
#include<memory>
#include<list>
#include<sstream>
#include<fstream>
#include<vector>
#include <stdarg.h>
#include<map>
#include<mutex>
#include"singleton.h"
#include "util.h" 
#include"thread.h"

using namespace std;

//使用logger写入日志级别为level的日志（流式日志）
 #define HCSERVER_LOG_LEVEL(logger, level) \
     if(level >= logger->getLevel()) \
         HCServer::LogEventWrap(HCServer::LogEvent::ptr(new HCServer::LogEvent(logger, level, \
                         __FILE__, __LINE__, 0, HCServer::GetthreadId(),\
                 HCServer::GetFiberId(), time(0),HCServer::Thread::Getname()))).getSS()
 //使用logger写入日志界别为debug的日志（流式日志）
 #define HCSERVER_LOG_DEBUG(logger) HCSERVER_LOG_LEVEL(logger, HCServer::LogLevel::DEBUG)
 //使用logger写入日志界别为info的日志（流式日志）
 #define HCSERVER_LOG_INFO(logger) HCSERVER_LOG_LEVEL(logger, HCServer::LogLevel::INFO)
 //使用logger写入日志界别为warn的日志（流式日志）
 #define HCSERVER_LOG_WARN(logger) HCSERVER_LOG_LEVEL(logger, HCServer::LogLevel::WARN)
 //使用logger写入日志界别为error的日志（流式日志）
 #define HCSERVER_LOG_ERROR(logger) HCSERVER_LOG_LEVEL(logger, HCServer::LogLevel::ERROR)
 //使用logger写入日志界别为fatal的日志（流式日志）
 #define HCSERVER_LOG_FATAL(logger) HCSERVER_LOG_LEVEL(logger, HCServer::LogLevel::FATAL)

 //使用logger写入日志级别为level的日志（格式化,printf）
 #define HCSERVER_LOG_FMT_LEVEL(logger, level, fmt, ...) \
     if(logger->getLevel() <= level) \
         HCServer::LogEventWrap(HCServer::LogEvent::ptr(new HCServer::LogEvent(logger, level, \
                         __FILE__, __LINE__, 0, HCServer::GetthreadId(),\
                 HCServer::GetFiberId(), time(0),HCServer::Thread::Getname()))).GetEvent()->format(fmt, __VA_ARGS__)

 //使用logger写入日志级别为debug的日志（格式化,printf）
 #define HCSERVER_LOG_FMT_DEBUG(logger, fmt, ...) HCSERVER_LOG_FMT_LEVEL(logger, HCServer::LogLevel::DEBUG, fmt, __VA_ARGS__)
 //使用logger写入日志级别为info的日志（格式化,printf）
 #define HCSERVER_LOG_FMT_INFO(logger, fmt, ...)  HCSERVER_LOG_FMT_LEVEL(logger, HCServer::LogLevel::INFO, fmt, __VA_ARGS__)
 //使用logger写入日志级别为warn的日志（格式化,printf）
 #define HCSERVER_LOG_FMT_WARN(logger, fmt, ...)  HCSERVER_LOG_FMT_LEVEL(logger, HCServer::LogLevel::WARN, fmt, __VA_ARGS__)
 //使用logger写入日志级别为error的日志（格式化,printf）
 #define HCSERVER_LOG_FMT_ERROR(logger, fmt, ...) HCSERVER_LOG_FMT_LEVEL(logger, HCServer::LogLevel::ERROR, fmt, __VA_ARGS__)
 //使用logger写入日志级别为fatal的日志（格式化,printf）
 #define HCSERVER_LOG_FMT_FATAL(logger, fmt, ...) HCSERVER_LOG_FMT_LEVEL(logger, HCServer::LogLevel::FATAL, fmt, __VA_ARGS__)
 //获取主日志器
#define HCSERVER_LOG_ROOT() HCServer::m_LoggerMger::Getinstance()->getRoot()
 //获取指定名称的日志器，如果不存在则创建
#define HCSERVER_LOG_NAME(name) HCServer::m_LoggerMger::Getinstance()->getLogger(name)

namespace  HCServer{
class Logger;
class LoggerManager;
//日志信息的级别
class LogLevel
{
public:
    enum Level{
        UNKONW=0,//未知级别
        DEBUG = 1,	//调试级别
		INFO = 2,	//普通信息级别
		WARN = 3,	//警告信息
		ERROR = 4,	//错误信息
		FATAL = 5	//灾难级信息
    };
    static const char * ToString(LogLevel::Level level);
    static LogLevel::Level FromString(const string& str);
};

//日志事件
class LogEvent
{
public:
    typedef shared_ptr<LogEvent> ptr;
    LogEvent(shared_ptr<Logger> logger,LogLevel::Level level,const char *file,int32_t line,uint32_t elapse,uint32_t threadId,
            uint32_t fiberId,uint64_t time,const string&thread_name);

    LogLevel::Level getLevel() const { return m_level;}
    const char * getFile() const{return m_file;}
    int32_t getline() const{return m_line;}
    uint32_t getelapse() const{return m_elapse;}
    uint32_t getthreadId() const{return m_threadId;}
    uint32_t getfiberId() const{return m_fiberId;}
    const string& getthreadname()const {return m_thread_name;}
    uint64_t gettime() const{return time;}
    const string getcontent() const {return m_ss.str();}
    shared_ptr<Logger> getlogger() const {return m_logger;}

    stringstream &getSS() {return m_ss;}

    void format(const char*fmt,...);
    void Format(const char*fmt,va_list al);
private:
    LogLevel::Level m_level;//当前事件的级别
    const char *m_file=NULL;//文件名
    int32_t m_line=0;//行号
    uint32_t m_elapse=0;//程序启动累计耗时
    uint32_t m_threadId=0;//线程id
    uint32_t m_fiberId=0;//协程id
    uint64_t time=0;//时间戳
    string m_thread_name;//线程名称
    stringstream m_ss;// 线程消息体流   
    shared_ptr<Logger> m_logger; // 目标日志器
};

 // 日志事件包装类型（利用析构函数，触发日志写入）
class LogEventWrap{
public:
    LogEventWrap(LogEvent::ptr e);
    ~LogEventWrap();
    stringstream &getSS();
    LogEvent::ptr GetEvent() {return m_event;}
private:
    LogEvent::ptr m_event;
};


//日志的输出格式
class LogFormatter
{
public:
    typedef shared_ptr<LogFormatter> ptr;
    LogFormatter(const string& pattern);

    //将日志信息格式化成自己想要的格式的字符串
    string format(shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event);
public:
    // 具体日志格式项
    class FormatItem
    {
    public:
        typedef shared_ptr<FormatItem> ptr;
        virtual ~FormatItem(){};

        // 将对应的日志内容写入到os
        virtual void Format(ostream &os,shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::ptr event)=0;
    };
    void init();
    bool isError() const {return m_error;}
    const string getPattern()const {return m_pattern;}
private:
    //日志格式
    string m_pattern;
    //通过日志格式解析出来的FormatItem
    vector<FormatItem::ptr> m_items;
    //错误参数
    bool m_error=false;
};

//日志输出地
class LogAppender
{
friend class Logger;

public:
    typedef shared_ptr<LogAppender> ptr;
    typedef Spinlock MutexType;

    virtual ~LogAppender(){};
    virtual void log(shared_ptr<Logger> logger, LogLevel::Level level,LogEvent::ptr event)=0;

    virtual string ToYamlString()=0;//将格式转换为yaml string
    
    void SetLogFormatter(LogFormatter::ptr val);//设置当前要输出的日志的格式
    LogFormatter::ptr getformatter();//返回当前输出的日志的格式

    LogLevel::Level getLevel() {return m_level;}//返回当前日志的级别
    void SetLevel(LogLevel::Level level) {m_level=level;}//修改当前日志输出地的级别
protected:
    LogLevel::Level m_level=LogLevel::Level::DEBUG;//日志输出地的级别
    bool m_hasFormatte=false; //用于判断appender是否有自己的formatter格式
    LogFormatter::ptr m_formatter;
    MutexType m_mutex;
};

//日志器
class Logger:public enable_shared_from_this<Logger>
{
friend class LoggerManager;
public:
    typedef shared_ptr<Logger> ptr; 
    typedef Spinlock MutexType;

    Logger(const string &name="root");
    //输出日志，将日志在定义的输出地中以某种格式输出出来
    void log(LogLevel::Level level,LogEvent::ptr event);

    void debug(LogEvent::ptr event);
    void info(LogEvent::ptr event);
    void warn(LogEvent::ptr event);
    void error(LogEvent::ptr event);
    void fatal(LogEvent::ptr event);

    void AddAppender(LogAppender::ptr appender);//添加日志输出器
    void DelAppender(LogAppender::ptr appender);//删除日志输出器
    void clearAppender();
    LogLevel::Level getLevel() const{ return m_level;}; //返回当前的日志级别
    void SetLevel(LogLevel::Level level ){m_level=level;}; //修改/设置日志级别

    const string & getname() const {return m_name;};//返回日志名称

    void setFormatter(LogFormatter::ptr val);
    void setFormatter(const string& val);
    LogFormatter::ptr getformatter();

    string ToYamlString();//将格式转换为yaml string格式
private:
    string m_name; //日志名称
    LogLevel::Level m_level;//当前日志器有能力输出的最大级别
    list<LogAppender::ptr> m_appenders;//Appender的集合
    LogFormatter::ptr m_formatter;
    Logger::ptr m_root;
    MutexType m_mutex;
};



//输出到控制台的Appender
class StdOutLogAppender:public LogAppender
{
friend class Logger;
public:
    typedef shared_ptr<StdOutLogAppender> ptr;
    void log(Logger::ptr logger,LogLevel::Level level,LogEvent::ptr event) override;
    string ToYamlString() override;
private:
};
//输出到文件的Appender
class FileLogAppender:public LogAppender
{
friend class Logger;
public:
    typedef shared_ptr<FileLogAppender> ptr;
    FileLogAppender(const string & filename);//定义文件名
    void log(Logger::ptr logger,LogLevel::Level level,LogEvent::ptr event) override;
    string ToYamlString() override;

    //重新打开文件，成功返回true
    bool reopen();
private:
    string m_filename;
    ofstream m_filestream;
    uint64_t m_lasttime=0;
};

//管理所有的logger
class LoggerManager{
public: 
    typedef Spinlock MutexType;

    LoggerManager();
    // 获取名称为name的日志器
    // 如果name不存在，则创建一个，并使用root配置
    Logger::ptr getLogger(const string& name);
    
    void init();
    Logger::ptr getRoot() const { return m_root;}
    // 转成yaml格式的配置
    string toYamlString();
private:
    // 所有日志器
    std::map<string, Logger::ptr> m_loggers;
    // 主日志器（默认日志器）
    Logger::ptr m_root;
    MutexType m_mutex;//互斥量
};
typedef HCServer::Singleton<LoggerManager> m_LoggerMger;

}

#endif // !__HCSERVER_LOG_H
#define __HCSERVER_LOG_H