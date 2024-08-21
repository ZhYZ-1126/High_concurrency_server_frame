#include"../HCServer/bytearray.h"
#include"../HCServer/HCServer.h"


static HCServer::Logger::ptr g_logger=HCSERVER_LOG_ROOT();

void test_bytearray()
{

//setPosition(0)设置当前所在的内存块的位置
#define XX(type,len,writefun,readfun,base_len) { \
    vector<type> vec; \
    for(int i=0;i<len;++i){ \
        vec.push_back(rand()); \
    }\
    HCServer::ByteArray::ptr ba(new HCServer::ByteArray(base_len)); \
    for(auto &i:vec){ \
        ba->writefun(i); \
    } \
    ba->setPosition(0); \
    for(size_t i=0;i<vec.size();++i){ \
        int32_t v=ba->readfun(); \
        HCSERVER_ASSERT1(v==vec[i]); \
    }\
    HCSERVER_ASSERT1(ba->getReadSize()==0);\
    HCSERVER_LOG_INFO(g_logger) << #writefun "/" #readfun \
                    " (" #type " ) len=" << len \
                    << " base_len=" << base_len \
                    << " size=" << ba->getSize(); \
}
    XX(int8_t,100,writeFint8,readFint8,1);
    XX(uint8_t, 100, writeFuint8, readFuint8, 1);
    XX(int16_t,  100, writeFint16,  readFint16, 1);
    XX(uint16_t, 100, writeFuint16, readFuint16, 1);
    XX(int32_t,  100, writeFint32,  readFint32, 1);
    XX(uint32_t, 100, writeFuint32, readFuint32, 1);
    XX(int64_t,  100, writeFint64,  readFint64, 1);
    XX(uint64_t, 100, writeFuint64, readFuint64, 1);

    XX(int32_t,  100, writeInt32,  readInt32, 1);
    XX(uint32_t, 100, writeUint32, readUint32, 1);
    XX(int64_t,  100, writeInt64,  readInt64, 1);
    XX(uint64_t, 100, writeUint64, readUint64, 1);

#undef XX


//写入数据到bytearray中
//设置操作位置，用来读取
//读取数据
//设置操作位置，用来写入到文件中
//用新的对象从文件中读取
//对比写进文件的内容和读取文件的内容是否一样
#define XX(type,len,writefun,readfun,base_len) \
{ \
    vector<type> vec; \
    for(int i=0;i<len;++i) \
    { \
        vec.push_back(rand()); \
    } \
    HCServer::ByteArray::ptr ba(new HCServer::ByteArray(base_len)); \
    for(auto& i : vec) \
    { \
        ba->writefun(i); \
    } \
    ba->setPosition(0); \
    for(size_t i=0;i<vec.size();++i) \
    { \
        type v=ba->readfun(); \
        HCSERVER_ASSERT1(v==vec[i]); \
    } \
    HCSERVER_ASSERT1(ba->getReadSize()==0); \
    HCSERVER_LOG_INFO(g_logger) << #writefun "/" #readfun \
                    " (" #type " ) len=" << len \
                    << " base_len=" << base_len \
                    << " size=" << ba->getSize(); \
    ba->setPosition(0); \
    HCSERVER_ASSERT1(ba->writeToFile("/tmp/" #type "_" #len "-" #readfun ".dat")); \
    HCServer::ByteArray::ptr ba2(new HCServer::ByteArray(base_len * 2)); \
    HCSERVER_ASSERT1(ba2->readFromFile("/tmp/" #type "_" #len "-" #readfun ".dat")); \
    ba2->setPosition(0); \
    HCSERVER_ASSERT1(ba->toString() == ba2->toString()); \
    HCSERVER_ASSERT1(ba->getPosition() == 0); \
    HCSERVER_ASSERT1(ba2->getPosition() == 0); \
}
    XX(int8_t,  100, writeFint8, readFint8, 1);
    XX(uint8_t, 100, writeFuint8, readFuint8, 1);
    XX(int16_t,  100, writeFint16,  readFint16, 1);
    XX(uint16_t, 100, writeFuint16, readFuint16, 1);
    XX(int32_t,  100, writeFint32,  readFint32, 1);
    XX(uint32_t, 100, writeFuint32, readFuint32, 1);
    XX(int64_t,  100, writeFint64,  readFint64, 1);
    XX(uint64_t, 100, writeFuint64, readFuint64, 1);

    XX(int32_t,  100, writeInt32,  readInt32, 1);
    XX(uint32_t, 100, writeUint32, readUint32, 1);
    XX(int64_t,  100, writeInt64,  readInt64, 1);
    XX(uint64_t, 100, writeUint64, readUint64, 1);
#undef XX
}

int main()
{
    test_bytearray();
    return 0;
}