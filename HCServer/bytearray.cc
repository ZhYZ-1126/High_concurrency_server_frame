#include"bytearray.h"
#include"HCServer.h"
#include <fstream>
#include <sstream>
#include <string.h>
#include <iomanip>

#include "endian.h"
#include "log.h"
using namespace std;

namespace HCServer{

static HCServer::Logger::ptr g_logger=HCSERVER_LOG_NAME("system");

ByteArray::Node::Node(size_t s)
    :ptr(new char[s])
    ,next(nullptr)
    ,size(s) {
}

ByteArray::Node::Node()
    :ptr(nullptr)
    ,next(nullptr)
    ,size(0) {
}

ByteArray::Node::~Node() {
    if(ptr) {
        delete[] ptr;
    }
}

ByteArray::ByteArray(size_t base_size)
    :m_baseSize(base_size)
    ,m_position(0)
    ,m_size(0)
    ,m_capacity(base_size)
    ,m_endian(HCSERVER_BIG_ENDIAN)
    ,m_root(new Node(base_size))
    ,m_cur(m_root) {
}

ByteArray::~ByteArray() {
    Node* tmp = m_root;
    while(tmp) {//遍历链表，删除全部数据
        m_cur = tmp;
        tmp = tmp->next;
        delete m_cur;
    }
}
//是否是小端
bool ByteArray::isLittleEndian() const {
    return m_endian == HCSERVER_LITTLE_ENDIAN;
}
//是否设置为小端
void ByteArray::setIsLittleEndian(bool val) {
    if(val) {
        m_endian = HCSERVER_LITTLE_ENDIAN;
    } else {
        m_endian = HCSERVER_BIG_ENDIAN;
    }
}


//write
void ByteArray::writeFint8(int8_t value)//写入固定长度int8_t类型的数据
{
    write(&value, sizeof(value));
}
void ByteArray::writeFuint8(uint8_t value)// 写入固定长度uint8_t类型的数据
{
    write(&value, sizeof(value));
}
void ByteArray::writeFint16(int16_t value)//写入固定长度int16_t类型的数据(大端/小端)
{
    if(m_endian != HCSERVER_BYTE_ORDER) {
        value = byteswap(value);//转为大字节序
    }
    write(&value, sizeof(value));
}
void ByteArray::writeFuint16(uint16_t value)//写入固定长度uint16_t类型的数据(大端/小端)
{
    if(m_endian != HCSERVER_BYTE_ORDER) {
        value = byteswap(value);
    }
    write(&value, sizeof(value));
}
void ByteArray::writeFint32(int32_t value)//写入固定长度int32_t类型的数据(大端/小端)
{
    if(m_endian != HCSERVER_BYTE_ORDER) {
        value = byteswap(value);
    }
    write(&value, sizeof(value));
}
void ByteArray::writeFuint32(uint32_t value)//写入固定长度uint32_t类型的数据(大端/小端)
{
    if(m_endian != HCSERVER_BYTE_ORDER) {
        value = byteswap(value);
    }
    write(&value, sizeof(value));
}
void ByteArray::writeFint64(int64_t value)//写入固定长度int64_t类型的数据(大端/小端)
{
    if(m_endian != HCSERVER_BYTE_ORDER) {
        value = byteswap(value);
    }
    write(&value, sizeof(value)); 
}
void ByteArray::writeFuint64(uint64_t value)//写入固定长度uint64_t类型的数据(大端/小端)
{
    if(m_endian != HCSERVER_BYTE_ORDER) {
        value = byteswap(value);
    }
    write(&value, sizeof(value));
}

static uint32_t EncodeZigzag32(const int32_t & v)//int32_t有符号转成无符号
{
    if(v < 0) {
        return ((uint32_t)(-v)) * 2 - 1;
    } else {
        return v * 2;
    }
}
static uint64_t EncodeZigzag64(const int64_t& v) //int64_t有符号转成无符号
{
    if(v < 0) {
        return ((uint64_t)(-v)) * 2 - 1;
    } else {
        return v * 2;
    }
}

void ByteArray::writeInt32(int32_t value)//写入有符号Varint32类型的数据
{
    writeUint32(EncodeZigzag32(value));
}
void ByteArray::writeUint32(uint32_t value)//写入无符号Varint32类型的数据
{
    uint8_t tmp[5];
    uint8_t i = 0;
    while(value >= 0x80) {
        tmp[i++] = (value & 0x7F) | 0x80;
        value >>= 7;//右移7位
    }
    tmp[i++] = value;
    write(tmp, i);
}
void ByteArray::writeInt64(int64_t value)//写入有符号Varint64类型的数据
{
    writeUint64(EncodeZigzag64(value));
}   
void ByteArray::writeUint64(uint64_t value)//写入无符号Varint64类型的数据
{
    uint8_t tmp[10];
    uint8_t i = 0;
    while(value >= 0x80) {
        tmp[i++] = (value & 0x7F) | 0x80;
        value >>= 7;
    }
    tmp[i++] = value;
    write(tmp, i);
}
void ByteArray::writeFloat(float value)//写入float类型的数据
{
    uint32_t v;
    memcpy(&v, &value, sizeof(value));
    writeFuint32(v);
}
void ByteArray::writeDouble(double value)//写入double类型的数据
{
    uint64_t v;
    memcpy(&v, &value, sizeof(value));
    writeFuint64(v);
}
void ByteArray::writeStringF16(const string& value)//写入string类型的数据,用uint16_t作为长度类型
{
    writeFuint16(value.size());
    write(value.c_str(), value.size());
}
void ByteArray::writeStringF32(const string& value)//写入string类型的数据,用uint32_t作为长度类型
{
    writeFuint32(value.size());
    write(value.c_str(), value.size());
}
void ByteArray::writeStringF64(const string& value)//写入string类型的数据,用uint64_t作为长度类型
{
    writeFuint64(value.size());
    write(value.c_str(), value.size());
}
void ByteArray::writeStringVint(const string& value)//写入string类型的数据,用无符号Varint64作为长度类型(可变长度)
{
    writeUint64(value.size());
    write(value.c_str(), value.size());
}
void ByteArray::writeStringWithoutLength(const string& value)//写入string类型的数据,无长度
{
    write(value.c_str(), value.size());
}


//read
int8_t ByteArray::readFint8()//读取int8_t类型的数据
{
    int8_t v;
    read(&v, sizeof(v));
    return v;
}
uint8_t ByteArray::readFuint8()//读取uint8_t类型的数据
{
    uint8_t v;
    read(&v, sizeof(v));
    return v;
}

#define XX(type) \
    type v; \
    read(&v, sizeof(v)); \
    if(m_endian == HCSERVER_BYTE_ORDER) { \
        return v; \
    } else { \
        return byteswap(v); \
    }

int16_t  ByteArray::readFint16()//读取int16_t类型的数据
{
    XX(int16_t);
}
uint16_t ByteArray::readFuint16()//读取uint16_t类型的数据
{
    XX(uint16_t);
}
int32_t  ByteArray::readFint32()//读取int32_t类型的数据
{
    XX(int32_t);
}
uint32_t ByteArray::readFuint32()//读取uint32_t类型的数据 
{
    XX(uint32_t);
}
int64_t ByteArray::readFint64()//读取int64_t类型的数据
{
    XX(int64_t);
}
uint64_t ByteArray::readFuint64()//读取uint64_t类型的数据
{
    XX(uint64_t);
}

#undef XX


static int32_t DecodeZigzag32(const uint32_t& v) {
    return (v >> 1) ^ -(v & 1);
}

static int64_t DecodeZigzag64(const uint64_t& v) {
    return (v >> 1) ^ -(v & 1);
}
int32_t ByteArray::readInt32()//读取有符号Varint32类型的数据
{
    return DecodeZigzag32(readUint32());
}
uint32_t ByteArray::readUint32()//读取无符号Varint32类型的数据
{
    uint32_t result = 0;
    for(int i = 0; i < 32; i += 7) {
        uint8_t b = readFuint8();
        if(b < 0x80) {
            result |= ((uint32_t)b) << i;
            break;
        }else {
            result |= (((uint32_t)(b & 0x7f)) << i);
        }
    }
    return result; 
}
int64_t ByteArray::readInt64()//读取有符号Varint64类型的数据
{
    return DecodeZigzag64(readUint64());
}
uint64_t ByteArray::readUint64()//读取无符号Varint64类型的数据
{
    uint64_t result = 0;
    for(int i = 0; i < 64; i += 7) {
        uint8_t b = readFuint8();
        if(b < 0x80) {
            result |= ((uint64_t)b) << i;
            break;
        } else {
            result |= (((uint64_t)(b & 0x7f)) << i);
        }
    }
    return result;
}
float ByteArray::readFloat()//读取float类型的数据
{
    uint32_t v = readFuint32();
    float value;
    memcpy(&value, &v, sizeof(v));
    return value;
}
double ByteArray::readDouble()//读取double类型的数据
{
    uint64_t v = readFuint64();
    double value;
    memcpy(&value, &v, sizeof(v));
    return value;
}
string ByteArray::readStringF16()//读取string类型的数据,用uint16_t作为长度
{
    uint16_t len = readFuint16();
    string buff;
    buff.resize(len);
    read(&buff[0], len);
    return buff; 
}
string ByteArray::readStringF32()//读取string类型的数据,用uint32_t作为长度
{
    uint32_t len = readFuint32();
    string buff;
    buff.resize(len);
    read(&buff[0], len);
    return buff;
}
string ByteArray::readStringF64()//读取string类型的数据,用uint64_t作为长度
{
    uint64_t len = readFuint64();
    string buff;
    buff.resize(len);
    read(&buff[0], len);
    return buff;
}
string ByteArray::readStringVint()//读取string类型的数据,用无符号Varint64作为长度
{
    uint64_t len = readUint64();
    string buff;
    buff.resize(len);
    read(&buff[0], len);
    return buff;
}


//内部操作
void ByteArray::clear()//清空ByteArray(数据)
{
    m_position = m_size = 0;
    m_capacity = m_baseSize;
    Node* tmp = m_root->next;
    while(tmp) {
        m_cur = tmp;
        tmp = tmp->next;
        delete m_cur;
    }
    m_cur = m_root;
    m_root->next = NULL;
}
//写入size长度的数据到内存池
void ByteArray::write(const void* buf, size_t size)
{
    if(size == 0) {
        return;
    }
    addCapacity(size);//判断当前的容量是否足够写下size大小的buf，如果不够则扩充内存池容量

    size_t npos = m_position % m_baseSize;//当前节点处于该内存块的位置
    size_t ncap = m_cur->size - npos;//当前节点的容量
    size_t bpos = 0;//当前已经写入的buf的偏移量

    while(size > 0) {
        if(ncap >= size) {//当前节点的所剩容量足够写下size个字节大小的buf
            memcpy(m_cur->ptr + npos, (const char*)buf + bpos, size); //从存储区 buf + bpos 复制 size 个字节到存储区 m_cur->ptr + npos。
            if(m_cur->size == (npos + size)) {
                m_cur = m_cur->next;
            }
            m_position += size;
            bpos += size;
            size = 0;
        } else {//当前节点的所剩容量不够写下size个字节大小的buf，先将所剩的容量填满后再将未写入的buf写入下一个内存块中
            memcpy(m_cur->ptr + npos, (const char*)buf + bpos, ncap);
            m_position += ncap;
            bpos += ncap;
            size -= ncap;
            m_cur = m_cur->next;//跳到下个节点，方便继续写剩余的还未写进去的buf部分
            ncap = m_cur->size;
            npos = 0;
        }
    }

    //更新数据的大小
    if(m_position > m_size) {
        m_size = m_position;
    }
}
//从内存池读取size长度的数据
void ByteArray::read(void* buf, size_t size)
{
    if(size > getReadSize()) {//size大于可读数据的大小
        throw out_of_range("not enough len");
    }

    size_t npos = m_position % m_baseSize;//当前节点在内存块中所处的位置
    size_t ncap = m_cur->size - npos;//当前节点的容量
    size_t bpos = 0;//当前已经写入的偏移量
    while(size > 0) {
        if(ncap >= size) {//放得下size大小的数据
            memcpy((char*)buf + bpos, m_cur->ptr + npos, size);
            if(m_cur->size == (npos + size)) {
                m_cur = m_cur->next;
            }
            m_position += size;
            bpos += size;
            size = 0;
        } else {
            memcpy((char*)buf + bpos, m_cur->ptr + npos, ncap);
            m_position += ncap;
            bpos += ncap;
            size -= ncap;
            m_cur = m_cur->next;
            ncap = m_cur->size;
            npos = 0;
        }
    }
}
//从内存池某个位置开始读取size长度的数据(不影响当前操作的位置)
void ByteArray::read(void* buf, size_t size, size_t position) const
{
    //position 读取开始位置
    if(size > (m_size - position)) {
        throw std::out_of_range("not enough len");
    }

    size_t npos = position % m_baseSize;
    size_t ncap = m_cur->size - npos;
    size_t bpos = 0;
    Node* cur = m_cur;
    while(size > 0) {
        if(ncap >= size) {
            memcpy((char*)buf + bpos, cur->ptr + npos, size);
            if(cur->size == (npos + size)) {
                cur = cur->next;
            }
            position += size;
            bpos += size;
            size = 0;
        } else {
            memcpy((char*)buf + bpos, cur->ptr + npos, ncap);
            position += ncap;
            bpos += ncap;
            size -= ncap;
            cur = cur->next;
            ncap = cur->size;
            npos = 0;
        }
    }
}
//设置内存块的位置（内存池中的内存块的位置）
void ByteArray::setPosition(size_t v) 
{
    if(v > m_capacity) {
        throw out_of_range("set_position out of range");
    }
    m_position = v;
    if(m_position > m_size) {
        m_size = m_position;
    }
    m_cur = m_root;
    while(v > m_cur->size) {
        v -= m_cur->size;
        m_cur = m_cur->next;
    }
    if(v == m_cur->size) {
        m_cur = m_cur->next;
    }
}
//把ByteArray的当前所有数据写入到文件中(不影响当前操作的位置)
bool ByteArray::writeToFile(const string& name) const
{
    ofstream ofs;
    ofs.open(name, ios::trunc | ios::binary);
    if(!ofs) {
        HCSERVER_LOG_ERROR(g_logger) << "writeToFile name=" << name
            << " error , errno=" << errno << " errstr=" << strerror(errno);
        return false;
    }

    int64_t read_size = getReadSize();//得到可读数据的大小
    int64_t pos = m_position;//当前位置
    Node* cur = m_cur;

    while(read_size > 0) {
        int diff = pos % m_baseSize;//当前节点在内存块中所处的位置（偏移量，即前面读过的部分）
        int64_t len = (read_size > (int64_t)m_baseSize ? m_baseSize : read_size) - diff;
        ofs.write(cur->ptr + diff, len);
        cur = cur->next;
        pos += len;
        read_size -= len;
    }

    return true;
}
//从文件中读取数据
bool ByteArray::readFromFile(const string& name)
{
    ifstream ifs;
    ifs.open(name, ios::binary);
    if(!ifs) {
        HCSERVER_LOG_ERROR(g_logger) << "readFromFile name=" << name
            << " error, errno=" << errno << " errstr=" << strerror(errno);
        return false;
    }

    shared_ptr<char> buff(new char[m_baseSize], [](char* ptr) { delete[] ptr;});
    while(!ifs.eof()) {
        ifs.read(buff.get(), m_baseSize);
        write(buff.get(), ifs.gcount());
    }
    return true;
}
//将ByteArray里面的数据[m_position, m_size)转成string
string ByteArray::toString() const
{
    string str;
    str.resize(getReadSize());
    if(str.empty()) {
        return str;
    }
    read(&str[0], str.size(), m_position);
    return str;
}
//将ByteArray里面的数据[m_position, m_size)转成16进制的string(格式:FF FF FF)
string ByteArray::toHexString() const
{
    string str = toString();
    stringstream ss;

    for(size_t i = 0; i < str.size(); ++i) {
        if(i > 0 && i % 32 == 0) {
            ss << endl;
        }
        ss << setw(2) << setfill('0') << hex
           << (int)(uint8_t)str[i] << " ";
    }
    return ss.str();
}

//从当前位置获取可读取的缓存,保存成iovec数组(不影响当前操作的位置)
uint64_t ByteArray::getReadBuffers(vector<iovec>& buffers, uint64_t len) const
{
    len = len > getReadSize() ? getReadSize() : len;
    if(len == 0) {
        return 0;
    }

    uint64_t size = len;

    size_t npos = m_position % m_baseSize;//当前节点在内存块中所处的位置
    size_t ncap = m_cur->size - npos;//当前内存块剩余的容量
    struct iovec iov;
    Node* cur = m_cur;

    while(len > 0) {
        if(ncap >= len) {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = len;
            len = 0;
        } else {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = ncap;
            len -= ncap;
            cur = cur->next;
            ncap = cur->size;
            npos = 0;
        }
        buffers.push_back(iov);
    }
    return size;
}

//从指定位置获取可读取的缓存,保存成iovec数组,从position位置开始(不影响当前操作的位置)
uint64_t ByteArray::getReadBuffers(vector<iovec>& buffers, uint64_t len, uint64_t position) const
{
    len = len > getReadSize() ? getReadSize() : len;
    if(len == 0) {
        return 0;
    }

    uint64_t size = len;

    size_t npos = position % m_baseSize;//当前节点在内存块中所处的位置
    size_t count = position / m_baseSize;//获取内存块的数量
    Node* cur = m_root;
    while(count > 0) {//遍历得到position所在的内存块的位置指针
        cur = cur->next;
        --count;
    }

    size_t ncap = cur->size - npos;//当前内存块剩余的容量
    struct iovec iov;
    while(len > 0) {
        if(ncap >= len) {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = len;
            len = 0;
        } else {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = ncap;
            len -= ncap;
            cur = cur->next;
            ncap = cur->size;
            npos = 0;
        }
        buffers.push_back(iov);
    }
    return size;
}

//从当前位置获取可写入的缓存,保存成iovec数组(不影响当前操作的位置)
uint64_t ByteArray::getWriteBuffers(vector<iovec>& buffers, uint64_t len)
{
    if(len == 0) {
        return 0;
    }
    addCapacity(len);//判断当前内存池的容量是否足够写下len大小的buffers，如果不够则扩充内存池容量
    uint64_t size = len;

    size_t npos = m_position % m_baseSize;//当前节点在内存块中所处的位置
    size_t ncap = m_cur->size - npos;//当前内存块剩余的容量
    struct iovec iov;
    Node* cur = m_cur;
    while(len > 0) {
        if(ncap >= len) {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = len;
            len = 0;
        } else {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = ncap;
            len -= ncap;
            cur = cur->next;
            ncap = cur->size;
            npos = 0;
        }
        buffers.push_back(iov);
    }
    return size;
}

void ByteArray::addCapacity(size_t size)  //扩容ByteArray,使其可以容纳size个数据(如果原本可以容纳,则不扩容)
{
    if(size == 0) {
        return;
    }
    size_t old_cap = getCapacity();
    if(old_cap >= size) {
        return;
    }

    size = size - old_cap;
    size_t count = ceil(1.0 * size / m_baseSize);//算出要扩充的内存块数量
    Node* tmp = m_root;
    while(tmp->next) {//找到末尾的内存节点
        tmp = tmp->next;
    }

    Node* first = NULL;
    for(size_t i = 0; i < count; ++i) {
        tmp->next = new Node(m_baseSize);
        if(first == NULL) {
            first = tmp->next;
        }
        tmp = tmp->next;
        m_capacity += m_baseSize;
    }

    if(old_cap == 0) {
        m_cur = first;
    }
}

}