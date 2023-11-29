#ifndef _LIST_TIMER_H
#define _LIST_TIMER_H
#include "timer.h"

class sort_timer_lst{
public:
    sort_timer_lst();
    ~sort_timer_lst();

    void add_timer(timer* timernode);

    void adjust_timer(timer* timernode);

    void del_timer(timer* timernode);

    void tick();

private:
    void add_timer(timer* timernode, timer* lst_head);
public:
    timer* head;
    timer* tail;
};

#endif