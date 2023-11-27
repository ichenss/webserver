#include <list>
#include <pthread.h>
#include "locker.h"

// 互斥锁+信号量实现
template <typename T>
class threadpool{
public:
    threadpool(int thread_number = 8, int max_request = 10000);
    ~threadpool();
    bool append(T* request, int state);

private:
    int m_thread_number;  //线程池中线程个数
    int m_max_request;  //请求队列中允许的请求最大数
    pthread_t* m_threads;  //描述线程池的数组，大小为m_pthread_number
    std::list<T*> m_workqueue;  //请求队列
    locker m_queuelocker;  //保护请求队列的互斥锁
    sem m_queuestat;  //是否有任务需要处理
    bool m_stop;  //是否结束线程

private:
    static void* worker(void* arg);
    void run();
};

template <typename T>
threadpool<T>::threadpool(int thread_number, int max_request):
m_thread_number(thread_number), m_max_request(max_request), m_stop(false), m_threads(NULL)
{
    if ((thread_number <= 0) || (max_request <= 0)) throw std::exception();

    m_threads = new pthread_t[m_thread_number];
    if (!m_threads) throw std::exception();
    // 创建thread_number个线程，并设置为脱离线程
    for (int i = 0; i < thread_number; i++){
        printf("create the %dth thread\n", i);
        if (pthread_create(m_threads + i, NULL, worker, this) != 0){
            delete []m_threads;
            throw std::exception();
        }
        if (pthread_detach(m_threads[i]) != 0){
            delete []m_threads;
            throw std::exception();
        }
    }
}

template <typename T>
threadpool<T>::~threadpool(){
    delete []m_threads;
    m_stop = true;
}

template <typename T>
bool threadpool<T>::append(T* request, int state){
    m_queuelocker.lock();
    if (m_workqueue.size() > m_max_request){
        m_queuelocker.unlock();
        return false;;
    }
    request->m_state = state;
    m_workqueue.push_back(request);
    m_queuelocker.unlock();
    m_queuestat.post();
    return true;
}

template <typename T>
void* threadpool<T>::worker(void* arg){
    threadpool* pool = (threadpool*)arg;
    pool->run();
    return pool;
}

template <typename T>
void threadpool<T>::run(){
    while(!m_stop){
        m_queuestat.wait();
        m_queuelocker.lock();
        if (m_workqueue.empty()){
            m_queuelocker.unlock();
            continue;
        }
        T* request = m_workqueue.front();
        m_workqueue.pop_front();
        m_queuelocker.unlock();
        if (!request) continue;
        if (request->m_state == 0) request->read_once();
        else if (request->m_state == 1) request->do_write();
        
    }
}