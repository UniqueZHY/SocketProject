/*************************************************************************
	> File Name: client.c
	> Author: suyelu
	> Mail: suyelu@haizeix.com
	> Created Time: 日  3/29 18:16:00 2020
 ************************************************************************/

#include "../common/chatroom.h"
#include "../common/tcp_client.h"
#include "../common/common.h"
#include "../common/color.h"
#include <sys/wait.h>
#include <signal.h>
char *conf = "./client.conf";
int sockfd;
//退出
void *logout(int signalnum) {
    close(sockfd);
    exit(1);
    printf("recv a signal");
}


int main() {
    int port;
    struct Msg msg;
    char ip[20] = {0};
    port = atoi(get_value(conf, "SERVER_PORT"));
    strcpy(ip, get_value(conf, "SERVER_IP"));//不能用conf_ans,因为是全局的，不能一直用，下次就变了
    printf("ip = %s , port = %d\n", ip, port);
    
    if ((sockfd = socket_connect(ip, port)) < 0) {
        perror("socket_connect");
        return 1;
    }
    strcpy(msg.from, get_value(conf, "MY_NAME"));
    printf("%s\n", msg.from);
    msg.flag = 2;
    if (chat_send(msg, sockfd) < 0) {
        return 2;
    }
    
    struct RecvMsg rmsg = chat_recv(sockfd);

    if (rmsg.retval < 0) {
        fprintf(stderr, "Error!\n");
        return 1;
        //必须写return，不return在server端fd已经被关掉，那就一直等待recv，但是没有
    }
    //服务器通知信息
    printf(GREEN"Server "NONE": %s", rmsg.msg.message);
    //如果=3,表示服务端拒绝连接
    if (rmsg.msg.flag == 3) {
        close(sockfd);
        return 1;
    }
    //同意连接
    pid_t pid;
    if ((pid = fork()) < 0){
        perror("fork");
    }
    if (pid == 0) {
        signal(SIGINT, logout);
        //系统调用，清空屏幕
        system("clear");
        while (1) {
            printf(L_PINK"Please Input Message:"NONE"\n");
            //从键盘输入信息，直到回车\n结束，放到msg.message
            scanf("%[^\n]s", msg.message);
//按了回车后，回车会一直在缓冲区内，需要用getchar()吃掉，不然会一直输出空信息
            getchar();
            chat_send(msg, sockfd);
            //发送完清零，以便后面用
            memset(msg.message, 0, sizeof(msg.message));
            system("clear");
        }
    } else {//父进程
        wait(NULL);//等一会子进程，防止变成孤儿
        close(sockfd);
    }
    return 0;
}


