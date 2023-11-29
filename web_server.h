#ifndef _WEB_SERVER_H
#define _WEB_SERVER_H
#include "http_parser.h"
#include "threadpool.h"
#include "utils.h"
#define TIME_OUT 5
class web_server{
public:
    web_server();
    ~web_server();

    void init(int port, int thread_num);
    void event_listen();
    void event_loop();
    
    void addfd(int epfd, int fd, __uint32_t event);
    void deal_new_connect();
    void deal_read_event(int socket);
    void deal_write_event(int socket);

    void thread_pool();

    void newtimer(int connfd, char* root, sockaddr_in client_address);
    void deal_signal_event(bool &timeout);
public:
    int m_port;
    int m_listenfd;
    int m_epollfd;
    char* m_root;
    struct epoll_event events[2000];
    http_parser* users;

    threadpool<http_parser>* m_pool;
    int m_thread_num;

    int m_pipefd[2];
    utils util;
};
#endif