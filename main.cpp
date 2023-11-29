#include "web_server.h"

using namespace std;

int main(int argc, char* argv[]){
    if (argc != 2) {
        printf("参数错误\n");
        exit(1);
    }

    int port = atoi(argv[1]);
    
    web_server webserver;
    webserver.init(port, 8);
    webserver.thread_pool();

    webserver.event_listen();
    webserver.event_loop();
    return 0;
}