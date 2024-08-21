#include "config.h"
#include<list>

namespace HCServer{

//Config::configvarmap Config::m_datas;

//在配置信息容器内查找相应名字的位置并且返回
ConfigVarBase::ptr Config::LookupBase(const string& name)
{
    RWMutexType::Readlock lock(getmutex());
    auto it=getdatas().find(name);
    return it==getdatas().end() ? nullptr :it->second;
}

//将yaml格式的内容进行转换，并且将合理的数据插入list容器中（模板）
static void ListAllMember(const string& prefix,const YAML::Node& node,list<pair<string, const YAML::Node> >& output) 
{
    //判断名称是否有效
    //find_first_not_of()搜索与其参数中指定的任何字符都不匹配的第一个字符，查找失败则返回npos，
    //查找成功则说明prefix里面存在与abcdefghikjlmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ._012345678都不匹配的字符，说明prefix命名错误
    if(prefix.find_first_not_of("abcdefghikjlmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ._012345678")!= string::npos)   
    {
        HCSERVER_LOG_ERROR(HCSERVER_LOG_ROOT()) << "Config invalid name: " << prefix << " : " << node;
        return;
    }
    //名称有效则添加到output容器中
    output.push_back(make_pair(prefix, node));
    if(node.IsMap())    //IsMap类似标准库中的Map（二叉树），对应YAML格式中的对象 
    {
        for(auto it = node.begin();it != node.end(); ++it) 
        {
            ListAllMember(prefix.empty() ? it->first.Scalar(): prefix + "." + it->first.Scalar(), it->second, output);
        }
    }
}

//通过识别yaml文件内的内容并对配置信息进行覆盖
void Config::LoadFileYaml(const YAML::Node&root)
{
    list<pair<string, const YAML::Node> > all_nodes;
    ListAllMember("",root,all_nodes);//识别yaml文件，将识别到的内容存与all_nodes容器中
    for(auto& i : all_nodes) 
    {
        string key = i.first;//key为配置信息名称（键值）
        if(key.empty()) 
        {
            continue;
        }
        transform(key.begin(), key.end(), key.begin(), ::tolower); //大写转小写
        ConfigVarBase::ptr var=LookupBase(key);//查找配置信息名称对应的配置信息的位置

        if(var) //如果找到了
        {
            if(i.second.IsScalar())//IsScalar是否为标量
            {
                var->fromString(i.second.Scalar()); //从YAML 标量 转成想要的类型
            }
            else//转到string里面去
            {
                std::stringstream ss; //输出流
                ss<<i.second;
                var->fromString(ss.str());  //从YAML String 转成想要的类型
            }
        }
    }
}

//遍历配置模块里面所有配置项，cb为回调函数
void Config::Visit(function<void(ConfigVarBase::ptr)> cb)
{
    RWMutex::Readlock lock(getmutex());
    configvarmap &m=getdatas();
    for(auto it=m.begin();it!=m.end();it++){
        cb(it->second);
    }
}

}