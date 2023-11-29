#ifndef _HTTP_PARSER_H
#define _HTTP_PARSER_H
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <poll.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/mman.h>
#include <sys/uio.h>
#include "timer.h"
using namespace std;

class timer;
class http_parser{
public:
    // HTTP请求方法
    enum METHOD{
        GET = 0,
        POST
        // ...剩六种
    };

    // 主状态机状态
    enum CHECK_STATE{
        CHECK_STATE_REQUESTLINE = 0, //分析请求行
        CHECK_STATE_HEADER, // 分析请求头
        CHECK_STATE_CONTENT // 分析请求正文
    };

    // 从状态机状态
    enum LINE_STATUS{
        LINE_OK = 0, // 一行解析成功
        LINE_BAD,   // 一行有错误
        LINE_OPEN   // 不是一个完整行
    };

    // 解析结果
    enum HTTP_CODE{
        NO_REQUEST,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURCE,
        FORBIDDEN_REQUEST,
        FILE_REQUEST,
        INTEARNAL_ERROR,
        CLOSED_CONNECTION
    };

public:
    int m_socket;
    CHECK_STATE m_check_state;  // 主状态机状态
    sockaddr_in address;

    int data_read = 0; // 接收read返回值
    int m_read_idx = 0; // 当前已读取多少字节数据
    int m_write_idx = 0; // m_write_buf中有效数据的大小
    int m_checked_idx = 0; // 当前已经分析完多少个字节的客户数据
    int m_start_line = 0; // 行在buffer中的起始位置

    int m_content_length; // 请求数据大小(Get无请求数据)

    char* m_url;
    char* m_version;
    METHOD m_method;   // 请求方法

    bool m_keep_alive;  // 长连接or短连接
    char* m_host;   //Host
    char* m_string; //存储HTTP请求数据  GET请求无此数据

    char* server_root;  // 服务器资源根路径
    struct stat m_file_stat;  // 目标文件的属性
    char* m_file_address;  // 请求资源的映射地址(mmap)
    struct iovec m_iv[2];
    int m_iv_count;
    /*
    ssize_t writev(int fd, const struct iovec *iov, int iovcnt);
    struct iovec {
        void  *iov_base;    //首地址
        size_t iov_len;     //长度
    };*/

    int bytes_to_send; // 响应头+响应正文总长

    int m_state;  // 判断读or写事件

    static int m_epollfd;  // epfd
    static int m_user_count;

    timer* m_timer;

public:
    void init();
    void init(int socket, char* m_root, sockaddr_in cliaddr);
    void read_once();
    void process();
    HTTP_CODE process_read();
    bool process_write(HTTP_CODE ret);
    LINE_STATUS parse_line();
    void close_conn(bool real_close);
    bool do_write();
    void fdmode(int epfd, int fd, __uint32_t events);
    HTTP_CODE do_request();
    

    char* get_line(){
        return m_read_buf + m_start_line;
    }

    HTTP_CODE parser_request_line(char* text);
    HTTP_CODE parser_request_headers(char* text);
    HTTP_CODE parser_content(char* text);

    bool add_headers(int content_len);
    bool add_content_length(int content_len);
    bool add_linger();
    bool add_content_type();
    bool add_blank_line();
    bool add_content(const char *content);

    bool add_status_line(int status, const char* title);
    bool add_response(const char* format, ...);
    static void addfd(int epfd, int fd, __uint32_t event);
    static void removefd(int epfd, int fd);

private:
    char m_read_buf[2048];  // 读缓冲
    char m_write_buf[1024];  // 写缓冲
    char m_real_file[200];  // 请求资源真实路径名
};

#endif