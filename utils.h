#ifndef _UTILS_H
#define _UTILS_H
#include "sort_list_timer.h"
#include <signal.h>

class utils{
public:
    utils(){}
    ~utils(){}

    void init(int timeslot);

    void addfd(int epollfd, int fd);

    static void sig_handler(int sig);

    void addsig(int sig, void(handler)(int), bool restart = true);

    void timer_handler();

public:
    // 描述管道读写端的文件描述符
    static int* u_pipefd;
    // epfd
    static int u_epollfd;
    // 链表的对象
    sort_timer_lst m_timer_lst;
    int m_TIMESLOT;
};

void cb_func(http_parser* user_data);

#endif