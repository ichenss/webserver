#include "web_server.h"

web_server::web_server(){
    users = new http_parser[MAX_USER];
    char path[200];
    getcwd(path, 200);
    // /home/yuhang/tiny_webserver/
    char root[6] = "/root";
    m_root = (char*)malloc(strlen(path) + strlen(root) + 1);
    strcpy(m_root, path);
    strcat(m_root, root);
    // /home/yuhang/tiny_webserver/root
}

web_server::~web_server(){

}

void web_server::init(int port, int thread_num){
    m_port = port;
    m_listenfd = -1;
    m_epollfd = -1;
    m_thread_num = thread_num;
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
    http_parser::m_epollfd = m_epollfd;
    addfd(m_epollfd, m_listenfd, EPOLLIN);

    utils::u_epollfd = m_epollfd;

    pipe(m_pipefd);
    utils::u_pipefd = m_pipefd;

    addfd(m_epollfd, m_pipefd[0], EPOLLIN);

    // 初始化+信号捕捉
    util.init(5);
    util.addsig(SIGALRM, util.sig_handler);

    // 开始循环监测
    alarm(util.m_TIMESLOT);
}

void web_server::event_loop(){
    bool timeout = false;
    while(1){
        int num = epoll_wait(m_epollfd, events, MAX_EVENTS, -1);
        for (int i = 0; i < num; i++) {
            if (events[i].data.fd == m_listenfd){
                // 处理监听套接字
                deal_new_connect();
            }
            else if(m_pipefd[0] == events[i].data.fd && events[i].events & EPOLLIN){
                deal_signal_event(timeout);
            }
            else if (events[i].events & EPOLLIN) {
                // 处理读事件
                deal_read_event(events[i].data.fd);
            }else if (events[i].events & EPOLLOUT){
                // 处理写事件
                deal_write_event(events[i].data.fd);
            }
        }
        if(timeout == true){
            //printf("检测链表中定时器事件是否超时\n");
            util.timer_handler();
            timeout = false;
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
        perror("accept error");
        exit(1);
    }
    if (http_parser::m_user_count >= MAX_USER) {
        printf("当前以达到最大客户端连接\n");
        return;
    }
    //printf("有新客户端连接\n");
    newtimer(connfd, m_root, cliaddr);
}

void web_server::deal_read_event(int socket){
    m_pool->append(users + socket, 0);
}

void web_server::deal_write_event(int socket){
    m_pool->append(users + socket, 1);
}

void web_server::thread_pool(){
    m_pool = new threadpool<http_parser>(m_thread_num);
}

 void web_server::newtimer(int connfd, char* root, sockaddr_in client_address){
    users[connfd].init(connfd, root,client_address);
    timer* timernode = new timer;
    timernode->user_data = &users[connfd];
    timernode->cb_func = cb_func;
    time_t cur = time(NULL);
    timernode->expire = cur + TIME_OUT * 3;
    util.m_timer_lst.add_timer(timernode);
 }

  void web_server::deal_signal_event(bool &timeout){
    int ret;
    char signal[1024];
    ret = read(m_pipefd[0], signal, sizeof(signal));
    if(ret > 0) timeout = true;
 }