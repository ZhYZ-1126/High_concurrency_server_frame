#ifndef _HCSERVER_BYTEARRAY_H__
#define _HCSERVER_BYTEARRAY_H__

#include<memory>
#include<string>
#include<vector>
#include<stdint.h>
#include<sys/types.h>
#include<sys/socket.h>
using namespace std;

namespace HCServer{

//二进制数组,提供基础类型的序列化,反序列化功能
class ByteArray{
public:
    typedef shared_ptr<ByteArray> ptr;

    //链表结构体,
    //构造出指定大小的内存块Node，然后将每一块拼接起来，形成链表结构
    struct Node{
        Node(size_t s);
        Node();
        ~Node();

        char* ptr;//当前内存块地址指针
        size_t size;//内存块大小
        Node* next;//下一个内存块地址
    };

    ByteArray(size_t base_size = 4096);
    ~ByteArray();


    //write
    //int
    void writeFint8(int8_t value);//写入固定长度int8_t类型的数据
    void writeFuint8(uint8_t value);// 写入固定长度uint8_t类型的数据
    void writeFint16(int16_t value);//写入固定长度int16_t类型的数据(大端/小端)
    void writeFuint16(uint16_t value);//写入固定长度uint16_t类型的数据(大端/小端)
    void writeFint32(int32_t value);//写入固定长度int32_t类型的数据(大端/小端)
    void writeFuint32(uint32_t value);//写入固定长度uint32_t类型的数据(大端/小端)
    void writeFint64(int64_t value);//写入固定长度int64_t类型的数据(大端/小端)
    void writeFuint64(uint64_t value);//写入固定长度uint64_t类型的数据(大端/小端)
    //varint
    void writeInt32(int32_t value);//写入有符号的长度不固定的Varint32类型的数据
    void writeUint32(uint32_t value);//写入无符号的长度不固定的Varint32类型的数据
    void writeInt64(int64_t value);//写入有符号的长度不固定的Varint64类型的数据
    void writeUint64(uint64_t value);//写入无符号的长度不固定的Varint64类型的数据
    //float,double
    void writeFloat(float value);//写入float类型的数据
    void writeDouble(double value);//写入double类型的数据
    //string
    void writeStringF16(const string& value);//写入string类型的数据,用uint16_t作为长度类型
    void writeStringF32(const string& value);//写入string类型的数据,用uint32_t作为长度类型
    void writeStringF64(const string& value);//写入string类型的数据,用uint64_t作为长度类型
    void writeStringVint(const string& value);//写入string类型的数据,用无符号Varint64作为长度类型(可变长度)
    void writeStringWithoutLength(const string& value);//写入string类型的数据,无长度


    //read
    //int
    int8_t readFint8();//读取int8_t类型的数据
    uint8_t readFuint8();//读取uint8_t类型的数据
    int16_t  readFint16();//读取int16_t类型的数据
    uint16_t readFuint16();//读取uint16_t类型的数据
    int32_t  readFint32();//读取int32_t类型的数据
    uint32_t readFuint32();//读取uint32_t类型的数据 
    int64_t  readFint64();//读取int64_t类型的数据
    uint64_t readFuint64();//读取uint64_t类型的数据
    //varint
    int32_t  readInt32();//读取有符号的长度不固定的Varint32类型的数据
    uint32_t readUint32();//读取无符号的长度不固定的Varint32类型的数据
    int64_t  readInt64();//读取有符号的长度不固定的Varint64类型的数据
    uint64_t readUint64();//读取无符号的长度不固定的Varint64类型的数据
    //float,double
    float readFloat();//读取float类型的数据
    double readDouble();//读取double类型的数据
    //string
    string readStringF16();//读取string类型的数据,用uint16_t作为长度
    string readStringF32();//读取string类型的数据,用uint32_t作为长度
    string readStringF64();//读取string类型的数据,用uint64_t作为长度
    string readStringVint();//读取string类型的数据,用无符号Varint64作为长度



    //内部操作
    void clear();//清空ByteArray(数据)
    void write(const void* buf, size_t size);//写入size长度的数据到内存池
    void read(void* buf, size_t size);//从内存池读取size长度的数据
    void read(void* buf, size_t size, size_t position) const;//从内存池读取size长度的数据(不影响当前操作的位置)
    size_t getPosition() const { return m_position;}    //返回ByteArray当前操作位置
    void setPosition(size_t v); //设置ByteArray当前位置
    bool writeToFile(const string& name) const;//把ByteArray的当前所有数据写入到文件中(不影响当前操作的位置)
    bool readFromFile(const string& name);//从文件中读取数据
    size_t getBaseSize() const { return m_baseSize;}    //返回内存块的大小(一个节点的大小——字节)
    size_t getReadSize() const { return m_size - m_position;}   //返回可读取数据大小
    bool isLittleEndian() const;    //是否是小端
    void setIsLittleEndian(bool val);   //是否设置为小端
    size_t getSize() const { return m_size;}    //返回当前数据的大小(现有的数据大小——字节)
    string toString() const;   //将ByteArray里面的数据[m_position, m_size)转成std::string
    string toHexString() const;    //将ByteArray里面的数据[m_position, m_size)转成16进制的std::string(格式:FF FF FF)

    //获取可读取的缓存,保存成iovec数组(不影响当前操作的位置)
    uint64_t getReadBuffers(vector<iovec>& buffers, uint64_t len = ~0ull) const;
    //获取可读取的缓存,保存成iovec数组,从position位置开始(不影响当前操作的位置，没有副作用)
    uint64_t getReadBuffers(vector<iovec>& buffers, uint64_t len, uint64_t position) const;
    //获取可写入的缓存,保存成iovec数组(不影响当前操作的位置)
    uint64_t getWriteBuffers(vector<iovec>& buffers, uint64_t len);

private:
    void addCapacity(size_t size);  //扩容ByteArray,使其可以容纳size个数据(如果原本可以容纳,则不扩容)

    size_t getCapacity() const { return m_capacity - m_position;}   //获取当前的可写入容量

private:
    size_t m_baseSize;  //内存块的大小(一个节点的大小——字节)
    size_t m_position;  //当前操作位置(第几个字节个数)
    size_t m_capacity;  //当前的总容量(内存池的总大小——字节)
    size_t m_size;  //当前数据的大小(现有的数据大小——字节)
    int8_t m_endian;    //字节序,默认大端
    Node* m_root;   //第一个内存块指针(第一个节点指针)
    Node* m_cur;    //当前操作的内存块指针(当前操控节点指针)

};



}



#endif