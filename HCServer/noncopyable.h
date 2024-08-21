#ifndef _HCSERVER_NONCOPYABLE_H__
#define _HCSERVER_NONCOPYABLE_H__

namespace HCServer{

class Noncopyable{
public:
    Noncopyable()=default;
    ~Noncopyable()=default;
    Noncopyable(const Noncopyable &)=delete;
    Noncopyable& operator=(const Noncopyable &)=delete;

};

}



#endif