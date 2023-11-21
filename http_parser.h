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

using namespace std;

class http_parser{
public:
    static int m_epollfd;
    static int m_user_count;

public:
    void init(int connfd, char* m_root);
    void read_once();
    
};