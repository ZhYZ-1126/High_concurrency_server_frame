#ifndef __HCSERVER_SINGLETON_H
#define __HCSERVER_SINGLETON_H

namespace HCServer{

template<class T,class X = void,int N=0>
class Singleton{
public:
    static T *Getinstance(){
        static T v;
        return &v;
    }
};
template<class T,class X = void,int N=0>
class SingletonPtr{
public:
    static std::shared_ptr<T> Getinstance(){
        static std::shared_ptr<T> v(new T);
        return v;
    }
};
}

#endif