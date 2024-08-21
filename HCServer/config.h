#ifndef __HCSERVER_CONFIG_H__
#define __HCSERVER_CONFIG_H__

#include"log.h"
#include<memory>
#include<sstream>
#include<string>
#include<yaml-cpp/yaml.h>
#include<boost/lexical_cast.hpp>
#include<map>
#include<vector>
#include<list>
#include<set>
#include<unordered_map>
#include<unordered_set>
#include<functional>
#include"HCServer.h"
using namespace std;

namespace HCServer{

//基础配置信息类
class ConfigVarBase{
public:
    typedef shared_ptr<ConfigVarBase> ptr;
    ConfigVarBase(const string&name,const string &description= "")
    :m_name(name),m_description(description){
        transform(m_name.begin(),m_name.end(),m_name.begin(),::tolower);//将配置参数名称内的大写字母都转换成小写
    }
    virtual ~ConfigVarBase(){}
    const string &getname() const {return m_name; }
    const string &getdescription() const {return m_description;}

    virtual string  ToString()=0;//其他类型转换成字符串形式
    virtual bool fromString(const string& val)=0;//字符串形式转换为其他类型
    virtual string getTypeName() const=0;
private:
    string m_name;//配置信息名称
    string m_description;//配置信息描述
};

//实现各种类型的相互转换
//将F类型的v转换成T类型的v
template<class F,class T>
class LexicalCast{
public:
    T operator()(const F&v){
        return boost::lexical_cast<T>(v);
    }
};

//YAML String 转换成vector<T>
template<class T>
class LexicalCast<string,vector<T>>
{
public:
    vector<T> operator()(const string& v)
    {
        YAML::Node node=YAML::Load(v);  //将string类型转化成yaml数组
        typename std::vector<T> vec;    //typename用于指出模板声明（或定义）中的非独立名称是类型名
        stringstream ss;
        for(size_t i=0;i<node.size();++i)   //将yaml数组转换成vector类型后插入到vec容器内    
        {
            ss.str("");
            ss<<node[i];
            vec.push_back(LexicalCast<string,T>()(ss.str()));
        }
        return vec;
    }
};
//vector<T> 转换成 YAML String
template<class T>
class LexicalCast<std::vector<T>,string> 
{
public:
    string operator()(const vector<T>& v)
    {
        YAML::Node node;  
        for(auto &i : v)    //将vector类型转化成yaml数组
        {
            node.push_back(YAML::Load(LexicalCast<T,string>()(i)));
        }
        stringstream ss;
        ss<<node;
        return ss.str();
    }
};

//YAML String 转换成list<T>
template<class T>
class LexicalCast<string,list<T>>
{
public:
    list<T> operator()(const string& v)
    {
        YAML::Node node=YAML::Load(v);  //将string类型转化成yaml数组
        typename std::list<T> li;    //typename用于指出模板声明（或定义）中的非独立名称是类型名
        stringstream ss;
        for(size_t i=0;i<node.size();++i)   //将yaml数组转换成list类型    
        {
            ss.str("");
            ss<<node[i];
            li.push_back(LexicalCast<string,T>()(ss.str()));
        }
        return li;
    }
};
//list<T> 转换成 YAML String
template<class T>
class LexicalCast<std::list<T>,string>   
{
public:
    string operator()(const list<T>& v)
    {
        YAML::Node node;  
        for(auto &i : v)    //将list类型转化成yaml数组
        {
            node.push_back(YAML::Load(LexicalCast<T,string>()(i)));
        }
        stringstream ss;
        ss<<node;
        return ss.str();
    }
};

//YAML String 转换成set<T>
template<class T>
class LexicalCast<string,set<T>>
{
public:
    set<T> operator()(const string& v)
    {
        YAML::Node node=YAML::Load(v);  //将string类型转化成yaml数组
        typename std::set<T> se;    //typename用于指出模板声明（或定义）中的非独立名称是类型名
        stringstream ss;
        for(size_t i=0;i<node.size();++i)   //将yaml数组转换成set类型    
        {
            ss.str("");
            ss<<node[i];
            se.insert(LexicalCast<string,T>()(ss.str()));//set容器的插入是insert
        }
        return se;
    }
};
//set<T> 转换成 YAML String
template<class T>
class LexicalCast<std::set<T>,string>   
{
public:
    string operator()(const set<T>& v)
    {
        YAML::Node node;  
        for(auto &i : v)    //将set类型转化成yaml数组
        {
            node.push_back(YAML::Load(LexicalCast<T,string>()(i)));
        }
        stringstream ss;
        ss<<node;
        return ss.str();
    }
};

//YAML String 转换成unordered_set<T>
template<class T>
class LexicalCast<string,unordered_set<T>>
{
public:
    unordered_set<T> operator()(const string& v)
    {
        YAML::Node node=YAML::Load(v);  //将string类型转化成yaml数组
        typename std::unordered_set<T> use;    //typename用于指出模板声明（或定义）中的非独立名称是类型名
        stringstream ss;
        for(size_t i=0;i<node.size();++i)   //将yaml数组转换成unordered_set类型    
        {
            ss.str("");
            ss<<node[i];
            use.insert(LexicalCast<string,T>()(ss.str()));
        }
        return use;
    }
};
//unordered_set<T> 转换成 YAML String
template<class T>
class LexicalCast<std::unordered_set<T>,string>   
{
public:
    string operator()(const unordered_set<T>& v)
    {
        YAML::Node node;  
        for(auto &i : v)    //将unordered_set类型转化成yaml数组
        {
            node.push_back(YAML::Load(LexicalCast<T,string>()(i)));
        }
        stringstream ss;
        ss<<node;
        return ss.str();
    }
};

//YAML String 转换成map<string,T>
template<class T>
class LexicalCast<string,map<string,T>>
{
public:
    map<string,T> operator()(const string& v)
    {
        YAML::Node node=YAML::Load(v);  //将string类型转化成yaml数组
        typename std::map<string,T> ma;    //typename用于指出模板声明（或定义）中的非独立名称是类型名
        stringstream ss;
        for(auto it=node.begin();it!=node.end();it++)   //将yaml数组转换成map类型    
        {
            ss.str("");
            ss<<it->second;
            ma.insert(make_pair(it->first.Scalar(),LexicalCast<string,T>()(ss.str())));
        }
        return ma;
    }
};
//map<string,T> 转换成 YAML String
template<class T>
class LexicalCast<std::map<string,T>,string>   
{
public:
    string operator()(const map<string,T>& v)
    {
        YAML::Node node;  
        for(auto &i : v)    //将map类型转化成yaml数组
        {
            node[i.first]=YAML::Load(LexicalCast<T,string>()(i.second));
        }
        stringstream ss;
        ss<<node;
        return ss.str();
    }
};

//YAML String 转换成unordered_map<string,T>
template<class T>
class LexicalCast<string,unordered_map<string,T>>
{
public:
    unordered_map<string,T> operator()(const string& v)
    {
        YAML::Node node=YAML::Load(v);  //将string类型转化成yaml数组
        typename std::unordered_map<string,T> uma;    //typename用于指出模板声明（或定义）中的非独立名称是类型名
        stringstream ss;
        for(auto it=node.begin();it!=node.end();it++)   //将yaml数组转换成unordered_mapr类型    
        {
            ss.str("");
            ss<<it->second;
            uma.insert(make_pair(it->first.Scalar(),LexicalCast<string,T>()(ss.str())));
        }
        return uma;
    }
};
//unordered_map<string,T> 转换成 YAML String
template<class T>
class LexicalCast<std::unordered_map<string,T>,string>
{
public:
    string operator()(const unordered_map<string,T>& v)
    {
        YAML::Node node;  
        for(auto &i : v)    //将unordered_map类型转化成yaml数组
        {
            node[i.first]=YAML::Load(LexicalCast<T,string>()(i.second));
        }
        stringstream ss;
        ss<<node;
        return ss.str();
    }
};




//返回配置参数名为name的配置参数
//FromStr：将string转换成T；ToStr：将T转换成string
template<class T,class FromStr=LexicalCast<string,T>,class ToStr=LexicalCast<T,string>>
class ConfigVar : public ConfigVarBase {
public:
    typedef RWMutex RWMutexType;

    typedef shared_ptr<ConfigVar> ptr;
    //当一个配置发生更改时会通知旧配置信息是什么和新配置信息是什么（回调函数）
    typedef std::function<void (const T& old_value,const T& new_value)> on_change_cb;

    //name：配置参数名称
    //default_value：参数默认值
    //description：参数描述
    ConfigVar(const string& name,const T& default_value,const string& description = "")
        :ConfigVarBase(name, description)//调用父类的构造函数进行赋值
        ,m_val(default_value) {
    }

    //其他类型转换成字符串形式
    std::string ToString() override {
        try {
            //return boost::lexical_cast<string>(m_val);
            RWMutexType::Readlock lock(m_mutex);
            return ToStr()(m_val);
        } catch (exception& e) {
            HCSERVER_LOG_ERROR(HCSERVER_LOG_ROOT())<< "ConfigVar::toString exception "
                << e.what() << " convert: " << typeid(m_val).name() << " to string";
        }
        return "";  
    }
    //字符串形式转换为其他类型
    bool fromString(const string& val) override {
        try {
            //m_val = boost::lexical_cast<T>(val);
            setValue(FromStr()(val));
        } catch (exception& e) {
            HCSERVER_LOG_ERROR(HCSERVER_LOG_ROOT())<< "ConfigVar::toString exception "
                << e.what() << " convert: string to " << typeid(m_val).name();
        }
        return false;
    }

    const T getValue(){ 
        RWMutexType::Readlock lock(m_mutex);
        return m_val;
    }

    void setValue(const T& v) {
        {
            RWMutexType::Readlock lock(m_mutex);
            if(v==m_val) return ;//将yaml配置文件转化后的得到配置信息和原来的配置信息一样，则无需修改
            //配置信息发生变化，对执行回调函数配置信息进行覆盖修改
            for(auto &i : m_cbs){
                i.second(m_val,v);
            }
        }
        RWMutexType::Writelock lock(m_mutex);
        m_val=v;
    }
    string getTypeName() const override { return typeid(T).name();}//返回类型名称
    
    //添加监听
    uint64_t addListener(on_change_cb cb){
        static uint64_t s_fun_id=0;//静态生成一个key值
        RWMutexType::Writelock lock(m_mutex);
        ++s_fun_id;
        m_cbs[s_fun_id]=cb;
        return s_fun_id;
    }
    //删除监听
    void delListener(uint64_t key)
    {
        RWMutexType::Writelock lock(m_mutex);
        m_cbs.erase(key);
    }
    //返回回调函数
    on_change_cb getListener(uint64_t key){
        RWMutexType::Readlock lock(m_mutex);
        auto it=m_cbs.find(key);
        return it==m_cbs.end() ? nullptr : it->second;
    }
    //清除监听
    void clearListener(){
        RWMutexType::Writelock lock(m_mutex);
        m_cbs.clear();
    }
private:
    T m_val;
    //变更回调函数map数组，作为监听，满足条件时触发回调函数，uint64_t 是指key，要求是唯一值，一般可以用hash值
    std::map<uint64_t ,on_change_cb> m_cbs;
    RWMutexType m_mutex;
};

//ConfigVar的管理类 
class Config{
public:
    typedef unordered_map<string,ConfigVarBase::ptr> configvarmap;
    typedef RWMutex RWMutexType;

    //获取参数名为name的配置参数,如果存在直接返回，如果不存在,创建参数配置并用default_value赋值
    //返回对应的配置参数,如果参数名存在但是类型不匹配则返回nullptr
    //如果参数名包含非法字符[^0-9a-z_.] 抛出异常 std::invalid_argument
         
    template <class T>
    //实现对配置参数的创建和查找功能
    static typename ConfigVar<T>::ptr Lookup(const string& name,const T& default_value,const string& description = ""){
        // auto tmp=Lookup<T>(name);
        // if(tmp){//如果找到该名字
        //     HCSERVER_LOG_INFO(HCSERVER_LOG_ROOT())<<"Lookup name="<<name<<"exists";
        //     return tmp;
        // }
        RWMutexType::Writelock lock(getmutex());
        auto it=getdatas().find(name);
        if(it!=getdatas().end()){//该配置参数存在，无需创建
            //利用dynamic_pointer_cast的转换，如果同名的key（即类似system.int这些）对应的值的类型不同，会转换失败，返回nullptr
            auto tmp=dynamic_pointer_cast<ConfigVar<T> >(it->second);
            if(tmp){
                HCSERVER_LOG_INFO(HCSERVER_LOG_ROOT())<<"Lookup name="<<name<<"exists";
                return tmp;
            }else{
                HCSERVER_LOG_INFO(HCSERVER_LOG_ROOT())<<"Lookup name="<<name<<" exists but type not "<<typeid(T).name()<<" real tyep="
                <<it->second->getTypeName()<<" "<<it->second->ToString();
            }
        }
        if(name.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ._012345689")!=string::npos){
            HCSERVER_LOG_ERROR(HCSERVER_LOG_ROOT())<<"Lookup name invalid "<<name;
            throw invalid_argument(name);
        }
        typename ConfigVar<T>::ptr v(new ConfigVar<T>(name,default_value,description));//查找不到该配置信息的话则直接创建一个新的配置信息
        getdatas()[name]=v;
        return v;
    }

    template <class T>
    //重载Lookup用以实现第一个Lookup的查找功能
    static typename ConfigVar<T>::ptr Lookup(const string& name){
        RWMutexType::Readlock lock(getmutex());
        auto it=getdatas().find(name);//查找对应的名字
        if(it==getdatas().end())
        {
            return nullptr;
        }
        return dynamic_pointer_cast<ConfigVar<T> >(it->second);//找到则返回ConfigVarBase对象
    }
    
    static void LoadFileYaml(const YAML::Node& root);//在配置信息容器内查找相应名字的位置并且返回
    static ConfigVarBase::ptr LookupBase(const string& name);//通过识别yaml文件内的内容并对配置信息进行覆盖

    //遍历配置模块里面所有配置项，cb为回调函数
    static void Visit(function<void(ConfigVarBase::ptr)> cb);
private:
    static configvarmap& getdatas()
    {
        static configvarmap m_datas;
        return m_datas;
    }
    //获取互斥量，目地：(锁要比其他全局变量先创建,而静态变量优先级最先被初始化)
    static RWMutexType& getmutex()
    {
        static RWMutexType m_mutex;
        return m_mutex;
    }

};


}
#endif // 
