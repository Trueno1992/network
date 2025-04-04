#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <sys/epoll.h>  //epoll头文件

#define MAXSIZE 10
#define IP_ADDR "10.2.20.10"
#define IP_PORT 8888
#include<iostream>
using namespace std;

int main()
{
    int i_listenfd, i_connfd;
    struct sockaddr_in st_sersock;
    char msg[MAXSIZE + 1];
    int nrecvSize = 0;

    struct epoll_event ev, events[MAXSIZE];
    int epfd, nCounts;  //epfd:epoll实例句柄, nCounts:epoll_wait返回值

    if((i_listenfd = socket(AF_INET, SOCK_STREAM, 0) ) < 0) //建立socket套接字
    {
        printf("socket Error: %s (errno: %d)\n", strerror(errno), errno);
        exit(0);
    }

    memset(&st_sersock, 0, sizeof(st_sersock));
    st_sersock.sin_family = AF_INET;  //IPv4协议
    //st_sersock.sin_addr.s_addr = htonl(INADDR_ANY);
    inet_pton(AF_INET, IP_ADDR, &st_sersock.sin_addr);
    //INADDR_ANY转换过来就是0.0.0.0，泛指本机的意思，也就是表示本机的所有IP，因为有些机子不止一块网卡，多网卡的情况下，这个就表示所有网卡ip地址的意思。
    st_sersock.sin_port = htons(IP_PORT);

    if(bind(i_listenfd,(struct sockaddr*)&st_sersock, sizeof(st_sersock)) < 0) //将套接字绑定IP和端口用于监听
    {
        printf("bind Error: %s (errno: %d)\n", strerror(errno), errno);
        exit(0);
    }

    if(listen(i_listenfd, 20) < 0)  //设定可同时排队的客户端最大连接个数
    {
        printf("listen Error: %s (errno: %d)\n", strerror(errno), errno);
        exit(0);
    }

    if((epfd = epoll_create(MAXSIZE)) < 0)  //创建epoll实例
    {
        printf("epoll_create Error: %s (errno: %d)\n", strerror(errno), errno);
        exit(-1);
    }
    ev.events = EPOLLIN;
    ev.data.fd = i_listenfd;
    if(epoll_ctl(epfd, EPOLL_CTL_ADD, i_listenfd, &ev) < 0)
    {
        printf("epoll_ctl Error: %s (errno: %d)\n", strerror(errno), errno);
        exit(-1);
    }
    printf("======waiting for client's request======\n");
    //准备接受客户端连接
    while(1)
    {
        if((nCounts = epoll_wait(epfd, events, MAXSIZE, -1)) < 0)
        {
            printf("epoll_ctl Error: %s (errno: %d)\n", strerror(errno), errno);
            exit(-1);
        }
        else if(nCounts == 0)
        {
            printf("time out, No data!\n");
        }
        else
        {
            for(int i = 0; i < nCounts; i++)
            {
                int tmp_epoll_recv_fd = events[i].data.fd;
                if(tmp_epoll_recv_fd == i_listenfd) //有客户端连接请求
                {
                    if((i_connfd = accept(i_listenfd, (struct sockaddr*)NULL, NULL)) < 0)   //阻塞等待客户端连接
                    {
                        printf("accept Error: %s (errno: %d)\n", strerror(errno), errno);
                    //  continue;
                    }
                    else
                    {
                        printf("Client[%d], welcome!\n", i_connfd);
                    }
                    ev.events = EPOLLIN;
                    ev.data.fd = i_connfd;
                    if(epoll_ctl(epfd, EPOLL_CTL_ADD, i_connfd, &ev) < 0)
                    {
                        printf("epoll_ctl Error: %s (errno: %d)\n", strerror(errno), errno);
                        exit(-1);
                    }
                }
                else    //若是已连接的客户端发来数据请求
                {
                    //接受客户端发来的消息并作处理(小写转大写)后回写给客户端
                    memset(msg, '\0' ,sizeof(msg));
                    if((nrecvSize = read(tmp_epoll_recv_fd, msg, MAXSIZE)) < 0)
                    {
                        printf("read Error: %s (errno: %d)\n", strerror(errno), errno);
                        continue;
                    }
                    else if( nrecvSize == 0)    //read返回0代表对方已close断开连接。
                    {
                        printf("client has disconnected!\n");
                        epoll_ctl(epfd, EPOLL_CTL_DEL, tmp_epoll_recv_fd, NULL);
                        close(tmp_epoll_recv_fd);  //
                        continue;
                    }
                    else
                    {
                        printf("recvMsg:%d,%s\n", nrecvSize,msg);
                        for(int i=0; msg[i] != '\0'; i++)
                        {
                            msg[i] = toupper(msg[i]);
                        }
                        std::cout<<"strlen(msg)="<<strlen(msg)<<std::endl;
                        if(write(tmp_epoll_recv_fd, msg, strlen(msg)) < 0)
                        {
                            printf("write Error: %s (errno: %d)\n", strerror(errno), errno);
                        }

                    }
                }
            }
        }
    }//while
    close(i_listenfd);
    close(epfd);
    return 0;
}
