#include"log.h"
#include"config.h"
#include<map>
#include<iostream>
#include<functional>
#include<string.h>
#include <stdarg.h>
#include<time.h>

using namespace std;

//将枚举enum的level转换为string形式的level
namespace HCServer{
const char * LogLevel::ToString(LogLevel::Level level)
{
   switch(level){		
#define XX(name) \
    case LogLevel::name: \
        return #name; \
        break;

    XX(DEBUG);
	XX(INFO);
	XX(WARN);
	XX(ERROR);
    XX(FATAL);
#undef XX
	default:
		return "UNKNOW"; 
	}
	return "UNKNOW";
}
//将string形式的level转换为enum形式的level
LogLevel::Level LogLevel::FromString(const string& str)
{
#define XX(level,v) \
    if(str==#v){\
        return LogLevel::level; \
    }
    XX(DEBUG,debug);
	XX(INFO,info);
	XX(WARN,warn);
	XX(ERROR,error);
    XX(FATAL,fatal);

    XX(DEBUG,DEBUG);
	XX(INFO,INFO);
	XX(WARN,WARN);
	XX(ERROR,ERROR);
    XX(FATAL,FATAL);
    return LogLevel::UNKONW;
#undef XX
}
LogEventWrap::LogEventWrap(LogEvent::ptr e)
:m_event(e){
}

LogEventWrap::~LogEventWrap(){
    m_event->getlogger()->log(m_event->getLevel(),m_event);
}

stringstream & LogEventWrap::getSS(){
    return m_event->getSS();
}

void LogEvent::format(const char*fmt,...){
    va_list al;
    va_start(al,fmt);
    Format(fmt,al);
    va_end(al);
}
void LogEvent::Format(const char*fmt,va_list al){
    char * buf=nullptr;
    int len=vasprintf(&buf,fmt,al);
    if(len!=-1)
    {
        m_ss<<string(buf,len);
        free(buf);
    }
}

class MessageFormatItem:public LogFormatter::FormatItem{
public:
    MessageFormatItem(const string &str=""){}
    void Format(ostream &os,shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::ptr event) override{
        os<<event->getcontent();
    }
};
class LevelFormatItem:public LogFormatter::FormatItem{
public:
    LevelFormatItem(const string &str=""){}
    void Format(ostream &os,Logger::ptr logger,LogLevel::Level level,LogEvent::ptr event) override{
        os<<LogLevel::ToString(level);
    }
};
class ElapseFormatItem:public LogFormatter::FormatItem{
public:
    ElapseFormatItem(const string &str=""){}
    void Format(ostream &os,Logger::ptr logger,LogLevel::Level level,LogEvent::ptr event) override{
        os<<event->getelapse();
    }
};
class LognameFormatItem:public LogFormatter::FormatItem{
public:
    LognameFormatItem(const string &str=""){}
    void Format(ostream &os,Logger::ptr logger,LogLevel::Level level,LogEvent::ptr event) override{
        os<<event->getlogger()->getname();
    }
};
class ThreadIdFormatItem:public LogFormatter::FormatItem{
public:
    ThreadIdFormatItem(const string &str=""){}
    void Format(ostream &os,Logger::ptr logger,LogLevel::Level level,LogEvent::ptr event) override{
        os<<event->getthreadId();
    }
};
class FiberIdFormatItem:public LogFormatter::FormatItem{
public:
    FiberIdFormatItem(const string &str=""){}
    void Format(ostream &os,Logger::ptr logger,LogLevel::Level level,LogEvent::ptr event) override{
        os<<event->getfiberId();
    }
};
class ThreadNameFormatItem:public LogFormatter::FormatItem{
public:
    ThreadNameFormatItem(const string &str=""){}
    void Format(ostream &os,Logger::ptr logger,LogLevel::Level level,LogEvent::ptr event) override{
        os<<event->getthreadname();
    }
};
class TimeFormatItem:public LogFormatter::FormatItem{
public:
    TimeFormatItem(const string &format="%Y-%m-%d %H:%M:%S") :m_format(format){
        if(m_format.empty()) m_format="%Y-%m-%d %H:%M:%S";
    }
    void Format(ostream &os,Logger::ptr logger,LogLevel::Level level,LogEvent::ptr event) override{
        struct tm tm;
        time_t time=event->gettime();
        localtime_r(&time,&tm);
        char buf[64];
        strftime(buf,sizeof(buf),m_format.c_str(),&tm);
        os<<buf;
    }
private:
    string m_format;
};
class FileNameFormatItem:public LogFormatter::FormatItem{
public:
    FileNameFormatItem(const string &str=""){}
    void Format(ostream &os,Logger::ptr logger,LogLevel::Level level,LogEvent::ptr event) override{
        os<<event->getFile();
    }
};
class FilelineFormatItem:public LogFormatter::FormatItem{
public:
    FilelineFormatItem(const string &str=""){}
    void Format(ostream &os,Logger::ptr logger,LogLevel::Level level,LogEvent::ptr event) override{
        os<<event->getline();
    }
};
class NewFilelineFormatItem:public LogFormatter::FormatItem{
public:
    NewFilelineFormatItem(const string &str=""){}
    void Format(ostream &os,Logger::ptr logger,LogLevel::Level level,LogEvent::ptr event) override{
        os<<endl;
    }
};
class StringFormatItem:public LogFormatter::FormatItem{
public:
    StringFormatItem(const string &str)  :m_string(str){}
    void Format(ostream &os,Logger::ptr logger,LogLevel::Level level,LogEvent::ptr event) override{
        os<<m_string;
    }
private:
    string m_string;
};
class TabFormatItem:public LogFormatter::FormatItem{
public:
    TabFormatItem(const string &str="") {}
    void Format(ostream &os,Logger::ptr logger,LogLevel::Level level,LogEvent::ptr event) override{
        os<<"\t";
    } 
};

//定义日志事件信息
LogEvent::LogEvent(shared_ptr<Logger> logger,LogLevel::Level level,const char *file,int32_t line,uint32_t elapse,uint32_t threadId,uint32_t fiberId,uint64_t time,const string&thread_name)
:m_logger(logger),m_level(level),m_file(file),m_line(line),m_elapse(elapse),m_threadId(threadId),m_fiberId(fiberId),time(time),m_thread_name(thread_name)
{ }
//logger构造
Logger::Logger(const string &name)
    :m_name(name)
    ,m_level(LogLevel::DEBUG) //这里指定日志器一个自身默认级别是INFO
{
    //定义默认的日志输出格式
    m_formatter.reset(new LogFormatter("%d{%Y-%m-%d %H:%M:%S}%T%t%T%F%T%N%T[%p]%T[%c]%T%f:%l%T%m%n"));
} 
//设置logger的输出格式，参数类型为LogFormatter
void Logger::setFormatter(LogFormatter::ptr val)
{
    //写日志格式时加上锁
    MutexType::Lock lock(m_mutex);

    m_formatter=val;
    for(auto& i:m_appenders){
        MutexType::Lock ll(i->m_mutex);

        if(!i->m_hasFormatte){//如果当前输出地的没有输出格式，则会把输出地的输出格式设置为logger的输出格式
            i->m_formatter=m_formatter;
        }
    }
}
//设置logger的输出格式，参数类型为string
void Logger::setFormatter(const string& val)
{
    HCServer::LogFormatter::ptr new_val(new HCServer::LogFormatter(val));
    if(new_val->isError()){
        cout<<"Logger setFormatter name="<<m_name<<" value="<<val<<" is a invalid formatter"<<endl;
        return;
    }
    setFormatter(new_val);
}
//返回日志格式
LogFormatter::ptr Logger::getformatter()
{
    //读日志格式时加上锁
    MutexType::Lock lock(m_mutex);
    return m_formatter;
}
//添加日志信息
void Logger::AddAppender(LogAppender::ptr appender)
{   
    //写日志输出地时加上锁
    MutexType::Lock lock(m_mutex);
    if(!appender->getformatter()){//如果appender没有设置格式
        MutexType::Lock ll(appender->m_mutex);//上锁，防止在添加输出地的输出格式过程中，其他线程对当前输出地的输出格式进行添加或者修改
        appender->m_formatter=m_formatter;//把输出地的输出格式设置为logger的输出格式
    }
    m_appenders.push_back(appender);
}
//删除日志信息
void Logger::DelAppender(LogAppender::ptr appender)
{
    //删除日志输出地时加上锁
    MutexType::Lock lock(m_mutex);
    for(auto it=m_appenders.begin();it!=m_appenders.end();it++)
    {
        if(*it==appender) 
        {
            m_appenders.erase(it);
            break;
        }
    }
}
//清空日志输出信息
void Logger::clearAppender()
{
    MutexType::Lock lock(m_mutex);
    m_appenders.clear();
}
//将日志器的格式转换为yaml string格式并且输出
string Logger::ToYamlString()
{
    MutexType::Lock lock(m_mutex);
    YAML::Node node;
    node["name"]=m_name;
    if(m_level!=LogLevel::UNKONW){
        node["level"]=LogLevel::ToString(m_level);
    }
    if(m_formatter){
        node["formatter"]=m_formatter->getPattern();
    }
    for(auto &i:m_appenders){
        node["appenders"].push_back(YAML::Load(i->ToYamlString()));//Load函数将 参数string（日志输出格式） 转换成yaml::node类型
    }
    stringstream ss;
    ss<<node;
    return ss.str();
}
//输出日志，将日志在定义的输出地中以某种格式输出出来
void Logger::log(LogLevel::Level level,LogEvent::ptr event)
{
    //如果想要查看日志信息的级别大于等于当前日志器的级别，那么才进行输出日志操作
    if(level>=m_level)
    {
        auto self=shared_from_this();
        //上锁，防止appender被修改
        MutexType::Lock lock(m_mutex);
        if(!m_appenders.empty())//格式不为空
        {
            for(auto& i:m_appenders)
            {
                i->log(self,level,event);
            }
        }else if(m_root)//格式为空且为m_root
        {
            m_root->log(level,event);
        }
    }
}
void Logger::debug(LogEvent::ptr event)
{
    log(LogLevel::DEBUG,event);
}
void Logger::info(LogEvent::ptr event)
{
    log(LogLevel::INFO,event);
}
void Logger::warn(LogEvent::ptr event)
{
    log(LogLevel::WARN,event);
}
void Logger::error(LogEvent::ptr event)
{
    log(LogLevel::ERROR,event);
}
void Logger::fatal(LogEvent::ptr event)
{
    log(LogLevel::FATAL,event);
}
//设置当前要输出的日志的格式
void LogAppender::SetLogFormatter(LogFormatter::ptr val)
{
    //写日志格式时加上锁
    MutexType::Lock lock(m_mutex);
    m_formatter=val;
    if(m_formatter){
        m_hasFormatte=true;
    }else{
        m_hasFormatte=false;
    }
}
//返回当前输出的日志的格式
LogFormatter::ptr LogAppender::getformatter()
{
    //读日志格式时加上锁
    MutexType::Lock lock(m_mutex);
    return m_formatter;
}
//输出到控制台的Appender
void StdOutLogAppender::log(shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::ptr event)
{
    if(level>=m_level) 
    {
        MutexType::Lock lock(m_mutex);
        //输出按照规定格式格式化后的字符串形式的日志信息
        cout<<m_formatter->format(logger,level,event);
    }
}
//logger的输出地、输出格式转换为yaml string
string StdOutLogAppender::ToYamlString()
{
    MutexType::Lock lock(m_mutex);
    YAML::Node node;
    node["type"]="StdOutLogAppender";
    if(m_level!=LogLevel::UNKONW){
        node["level"]=LogLevel::ToString(m_level);
    }
    if(m_hasFormatte &&m_formatter){
        node["formatter"]=m_formatter->getPattern();
    }
    stringstream ss;
    ss<<node;
    return ss.str();
}
//定义文件名
FileLogAppender::FileLogAppender(const string & filename)
{
    m_filename=filename;
    reopen();
}
//输出到文件的Appender
void FileLogAppender::log(shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::ptr event)
{   
    if(level>=m_level)
    {
        // uint64_t now=time(0);
        // if(now!=m_lasttime){//周期性地打开日志文件
        //     reopen();
        //     m_lasttime=now; 
        // }
        MutexType::Lock lock(m_mutex);
        if(!(m_filestream<<m_formatter->format(logger,level,event))){
            cout<<"error"<<endl;
        }
    }
}
//logger的输出地、输出格式转换为yaml string
string FileLogAppender::ToYamlString()
{
    MutexType::Lock lock(m_mutex);
    YAML::Node node;
    node["type"]="FileLogAppender";
    node["filename"]=m_filename;
    if(m_level!=LogLevel::UNKONW){
        node["level"]=LogLevel::ToString(m_level);
    }
    if(m_hasFormatte && m_formatter){
        node["formatter"]=m_formatter->getPattern();
    }
    stringstream ss;
    ss<<node;
    return ss.str();
}
//重新打开文件,成功返回true
bool FileLogAppender::reopen()
{
    MutexType::Lock lock(m_mutex);
    if(m_filestream)
    {   //如果文件流是开启的，则关闭该文件流
        m_filestream.close();
    }
    m_filestream.open(m_filename);//打开文件流中对应的文件
    return !!m_filestream;
}

LogFormatter::LogFormatter(const string& pattern)
:m_pattern(pattern){
    init();
}

string LogFormatter::format(shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::ptr event)
{
    std::stringstream ss;
    for(auto&i:m_items)
    {
        i->Format(ss,logger,level,event);
    }
    return ss.str();
}

void LogFormatter::init()
{
    //格式：%d{%Y-%m-%d %H:%M:%S}%T%t%T%F%T[%p]%T[%c]%T%f:%l%T%m%n
    //vector容器，每个元素都存放str，format，type
    //第一string存%x中的x   第二string存{xxx}中的xxx  第三int是区分是%x%x --0  %x{xxx} --1
    vector<tuple<string,string,int>> vec;
    string nstr;//解析后的字符串，存%x中的x
    //解析格式
    for(size_t i=0;i<m_pattern.size();i++)
    {
        //1.如果不是%，则把m_pattern添加到nstr字符串中
        if(m_pattern[i]!='%'){
            nstr.append(1,m_pattern[i]);
            continue;
        }
        //2.m_pattern[i]是% && m_pattern[i + 1] == '%' ==> 两个%,第二个%当作普通字符
        if((i + 1) < m_pattern.size()) {
            if(m_pattern[i + 1] == '%') {
                nstr.append(1, '%');
                continue;
            }
        }
        // 3.m_pattern[i]是% && m_pattern[i + 1] != '%', 需要进行解析
        size_t n=i+1;               // 跳过'%',从'%'的下一个字符开始解析

        int fmt_status=0;           // 是否在解析大括号内的内容: 已经遇到'{',但是还没有遇到'}' 值为1（表示正在解析{}中的内容）
        size_t fmt_begin = 0;	// 大括号开始的位置
        std::string str;    //%x{}  //存x
        std::string fmt;	// 存放'{}'中间截取的字符

        //while循环的作用：用于解析%x{}这种类型的格式
        while(n<m_pattern.size()){
            //判断第i+1(n)个字符是不是空格符,如果是则退出循环
            if(fmt_status==0&&(!isalpha(m_pattern[n])&&m_pattern[n]!='{'&&m_pattern[n]!='}')){
                str=m_pattern.substr(i+1,n-i-1);
                break;
            }
            //如果还是连续的内容
            if(fmt_status==0)
            {
                if(m_pattern[n]=='{'){
                    str=m_pattern.substr(i+1,n-i-1);
                    fmt_status=1;
                    fmt_begin=n;
                    ++n;
                    continue;
                }
            }else if(fmt_status==1)
            {
                if(m_pattern[n]=='}'){
                    fmt=m_pattern.substr(fmt_begin+1,n-fmt_begin-1);
                    //cout<<"#"<<fmt<<endl;
                    fmt_status=0;
                    ++n;//方便后面的解析能够直接从'}'后的元素开始解析
                    break;
                }
            }
            ++n;
            //判断是否遍历结束
            if(n==m_pattern.size()){
                if(str.empty()){//说明没有碰到'{'
                    str=m_pattern.substr(i+1);
                }
            }
        }

        if(fmt_status==0){
            if(!nstr.empty()) {
                //如果当前的格式字符是%x中的x部分，则把x存入vec的元素的第一个字符串中
                vec.push_back(make_tuple(nstr,string(),0));
                nstr.clear();
            }
            //将格式为%x{}的解析结果存到vec容器中
            vec.push_back(make_tuple(str,fmt,1));
            i=n-1;
        }else if(fmt_status==1)//说明只有'{'没有'}'，格式出错
        {
            cout<<"pattern parse error "<<m_pattern<<"-"<<m_pattern.substr(i)<<endl;
            m_error=true;
            vec.push_back(make_tuple("<<pattern error>>",fmt,0));
        }
    }

    if(!nstr.empty()){
        vec.push_back(make_tuple(nstr,"",0));
    }


//%d{%Y-%m-%d %H:%M:%S}%T%t%T%F%T%N%T[%p]%T[%c]%T%f:%l%T%m%n
    static map<string, function<FormatItem::ptr(const string& str)> > s_format_items = {
#define XX(str, C) \
        {#str, [](const string& fmt) { return FormatItem::ptr(new C(fmt));}}

        XX(m, MessageFormatItem),       //消息
        XX(p, LevelFormatItem),         //日志级别
        XX(r, ElapseFormatItem),        //累计毫秒数
        XX(c, LognameFormatItem),       //日志名称
        XX(t, ThreadIdFormatItem),      //日志id
        XX(n, NewFilelineFormatItem),   //换行
        XX(d, TimeFormatItem),          //时间
        XX(f, FileNameFormatItem),      //文件名
        XX(l, FilelineFormatItem),      //行号
        XX(T, TabFormatItem),           //Tab
        XX(F, FiberIdFormatItem),       //协程id
        XX(N, ThreadNameFormatItem),    //线程名称
#undef XX
    };

        for(auto& i : vec) {
            if(std::get<2>(i) == 0) {//%x%x的情况
                m_items.push_back(FormatItem::ptr(new StringFormatItem(std::get<0>(i))));
            } else {//%x{}的情况
                auto it = s_format_items.find(std::get<0>(i));
                if(it == s_format_items.end()) {
                    m_items.push_back(FormatItem::ptr(new StringFormatItem("<<error_format %" + std::get<0>(i) + ">>")));
                    m_error=true;
                } else {
                    m_items.push_back(it->second(std::get<1>(i)));
                }
            }
            //cout<< "(" <<get<0>(i)<<") - ("<<get<1>(i)<<") - ("<<get<2>(i)<<")"<<endl;
        }
    //cout<<m_items.size()<<endl;
}


LoggerManager::LoggerManager(){
    m_root.reset(new Logger);
    m_root->AddAppender(LogAppender::ptr(new StdOutLogAppender));

    m_loggers[m_root->m_name]=m_root;
    init();
}
string LoggerManager::toYamlString()
{
    MutexType::Lock lock(m_mutex);
    YAML::Node node;
    for(auto& i:m_loggers){
        node.push_back(YAML::Load(i.second->ToYamlString()));
    }
    stringstream ss;
    ss<<node;
    return ss.str();
}
Logger::ptr LoggerManager::getLogger(const std::string& name){
    //读日志器时加上锁，防止在读的过程中其他线程添加或者修改日志器
    MutexType::Lock lock(m_mutex);

    auto it=m_loggers.find(name);
    if(it!=m_loggers.end())//如果logger存在则返回
    {
        return it->second;
    }
    //如果日志器不存在则创建一个新的logger，并且使用root的appender和formatter来初始化该日志器
    Logger::ptr logger(new Logger(name));
    logger->m_root=m_root;
    m_loggers[name]=logger;
    return logger;
}


//日志输出地的定义
struct LogAppenderDefine{
    int type=0;//0: Stdout 1:file
    LogLevel::Level level=LogLevel::UNKONW;
    string formatter;
    string filename;

    bool operator==(const LogAppenderDefine& oth) const{
        return type==oth.type&&formatter==oth.formatter&&filename==oth.filename;
    }
};

//日志器定义
struct LogDefine{
    string name;
    LogLevel::Level level=LogLevel::UNKONW;
    string formatter;
    vector<LogAppenderDefine> appenders;

    bool operator==(const LogDefine& oth) const {
        return name==oth.name&&level==oth.level&&formatter==oth.formatter&&appenders==oth.appenders;
    }

    bool operator<(const LogDefine& oth) const {
        return name<oth.name;
    }
};

//YAML String 转换成set<LogDefine>
template<>
class LexicalCast<string,set<LogDefine>>
{
public:
    set<LogDefine> operator()(const string& v)
    {
        YAML::Node node=YAML::Load(v);  //将string类型转化成yaml数组
        set<LogDefine> se;    //typename用于指出模板声明（或定义）中的非独立名称是类型名
        for(size_t i=0;i<node.size();++i)   //将yaml数组转换成vector类型    
        {
            auto n=node[i];
            if(!n["name"].IsDefined()){//"name"不在yaml文件中
                cout<<"log config error:name is null,"<<n<<endl;
                continue;
            }
            LogDefine ld;
            ld.name=n["name"].as<string>();
            ld.level=LogLevel::FromString(n["level"].IsDefined() ?  n["level"].as<string>() : " ");
            if(n["formatter"].IsDefined()){
                ld.formatter=n["formatter"].as<string>();
            }
            if(n["appenders"].IsDefined()){
                for(size_t x=0;x<n["appenders"].size();++x){
                    auto a=n["appenders"][x];
                    if(!a["type"].IsDefined()){
                        cout<<"log config error:appender type is null,"<<a<<endl;
                        continue;
                    }
                    string type=a["type"].as<string>();
                    LogAppenderDefine lad;
                    if(type=="FileLogAppender"){
                        lad.type=0;
                        if(!a["filename"].IsDefined()){
                            cout<<"log config error:filenameappender type is null,"<<a<<endl;
                            continue;
                        }
                        lad.filename=a["filename"].as<string>();
                        if(a["formatter"].IsDefined()){
                            lad.formatter=a["formatter"].as<string>();
                        }
                    }else if(type=="StdOutLogAppender"){
                        lad.type=1;
                    }else{
                        cout<<"log config error:appender type is invalid,"<<a<<endl;
                        continue;
                    }
                    ld.appenders.push_back(lad);
                }
            }
            se.insert(ld);
        }
        return se;
    }
};
//set<LogDefine> 转换成 YAML String
template<>
class LexicalCast<set<LogDefine>,string> 
{
public:
    string operator()(const set<LogDefine>& v)
    {
        YAML::Node node;  
        for(auto &i:v){
            YAML::Node n;
            n["name"]=i.name;
            if(i.level!=LogLevel::UNKONW){
                n["level"]=LogLevel::ToString(i.level);
            }
            if(!i.formatter.empty()){
                n["formatter"]=i.formatter;
            }
            for(auto &a:i.appenders){
                YAML::Node na;
                if(a.type==0){
                    na["type"]="FileLogAppender";
                    na["filename"]=a.filename;
                }else if(a.type==1){
                    na["type"]="StdOutLogAppender";
                }
                if(a.level!=LogLevel::UNKONW){
                    na["level"]=LogLevel::ToString(a.level);
                }
                na["level"]=LogLevel::ToString(a.level);

                if(!a.formatter.empty()){
                    na["formatter"]=a.formatter;
                }
                n["appenders"].push_back(na);
            }
            node.push_back(n);
        }
        stringstream ss;
        ss<<node;
        return ss.str();
    }
};

HCServer::ConfigVar<set<LogDefine>>::ptr g_log_define=HCServer::Config::Lookup("logs",set<LogDefine>(),"log define");

//变更事件(添加一个监听，当条件满足时，触发回调函数，从而修改日志器的配置)
//实现日志模块和配置模块的相结合，通过上传一个外部的yaml配置文件，将日志文件中使用硬编码方式而产生的配置信息进行覆盖
struct LogIniter{
    LogIniter(){
        //添加一个监听，当条件满足时，触发回调函数，该回调函数会将yaml配置文件转化后的得到配置信息对原来的配置信息进行覆盖修改
        g_log_define->addListener([](const set<LogDefine>& old_value,const set<LogDefine>& new_value){
            HCSERVER_LOG_INFO(HCSERVER_LOG_ROOT())<<"on_logger_conf_changed";
            //遍历新增的值
            for(auto & i:new_value){
                auto it=old_value.find(i);
                HCServer::Logger::ptr logger;
                if(it==old_value.end()){//旧的值里面不存在和新增的值相同的值
                    //添加该新值（logger）
                    logger=HCSERVER_LOG_NAME(i.name);
                }else {//旧的值里面存在和新增的值相同的值，并且对应的logger的信息不同
                    if(!(i==*it)){
                        //修改logger值
                        logger=HCSERVER_LOG_NAME(i.name);//找到该logger
                    }
                }
                //设置logger的日志级别、输出格式和清空输出地
                logger->SetLevel(i.level);
                if(!i.formatter.empty()){
                    logger->setFormatter(i.formatter);//设置日志格式
                }
                logger->clearAppender();
                //设置输出地的各种信息，如输出到哪、输出低的级别、输出地的输出格式，最后再把输出地的信息添加到logger中
                for(auto & a:i.appenders)
                {
                    HCServer::LogAppender::ptr ap;
                    if(a.type==0){//输出到文件
                        ap.reset(new FileLogAppender(a.filename));
                    }else if(a.type==1){//输出到终端
                        ap.reset(new StdOutLogAppender);
                    }
                    ap->SetLevel(a.level);
                    if(!a.formatter.empty()){
                        LogFormatter::ptr fmt(new LogFormatter(a.formatter));
                        if(!fmt->isError()){
                            ap->SetLogFormatter(fmt);
                        }else{
                            cout<<"log name="<<i.name<<" appender type="<<a.type<<" formatter="<<a.formatter<<" is invalid"<<endl;
                        }
                    }
                    logger->AddAppender(ap);
                }
            }
            //删除旧值
            for(auto &i:old_value){
                auto it=new_value.find(i);
                if(it==new_value.end()){
                    //删除logger（并不是真正意义上的删除该日志，而是将日志级别设置得非常高和删除该日志的格式，让我们无法访问该日志）
                    auto logger=HCSERVER_LOG_NAME(i.name);
                    logger->SetLevel((LogLevel::Level)100);
                    logger->clearAppender();
                }
            }
        });
    }
};

static LogIniter _log_init;

void LoggerManager::init(){
    
}
}