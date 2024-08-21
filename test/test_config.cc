#include"../HCServer/log.h"
#include"../HCServer/config.h"
#include<yaml-cpp/yaml.h>

HCServer::ConfigVar<int>::ptr g_int_value_config=HCServer::Config::Lookup("system.int",(int)8080,"system port");

//key（system.int）相同但是值的类型不相同，这种会报错
//HCServer::ConfigVar<float>::ptr g_int_valuex_config=HCServer::Config::Lookup("system.int",(float)8080,"system port");

HCServer::ConfigVar<float>::ptr g_float_value_config=HCServer::Config::Lookup("system.float",(float)10.2f,"system value");

HCServer::ConfigVar<vector<int>>::ptr g_int_vec_value_config=HCServer::Config::Lookup("system.int_vec",vector<int>{1,2},"system int vec");

HCServer::ConfigVar<list<int>>::ptr g_int_list_value_config=HCServer::Config::Lookup("system.int_list",list<int>{10,20},"system int list");

HCServer::ConfigVar<set<int>>::ptr g_int_set_value_config=HCServer::Config::Lookup("system.int_set",set<int>{11,22},"system int set");

HCServer::ConfigVar<unordered_set<int>>::ptr g_int_unordered_set_value_config=HCServer::Config::Lookup("system.int_unordered_set",
                                                                                unordered_set<int>{11,22},"system int unordered_set");

HCServer::ConfigVar<map<string,int>>::ptr g_string_int_map_value_config=HCServer::Config::Lookup("system.string_int_map",
                                                                                map<string,int>{{"k",1}},"system string int map");

HCServer::ConfigVar<unordered_map<string,int>>::ptr g_string_int_unordered_map_value_config=HCServer::Config::Lookup("system.string_int_unordered_map",
                                                                                (unordered_map<string,int>){{"k",1}},"system string int unordered map");

//解析yaml文件
void printf_yaml(const YAML::Node&node,int level)
{
    if(node.IsScalar()){//存在常量
        HCSERVER_LOG_INFO(HCSERVER_LOG_ROOT())<<string(level*4,' ')<<node.Scalar()<<" - "<<node.Type()<<" - "<<level;
    }else if(node.IsNull()){//存在空节点
        HCSERVER_LOG_INFO(HCSERVER_LOG_ROOT())<<string(level*4,' ')<<"NULL - "<<node.Type()<<" - "<<level;
    }else if(node.IsMap()){//二叉树存在数据
        for(auto it=node.begin();it!=node.end();it++)
        {
            HCSERVER_LOG_INFO(HCSERVER_LOG_ROOT())<<string(level*4,' ')<<it->first<<" - "<<it->second.Type()<<" - "<<level;
            printf_yaml(it->second,level+1);
        }
    }else if(node.IsSequence()){//数组存在数据
        for(size_t i=0;i<node.size();i++)
        {
            HCSERVER_LOG_INFO(HCSERVER_LOG_ROOT())<<string(level*4,' ')<< i <<" - "<< node[i].Type()<<" - "<<level;
            printf_yaml(node[i],level+1);
        }
    }
}

void test_yaml()
{
    //创建要加载的yaml路径
    YAML::Node root=YAML::LoadFile("/home/zhangyanzhou/HCServer/bin/conf/test_config.yml");
    printf_yaml(root,0);//传入printf_yaml中，对该yml文件的内容进行解析
    HCSERVER_LOG_INFO(HCSERVER_LOG_ROOT())<<root.Scalar();
}

//读取yaml文件内的信息，并且进行key的值进行类型的转换后覆盖到配置信息中
void test_config()
{
    //查找并返回参数名称为system.port的配置参数（类型为int），如果存在则直接返回，如果不存在则创建一个新的参数配置并用8080赋值
    HCSERVER_LOG_INFO(HCSERVER_LOG_ROOT())<<"before:"<<g_int_value_config->getValue();
    //查找并返回参数名称为system.value的配置参数（类型为int），如果存在则直接返回，如果不存在则创建一个新的参数配置并用10.2f赋值
    HCSERVER_LOG_INFO(HCSERVER_LOG_ROOT())<<"before:"<<g_float_value_config->ToString();
    // //查找并返回参数名称为system.int_vec的配置参数（类型为vector<int>)，如果存在则直接返回，如果不存在则创建一个新的参数配置并用{1,2}赋值
    // auto v=g_int_vec_value_config->getValue();
    // for(auto&i:v){
    //     HCSERVER_LOG_INFO(HCSERVER_LOG_ROOT())<<"before int_vec: "<<i;
    // }
    //使用宏封装接口
//vector,list,set,unordered_set的宏
#define XX(g_var,name,prefix) \
    { \
        auto & v=g_var->getValue();\
        for(auto &i:v){\
            HCSERVER_LOG_INFO(HCSERVER_LOG_ROOT())<<#prefix " " #name ": "<<i;\
        }\
    }
//map，unordered_map的宏
#define XX_M(g_var,name,prefix) \
    { \
        auto & v=g_var->getValue();\
        for(auto &i:v){\
            HCSERVER_LOG_INFO(HCSERVER_LOG_ROOT())<<#prefix " " #name ": { "<<i.first<<" - "<<i.second<<"}";\
        }\
    }
    XX(g_int_vec_value_config,int_vec,before);
    XX(g_int_list_value_config,int_list,before);
    XX(g_int_set_value_config,int_set,before);
    XX(g_int_unordered_set_value_config,int_unordered_set,before);
    XX_M(g_string_int_map_value_config,string_int_map,before);
    XX_M(g_string_int_unordered_map_value_config,string_int_unordered_map,before);

    YAML::Node root=YAML::LoadFile("/home/zhangyanzhou/HCServer/bin/conf/test_config.yml");
    HCServer::Config::LoadFileYaml(root);


    //在log.yml文件内查找并返回参数名称为system.port的配置参数（类型为int），如果存在则直接返回，如果不存在则创建一个新的参数配置并用8080赋值
    HCSERVER_LOG_INFO(HCSERVER_LOG_ROOT())<<"after:"<<g_int_value_config->getValue();
    //在log.yml文件内查找并返回参数名称为system.value的配置参数（类型为int），如果存在则直接返回，如果不存在则创建一个新的参数配置并用10.2f赋值
    HCSERVER_LOG_INFO(HCSERVER_LOG_ROOT())<<"after:"<<g_float_value_config->ToString();
    // //在log.yml文件内查找并返回参数名称为system.int_vec的配置参数（类型为vector<int>)，如果存在则直接返回，如果不存在则创建一个新的参数配置并用{1,2}赋值
    // v=g_int_vec_value_config->getValue();
    // for(auto&i:v){
    //     HCSERVER_LOG_INFO(HCSERVER_LOG_ROOT())<<"after int_vec: "<<i;
    // }
    XX(g_int_vec_value_config,int_vec,after);
    XX(g_int_list_value_config,int_list,after);
    XX(g_int_set_value_config,int_set,after);
    XX(g_int_unordered_set_value_config,int_unordered_set,after);
    XX_M(g_string_int_map_value_config,string_int_map,after);
    XX_M(g_string_int_unordered_map_value_config,string_int_unordered_map,after);

}

class Person{
public:
    Person(){}
    string m_name=" ";
    int m_age=18;
    string m_sex="女";
    // Person(const string &name,int age,string sex):
    // m_name(name),m_age(age),m_sex(sex){
    // }
    string toString() const{
        stringstream ss;
        ss<<"[Person name="<<m_name<<" age="<<m_age<<" sex="<<m_sex<<"]";
        return ss.str();
    }

    bool operator==(const Person & oth) const{
        return m_name==oth.m_name&&m_age==oth.m_age&&m_sex==oth.m_sex;
    }
};
namespace HCServer{
//YAML String 转换成自定义类型class
template<>
class LexicalCast<string,Person>
{
public:
    Person operator()(const string& v)
    {
        YAML::Node node=YAML::Load(v);  //将string类型转化成yaml数组
        Person p;
        //将yaml数组转换为class
        p.m_name=node["name"].as<string>();
        p.m_age=node["age"].as<int>();
        p.m_sex=node["sex"].as<string>();
        return p;
    }
};
//自定义类型class转换成 YAML String
template<>
class LexicalCast<Person,string> 
{
public:
    string operator()(const Person& p)
    {
        YAML::Node node;  
        //将class转换为yaml数组
        node["name"]=p.m_name;
        node["age"]=p.m_age;
        node["sex"]=p.m_sex;
        stringstream ss;
        ss<<node;
        return ss.str();
    }
};
}
HCServer::ConfigVar<Person>::ptr g_Person_class_config=HCServer::Config::Lookup("class.person",Person(),"system person");
HCServer::ConfigVar<map<string,Person>>::ptr g_Person_map_config=HCServer::Config::Lookup("class.map",map<string,Person>(),"system map person");
HCServer::ConfigVar<map<string,vector<Person>>>::ptr g_vector_Person_map_config=HCServer::Config::Lookup("class.vec_map",map<string,vector<Person>>(),"system map person");

//自定义的类型转换
void test_class()
{
    HCSERVER_LOG_INFO(HCSERVER_LOG_ROOT())<<"class.person before:"<<g_Person_class_config->getValue().toString()<<endl<<g_Person_class_config->ToString();
    //宏
#define XX_PW(g_var,prefix)\
    {\
        auto &v=g_var->getValue();\
        for(auto& i:v){\
            HCSERVER_LOG_INFO(HCSERVER_LOG_ROOT())<<prefix<<":"<<i.first<<" - "<<i.second.toString();\
        }\
        HCSERVER_LOG_INFO(HCSERVER_LOG_ROOT())<<prefix<<" size="<<v.size(); \
    }
    
    //配置事件机制，当一个配置信息发生改变时，会通知旧配置信息是什么和新配置信息是什么（回调函数）
    g_Person_class_config->addListener([](const Person& old_value,const Person& new_value){
        HCSERVER_LOG_INFO(HCSERVER_LOG_ROOT())<<"old value="<<old_value.toString()<<" new value="<<new_value.toString();
    });

    XX_PW(g_Person_map_config,"class.map before");
    HCSERVER_LOG_INFO(HCSERVER_LOG_ROOT())<<"class.vec_map before:"<<g_vector_Person_map_config->ToString();

    YAML::Node root=YAML::LoadFile("/home/zhangyanzhou/HCServer/bin/conf/test_config.yml"); 
    HCServer::Config::LoadFileYaml(root);

    HCSERVER_LOG_INFO(HCSERVER_LOG_ROOT())<<"class.person after:"<<g_Person_class_config->getValue().toString()<<endl<<g_Person_class_config->ToString();
    XX_PW(g_Person_map_config,"class.map after");
    HCSERVER_LOG_INFO(HCSERVER_LOG_ROOT())<<"class.vec_map after:"<<endl<<g_vector_Person_map_config->ToString();
   
}

void test_log()
{
    //创建一个名称为system的logger对象时，由于开始没有对其初始化，所有会先使用root的logger的输出格式，输出地，日志级别等信息来对该logger进行初始化，
    //当后面传入yaml文件并且里面有system用户时，会使用system用户的输出格式，输出地，日志级别等信息对之前初始化后的信息进行覆盖
    static HCServer::Logger::ptr system_log=HCSERVER_LOG_NAME("system");
    HCSERVER_LOG_INFO(system_log)<<"hello system"<<endl;
    cout<<"===================================="<<endl;
    cout<<HCServer::m_LoggerMger::Getinstance()->toYamlString()<<endl;
    YAML::Node root=YAML::LoadFile("/home/zhangyanzhou/HCServer/bin/conf/log.yml"); 
    HCServer::Config::LoadFileYaml(root);
    cout<<"===================================="<<endl;
    cout<<HCServer::m_LoggerMger::Getinstance()->toYamlString()<<endl;
    cout<<"===================================="<<endl;
    HCSERVER_LOG_INFO(system_log)<<"hello system after yaml"<<endl;

    system_log->setFormatter("%d - %m%n");//修改logger的formatter会影响到appender的formatter
    HCSERVER_LOG_INFO(system_log)<<"hello system after setFormatter yaml"<<endl;
    
}

int main()
{
    // //返回参数名称为system.port的配置参数，如果存在则直接返回，如果不存在则创建一个新的参数配置并用8080赋值
    // HCSERVER_LOG_INFO(HCSERVER_LOG_ROOT())<<g_int_value_config->getValue();
    // HCSERVER_LOG_INFO(HCSERVER_LOG_ROOT())<<g_int_value_config->ToString();
    // //返回参数名称为system.value的配置参数，如果存在则直接返回，如果不存在则创建一个新的参数配置并用10.2f赋值
    // HCSERVER_LOG_INFO(HCSERVER_LOG_ROOT())<<g_float_value_config->getValue();
    // HCSERVER_LOG_INFO(HCSERVER_LOG_ROOT())<<g_float_value_config->ToString();
    // test_yaml();
    

    //test_config();
    //test_class();

    test_log();
    HCServer::Config::Visit([](HCServer::ConfigVarBase::ptr var){
    HCSERVER_LOG_INFO(HCSERVER_LOG_ROOT())<<"name="<<var->getname()<<" description="<<var->getdescription()
                                    <<" typename="<<var->getTypeName()<<" value="<<var->ToString();
    });
    return 0;
}