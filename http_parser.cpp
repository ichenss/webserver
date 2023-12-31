#include "http_parser.h"

const char *ok_200_title = "OK";
const char *error_400_title = "Bad Request";
const char *error_400_form = "Your request has bad syntax or is inherently impossible to staisfy.\n";
const char *error_403_title = "Forbidden";
const char *error_403_form = "You do not have permission to get file form this server.\n";
const char *error_404_title = "Not Found";
const char *error_404_form = "The requested file was not found on this server.\n";
const char *error_500_title = "Internal Error";
const char *error_500_form = "There was an unusual problem serving the request file.\n";

int http_parser::m_user_count = 0;
int http_parser::m_epollfd = -1;

void http_parser::init(int socket, char* root, sockaddr_in cliaddr){
    init();
    m_socket = socket;
    server_root = root;
    addfd(m_epollfd, socket, EPOLLIN);
    m_user_count++;
}

void http_parser::init(){
    m_check_state = CHECK_STATE_REQUESTLINE;
    m_keep_alive = false;
    m_method = GET;
    m_url = 0;
    m_version = 0;
    m_content_length = 0;
    m_host = 0;
    m_start_line = 0;
    m_checked_idx = 0;
    m_read_idx = 0;
    m_write_idx = 0;
    bytes_to_send = 0;
    m_state = -1;
    memset(m_read_buf, '\0', 2048);
    memset(m_write_buf, '\0', 1024);
    memset(m_real_file, '\0', 200);
}

void http_parser::read_once(){
    data_read = read(m_socket, m_read_buf + m_read_idx, 2048 - m_read_idx);
    if (data_read == -1) {
        perror("read error");
        exit(1);
    }
    else if (data_read == 0) {
        printf("客户端断开连接\n");
        removefd(m_epollfd, m_socket);
    }
    m_read_idx += data_read;
    //printf("------------------------\n");
    //write(1, m_read_buf, m_read_idx);
    //printf("------------------------\n");
    process();
}

void http_parser::process(){
    HTTP_CODE read_ret = process_read();
    if (read_ret == NO_REQUEST){
        fdmode(m_epollfd, m_socket, EPOLLIN);
        return;
    }
    bool write_ret = process_write(read_ret);
    if (!write_ret){
        close_conn(true);
    }
    fdmode(m_epollfd, m_socket, EPOLLOUT);
}

http_parser::HTTP_CODE http_parser::process_read(){
    // 从状态机起始状态
    LINE_STATUS line_status = LINE_OK;
    HTTP_CODE ret = NO_REQUEST;
    char* text = 0;
    while((line_status = parse_line()) == LINE_OK){
        // 获取一行需要处理的字符串
        text = get_line();
        m_start_line = m_checked_idx;
        switch(m_check_state)
        {
            case CHECK_STATE_REQUESTLINE:
            {
                ret = parser_request_line(text);
                if (ret == BAD_REQUEST) return BAD_REQUEST;
                break;
            }
            case CHECK_STATE_HEADER:
            {
                ret = parser_request_headers(text);
                if (ret == BAD_REQUEST) return BAD_REQUEST;
                else if (ret == GET_REQUEST) return do_request();
                break;
            }
            case CHECK_STATE_CONTENT:
            {
                ret = parser_content(text);
                if (ret == GET_REQUEST) return do_request();
                line_status = LINE_OPEN;
                break;
            }
            default:
            {
                return INTEARNAL_ERROR;
            }
        }
    }
    return NO_REQUEST;
}

http_parser::HTTP_CODE http_parser::parser_request_line(char* text){
    m_url = strpbrk(text, " ");
    if (!m_url) return BAD_REQUEST;
    *m_url++ = '\0';
    // 获取到请求方法
    char* method = text;
    if (strcasecmp(method, "GET") == 0){
        m_method = GET;
    }else if (strcasecmp(method, "POST") == 0){
        m_method = POST;
    }else {
        return BAD_REQUEST;
    }
    // 跳过多余空格
    m_url += strspn(m_url, " ");
    // HTTP版本信息
    m_version = strpbrk(m_url, " ");
    if (!m_version) return BAD_REQUEST;
    *m_version++ = '\0';
    m_version += strspn(m_version, " ");
    if (strcasecmp(m_version, "HTTP/1.1") != 0){
        return BAD_REQUEST;
    }
    // 如果存在跳过协议域
    if (strncasecmp(m_url, "http://", 7) == 0) {
        m_url += 7;
        m_url = strchr(m_url, '/');
    }
    if (strncasecmp(m_url, "https://", 8) == 0) {
        m_url += 8;
        m_url = strchr(m_url, '/');
    }
    if (!m_url || m_url[0] != '/') return BAD_REQUEST;
    m_check_state = CHECK_STATE_HEADER;
    //printf("m_method: %d, m_url: %s, m_version: %s\n", m_method, m_url, m_version);
    return NO_REQUEST;
}

http_parser::HTTP_CODE http_parser::parser_request_headers(char* text){
    if (text[0] == '\0'){
        if (m_content_length != 0) {
            m_check_state = CHECK_STATE_CONTENT;
            return NO_REQUEST;
        }
        return GET_REQUEST;
    }
    else if(strncasecmp(text, "Connection:", 11) == 0){
        text += 11;
        text += strspn(text, " ");
        if (strcasecmp(text, "keep-alive") == 0){
            m_keep_alive = true;
            //printf("Connection: keep_alive\n");
        }
    }
    else if (strncasecmp(text, "Content-length:", 15) == 0){
        text += 15;
        text += strspn(text, " ");
        m_content_length = atoi(text);
        //printf("Conten-length: %d\n", m_content_length);
    }
    else if (strncasecmp(text, "Host:", 5) == 0) {
        text += 5;
        text += strspn(text, " ");
        m_host = text;
        //printf("Host: %s\n", text);
    }
    else {
        // ....
    }
    return NO_REQUEST;
}

http_parser::HTTP_CODE http_parser::parser_content(char* text){
    // ....
    return NO_REQUEST;
}

http_parser::HTTP_CODE http_parser::do_request(){
    //printf("HTTP 请求分析完毕，开始处理\n");
    strcpy(m_real_file, server_root);
    int len = strlen(server_root);
    // 默认连接界面
    if (strlen(m_url) == 1 && m_url[0] == '/'){
        strncpy(m_real_file + len, "/index.html", 200 - len - 1);
    }
    else{
        strncpy(m_real_file + len, m_url, 200 - len - 1);
    }
    //printf("m_real_file: %s\n", m_real_file);
    if (stat(m_real_file, &m_file_stat) < 0) return NO_RESOURCE;
    if (!(m_file_stat.st_mode & S_IROTH)) return FORBIDDEN_REQUEST;
    if (S_ISDIR(m_file_stat.st_mode)) return BAD_REQUEST;
    int fd = open(m_real_file, O_RDONLY);
    m_file_address = (char*)mmap(NULL, m_file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);

    close(fd);
    return FILE_REQUEST;

}

http_parser::LINE_STATUS http_parser::parse_line(){
    char temp;
    for (; m_checked_idx < m_read_idx; ++m_checked_idx){
        temp = m_read_buf[m_checked_idx];
        if (temp == '\r'){
            if ((m_checked_idx + 1) == m_read_idx){
                return LINE_OPEN;
            }
            else if (m_read_buf[m_checked_idx + 1] == '\n'){
                m_read_buf[m_checked_idx++] = '\0';
                m_read_buf[m_checked_idx++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
    }
    return LINE_OPEN;
}

// 开始封装响应报文
bool http_parser::process_write(HTTP_CODE ret){
    switch(ret)
    {
        case(FILE_REQUEST):
        {
            add_status_line(200, ok_200_title);
            if (m_file_stat.st_size != 0){
                add_headers(m_file_stat.st_size);
                m_iv[0].iov_base = m_write_buf;
                m_iv[0].iov_len = m_write_idx;

                m_iv[1].iov_base = m_file_address;
                m_iv[1].iov_len = m_file_stat.st_size;
                m_iv_count = 2;
                bytes_to_send = m_write_idx + m_file_stat.st_size;
                return true;
            }
            else{
                const char* ok_string = "<html><body></body></html>";
                add_headers(strlen(ok_string));
                if (!add_content(ok_string)){
                    return false;
                }
            }
        }
        case(INTEARNAL_ERROR):
        {
            add_status_line(500, error_500_title);
            add_headers(strlen(error_500_form));
            if (!add_content(error_500_form)){
                return false;
            }
            break;
        }
        case(FORBIDDEN_REQUEST):
        {
            add_status_line(403, error_403_title);
            add_headers(strlen(error_403_form));
            if (!add_content(error_403_form)){
                return false;
            }
            break;
        }
        case(BAD_REQUEST):
        {
            add_status_line(404, error_404_title);
            add_headers(strlen(error_404_form));
            if (!add_content(error_404_form)){
                return false;
            }
            break;
        }
        default:
            return false;
    }
    m_iv[0].iov_base = m_write_buf;
    m_iv[0].iov_len = m_write_idx;
    m_iv_count = 1;
    bytes_to_send = m_write_idx;
    return true;
}

bool http_parser::add_content(const char *content){
    return add_response("%s", content);
}

// 响应头
bool http_parser::add_headers(int content_len){
    return add_content_length(content_len) && add_linger() && add_content_type() && add_blank_line();
}

// 响应报文的长度
bool http_parser::add_content_length(int content_len){
    return add_response("Content-length: %d\r\n", content_len);
}

// 响应报文的长短连接
bool http_parser::add_linger(){
    return add_response("Connection: %s\r\n", (m_keep_alive == true) ? "keep-alive" : "close");
}

// 空行
bool http_parser::add_blank_line(){
    return add_response("%s", "\r\n");
}

// 状态行
bool http_parser::add_status_line(int status, const char* title){
    return add_response("%s %d %s\r\n", "HTTP/1.1", status, title);
}

// 响应报文类型
bool http_parser::add_content_type(){
    char file_type[32];
    char* p = strrchr(m_real_file, '.');
    //printf("----------------------%s\n", p);
    strcpy(file_type, p);
    if (strlen(file_type) == 0) return false;
    if (strcasecmp(file_type, ".jpg") == 0) {
        strcpy(file_type, "image/jpeg; charset=utf-8");
    }
    else if (strcasecmp(file_type, ".gif") == 0) {
        strcpy(file_type, "image/gif; charset=utf-8");
    }
    else if(strcasecmp(file_type, ".png") == 0) {
        strcpy(file_type, "image/png; charset=utf-8");
    }
    else
        strcpy(file_type, "text/html; charset=utf-8");
    return add_response("Content-Type: %s\r\n", file_type);
}

bool http_parser::add_response(const char* format, ...){
    if (m_write_idx >= 1024) return false;
    va_list arg_list;
    va_start(arg_list, format);
    int len = vsnprintf(m_write_buf + m_write_idx, 1024 - 1 - m_write_idx, format, arg_list);
    if (len >= (1024 - 1 - m_write_idx)){
        va_end(arg_list);
        return false;
    }
    m_write_idx += len;
    va_end(arg_list);

    //printf("request:\n%s\n", m_write_buf);

    return true;
}

bool http_parser::do_write(){
    int temp = 0;
    if (bytes_to_send == 0){
        init();
        return true;
    }
    temp = writev(m_socket, m_iv, m_iv_count);
    fdmode(m_epollfd, m_socket, EPOLLIN);
    return true;
}

void http_parser::fdmode(int epfd, int fd, __uint32_t events){
    epoll_event event;
    event.events = events | EPOLLONESHOT | EPOLLET;
    event.data.fd = fd;
    epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &event);
}

void http_parser::close_conn(bool real_close){
    if (real_close && (m_socket != -1))
    {
        //printf("close: %d\n", m_socket);
        removefd(m_epollfd, m_socket);
        m_socket = -1;
        m_user_count--;
    }
}

void http_parser::addfd(int epfd, int fd, __uint32_t event){
    struct epoll_event ev;
    ev.data.fd = fd;
    // EPOLLONESHOT防止一个任务被多个线程处理 one loop per thread
    ev.events = event | EPOLLONESHOT | EPOLLET;
    epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
    // setnonblocking(fd);
}

void http_parser::removefd(int epfd, int fd){
    int ret = epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
    if (ret == -1){
        perror("epoll_ctl error");
        exit(1);
    }
    printf("fd = %d, 以接触监听\n", fd);
    close(fd);
}

//对文件描述符设置非阻塞
// int setnonblocking(int fd)
// {
//     int old_option = fcntl(fd, F_GETFL);
//     int new_option = old_option | O_NONBLOCK;
//     fcntl(fd, F_SETFL, new_option);
//     return old_option;
// }
