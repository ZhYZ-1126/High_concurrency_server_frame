/*
*hook模块
*实现对系统的函数进行封装
*/


#ifndef _HCSERVER_HOOK_H__
#define _HCSERVER_HOOK_H__

#include<unistd.h>
#include<time.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/ioctl.h>
#include<fcntl.h>
#include<stdint.h>

namespace HCServer{

    bool is_hook_enable();              //返回当前线程是否hook
    void set_hook_enable(bool flag);    //设置当前线程的hook状态
}

extern "C"{//extern说明 此变量/函数是在别处定义的，要在此处引用 "C":c风格


//和sleep相关
typedef unsigned int (*sleep_fun)(unsigned int seconds);
extern sleep_fun sleep_f;

typedef int(*usleep_fun)(useconds_t usec);
extern usleep_fun usleep_f;

typedef int (*nanosleep_fun)(const struct timespec *req,struct timespec *rem); 
extern nanosleep_fun nanosleep_f; 

//socket
typedef int (*socket_fun)(int domain,int type,int protocol);    //typedef定义函数指针类型
extern socket_fun socket_f; //extern外部变量(全局静态存储区)————声明一个成员变量

extern int connect_with_timeout(int fd, const struct sockaddr* addr, socklen_t addrlen, uint64_t timeout_ms);   //(增加了超时变量)
typedef int (*connect_fun)(int sockfd,const struct sockaddr* addr,socklen_t addrlen);   //客户端连接服务器地址
extern connect_fun connect_f;   //extern外部变量(全局静态存储区)————声明一个成员变量
    
typedef int (*accept_fun)(int s,struct sockaddr* addr,socklen_t* addrlen);  //服务器接收连接
extern accept_fun accept_f; //extern外部变量(全局静态存储区)————声明一个成员变量


//read
typedef ssize_t (*read_fun)(int fd, void *buf, size_t count);
extern read_fun read_f;
    

typedef ssize_t (*readv_fun)(int fd, const struct iovec *iov, int iovcnt);
extern readv_fun readv_f;
    
typedef ssize_t (*recv_fun)(int sockfd, void *buf, size_t len, int flags);
extern recv_fun recv_f;
    
typedef ssize_t (*recvfrom_fun)(int sockfd, void *buf, size_t len, int flags,struct sockaddr *src_addr, socklen_t *addrlen);
extern recvfrom_fun recvfrom_f;
    
typedef ssize_t (*recvmsg_fun)(int sockfd, struct msghdr *msg, int flags);
extern recvmsg_fun recvmsg_f;

//write
typedef ssize_t (*write_fun)(int fd, const void *buf, size_t count);
extern write_fun write_f;

typedef ssize_t (*writev_fun)(int fd, const struct iovec *iov, int iovcnt);
extern writev_fun writev_f;
    
typedef ssize_t (*send_fun)(int sockfd, const void *buf, size_t len, int flags);
extern send_fun send_f;
    
typedef ssize_t (*sendto_fun)(int sockfd, const void *buf, size_t len, int flags,const struct sockaddr *dest_addr, socklen_t addrlen);
extern sendto_fun sendto_f;
    
typedef ssize_t (*sendmsg_fun)(int sockfd, const struct msghdr *msg, int flags);
extern sendmsg_fun sendmsg_f;


//close
typedef int (*close_fun)(int fd);
extern close_fun close_f;



//socket操作相关的
typedef int (*fcntl_fun)(int fd, int cmd,.../* arg */ );
extern fcntl_fun fcntl_f;

typedef int (*ioctl_fun)(int d, unsigned long int request, ...);
extern ioctl_fun ioctl_f;

typedef int (*getsockopt_fun)(int sockfd, int level, int optname,void *optval, socklen_t *optlen);
extern getsockopt_fun getsockopt_f;

typedef int (*setsockopt_fun)(int sockfd, int level, int optname,const void *optval, socklen_t optlen);
extern setsockopt_fun setsockopt_f;

}


#endif