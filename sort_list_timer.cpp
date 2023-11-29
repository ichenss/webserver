#include "sort_list_timer.h"

sort_timer_lst::sort_timer_lst(){
    head = nullptr;
    tail = nullptr;
}

sort_timer_lst::~sort_timer_lst(){
    timer* p = head;
    while(p){
        head = p->next;
        delete p;
        p = head;
    }
}

// timernode为最小情况
void sort_timer_lst::add_timer(timer* timernode){
    if (!timernode) return;
    if (!head){
        head = tail = timernode;
    }
    if (timernode->expire < head->expire){
        timernode->next = head;
        head->prev = timernode;
        head = timernode;
        return;
    }
    add_timer(timernode, head);
}

// timernode非最小情况
void sort_timer_lst::add_timer(timer* timernode, timer* head){
    timer* prev = head;
    timer* tmp = head->next;
    while(tmp){
        if (timernode->expire < tmp->expire){
            prev->next = timernode;
            timernode->next = tmp;
            tmp->prev = timernode;
            timernode->prev = prev;
            break;
        }
        prev = tmp;
        tmp = tmp->next;
    }
    if (!tmp){
        prev->next = timernode;
        timernode->prev = prev;
        timernode->next = nullptr;
        tail = timernode;
    }
}

// 有序链表的更新
void sort_timer_lst::adjust_timer(timer* timernode){
    if (!timernode) return;
    timer* tmp = timernode->next;
    if (!tmp || (timernode->expire < tmp->expire)) return;
    if (timernode == head){
        head = head->next;
        head->prev= nullptr;
        timernode->next = nullptr;
        add_timer(timernode, head);
    }
    else{
        timernode->prev->next = timernode->next;
        timernode->next->prev = timernode->prev;
        add_timer(timernode, timernode->next);
    }
}

// 删除节点
void sort_timer_lst::del_timer(timer* timernode){
    if (!timernode) return;
    if ((timernode == head) && (timernode == tail)) {
        delete timernode;
        return;
    }
    if (timernode == head){
        head = head->next;
        head->prev = nullptr;
        delete timernode;
        return;
    }
    if (timernode == tail){
        tail = tail->prev;
        tail ->next = nullptr;
        delete timernode;
        return;
    }
    timernode->prev->next = timernode->next;
    timernode->next->prev = timernode->prev;
    delete timernode;
}

// 检测
void sort_timer_lst::tick(){
    if (!head) return;
    time_t cur = time(NULL);
    timer* tmp = head;
    while(tmp){
        if (cur < tmp->expire) break;
        tmp->cb_func(tmp->user_data);
        head = tmp->next;
        if (head) head->prev = nullptr;
        delete tmp;
        tmp = head;
    }
}