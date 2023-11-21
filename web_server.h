#include "http_parser.h"

class web_server{
public:
    web_server();
    ~web_server();

    void init(int port);
    void event_listen();
    void event_loop();
    
    void addfd(int epfd, int fd, __uint32_t event);
    void deal_new_connect();
    void deal_read_event(int socket);
public:
    int m_port;
    int m_listenfd;
    int m_epollfd;
    char* m_root;
    struct epoll_event events[2000];

    http_parser* users;
};
