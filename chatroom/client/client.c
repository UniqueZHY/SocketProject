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

char *conf = "./client.conf";
int sockfd;

char logfile[50] = {0};
void logout(int signalnum) {
    close(sockfd);
    printf("您已退出.\n");
    exit(1);
}

int main() {
    int port;
    struct Msg msg;
    char ip[20] = {0};
    port = atoi(get_value(conf, "SERVER_PORT"));
    //取出配置信息
    strcpy(ip, get_value(conf, "SERVER_IP"));//不能用conf_ans,因为是全局的，不能一直用，下次就变了
    strcpy(logfile, get_value(conf, "LOG_FILE"));
    

    if ((sockfd = socket_connect(ip, port)) < 0) {
        perror("socket_connect");
        return 1;
    }
    //strcpy(msg.from, get_value(conf, "MY_NAME"));
    struct passwd *pwd;
    pwd = getpwuid(getuid());
    
    strcpy(msg.from, pwd->pw_name);
    msg.flag = 2;
    if (chat_send(msg, sockfd) < 0) {
        return 2;
    }
    
    struct RecvMsg rmsg = chat_recv(sockfd);
    
    if (rmsg.retval < 0) {
        fprintf(stderr, "Error!\n");
        return 1;
 //必须写return，不return在server端fd已经被关掉，那就一直等待recv
    }

     //如果=3,表示服务端拒绝连接
    if (rmsg.msg.flag == 3) {
        close(sockfd);
        return 1;
    }

    signal(SIGINT, logout);
    pid_t pid;
    if ((pid = fork()) < 0){
        perror("fork");
    }
    if (pid == 0) {
        sleep(2);
        char c = 'a';
        while (c != EOF) {
            system("clear");//系统调用，清空屏幕
            printf(L_PINK"Please Input Message:"NONE"\n");
            //清零，为了下一条消息
            memset(msg.message, 0, sizeof(msg.message));//从键盘输入信息，直到回车\n结束，放到msg.message
            scanf("%[^\n]s", msg.message);//按了回车后，回车会一直在缓冲区内，需要用getchar()吃掉，不然会一直输出空信息
            c = getchar();
            msg.flag = 0;
            //发信息的时候如果是@就标记一下
            if (msg.message[0] == '@') {
                msg.flag = 1;
            } else if (msg.message[0] == '#') {
                msg.flag = 4;
            }
            if (!strlen(msg.message)) continue;
            chat_send(msg, sockfd);
        }
        close(sockfd);
    } else {
        //以“W ”打开聊天日志.标准输出stdout变成文件
        freopen(logfile, "w", stdout);//把stdout从新打开为另一个文件，printf后面的输出都重定向到该文件
        //fprintf打到一个确定文件里
        printf(L_GREEN"Server "NONE": %s\n", rmsg.msg.message);
        fflush(stdout);
        while (1) {//收
            rmsg = chat_recv(sockfd);
            if (rmsg.retval < 0) {
                printf("Error in Server!\n");
                break;
            }
            if (rmsg.msg.flag == 0) {//公聊信息
                printf(L_BLUE"%s"NONE": %s\n", rmsg.msg.from, rmsg.msg.message);
            } else if (rmsg.msg.flag == 2) {//客户端信息
                printf(YELLOW"通知信息: " NONE "%s\n", rmsg.msg.message);
            } else if (rmsg.msg.flag == 1){
                //私聊输出时加个*作为标志
                printf(L_BLUE"%s"L_GREEN"*"NONE": %s\n", rmsg.msg.from, rmsg.msg.message);
            } else {
                printf("Error!\n");
            }
            fflush(stdout);//刷新
        }
        wait(NULL);//等一会子进程，防止变成孤儿
        close(sockfd);
    }
    return 0;
}

