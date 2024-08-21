#ifndef  _HCSERVER_ENDIAN_H
#define _HCSERVER_ENDIAN_H

#include <byteswap.h>
#include<stdint.h>
#include<iostream>
using namespace std;

#define HCSERVER_LITTLE_ENDIAN 1
#define HCSERVER_BIG_ENDIAN 2

//转字节序号，从小转大，从大转小
namespace HCServer{

template<class T>
typename std::enable_if<sizeof(T)==sizeof(uint64_t),T>::type 
byteswap(T value){

    return (T)bswap_64((uint64_t)value);
}

template<class T>
typename std::enable_if<sizeof(T)==sizeof(uint32_t),T>::type 
byteswap(T value){
    return (T)bswap_32((uint64_t)value);
}

template<class T>
typename std::enable_if<sizeof(T)==sizeof(uint16_t),T>::type 
byteswap(T value){
    return (T)bswap_16((uint64_t)value);
}

#if BYTE_ORDER==BIG_ENDIAN
#define HCSERVER_BYTE_ORDER HCSERVER_BIG_ENDIAN
#else
#define HCSERVER_BYTE_ORDER HCSERVER_LITTLE_ENDIAN
#endif 

#if  HCSERVER_BYTE_ORDER==HCSERVER_BIG_ENDIAN
template <class T>
T byteswapOnLittleEndian(T t){
    return t;
}
template <class T>
T byteswapOnBigEndian(T t){
    return byteswap(t);
}
#else 
template <class T>
T byteswapOnLittleEndian(T t){
    return byteswap(t);
}
template <class T>
T byteswapOnBigEndian(T t){
    return t;
}


#endif

}



#endif 