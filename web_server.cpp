#include "web_server.h"


web_server::web_server(){
    users = new http_parser[2000];
    char path[200];
    getcwd(path, 200);
    // /home/yuhang/tiny_webserver/web_server.cpp
    char root[6] = "/root";
    m_root = (char*)malloc(sizeof(path) + sizeof(root) + 1);
    strcpy(m_root, path);
    strcat(m_root, root);
    // /home/yuhang/tiny_webserver/web_server.cpp/root
}

web_server::~web_server(){

}

void web_server::init(int port){
    m_port = port;
    m_listenfd = -1;
    m_epollfd = -1;
    memset(events, '\0', sizeof(events));
}

void web_server::event_listen(){
    m_listenfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in seraddr;
    seraddr.sin_family = AF_INET;
    seraddr.sin_port = htons(m_port);
    seraddr.sin_addr.s_addr = INADDR_ANY;
    // 设置端口复用
    int flag = 1;
    setsockopt(m_listenfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
    int ret = bind(m_listenfd, (struct sockaddr*)&seraddr, sizeof(seraddr));
    if (ret == -1) {
        perror("bind error");
        exit(1);
    }
    listen(m_listenfd, 64);
    m_epollfd = epoll_create(64);
    addfd(m_epollfd, m_listenfd, EPOLLIN);
    http_parser::m_epollfd = m_epollfd;
}

void web_server::event_loop(){
    while(1){
        int num = epoll_wait(m_epollfd, events, 2000, -1);
        for (int i = 0; i < num; i++) {
            if (events[i].data.fd == m_listenfd){
                // 处理监听套接字
                deal_new_connect();
            }else if (events[i].events & EPOLLIN) {
                // 处理读事件
                deal_read_event(events[i].data.fd);
            }
        }
    }
}

void web_server::addfd(int epfd, int fd, __uint32_t event){
    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = event;
    epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
}

void web_server::deal_new_connect(){
    struct sockaddr_in cliaddr;
    socklen_t clilen = sizeof(cliaddr);
    int connfd = accept(m_listenfd, (struct sockaddr*)&cliaddr, &clilen);
    if (connfd < 0) {
        // 错误处理
    }
    if (http_parser::m_user_count >= 2000) {
        // 错误处理
    }
    printf("有新客户端连接\n");
    users[connfd].init(connfd, m_root);
}

void web_server::deal_read_event(int socket){
    users[socket].read_once();
}
