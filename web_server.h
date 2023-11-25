#include "http_parser.h"
#include "threadpool.h"

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

    void thread_pool();
    
public:
    int m_port;
    int m_listenfd;
    int m_epollfd;
    char* m_root;
    struct epoll_event events[2000];

    threadpool<http_parser>* m_pool;
    int m_thread_num;

    http_parser* users;
};
