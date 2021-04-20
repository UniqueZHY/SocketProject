/*************************************************************************
	> File Name: server.c
	> Author: suyelu
	> Mail: suyelu@haizeix.com
	> Created Time: 日  3/29 16:12:32 2020
 ************************************************************************/

#include "../common/common.h"
#include "../common/tcp_server.h"
#include "../common/chatroom.h"
#include "../common/color.h"

struct User {//用户结构体
    char name[20];
    int online;//在线人数
    pthread_t tid;//线程ID
    int fd;//哪一个连接
};


char *conf = "./server.conf";

struct User *client;//用户连接
int sum = 0;


void get_online(char *message) {
    int cnt = 0;
    char tmp[25] = {0};
    sprintf(message, "当前有 ");
    for (int i = 0; i < MAX_CLIENT; i++) {
        if(client[i].online) {
            if (cnt != 0 ) sprintf(tmp, ",%s", client[i].name);
            else sprintf(tmp, "%s", client[i].name);
            strcat(message, tmp);
            cnt++;
            if (cnt >= 50) break;
        } 
    }
    sprintf(tmp, " 等%d个用户在线", sum);
    strcat(message, tmp);
}

void send_all(struct Msg msg) {
    for (int i = 0; i < MAX_CLIENT; i++) {
        if (client[i].online)
            chat_send(msg, client[i].fd);//对每一个发送
    }
}
void send_all_ex(struct Msg msg, int sub) {
    for (int i = 0; i < MAX_CLIENT; i++) {
        if (sub == i) continue;
        if (client[i].online)
            chat_send(msg, client[i].fd);
    }
}

int check_name(char *name) {
    for (int i = 0; i < MAX_CLIENT; i++) {
        //在线并且等于待查找的
        if (client[i].online && !strcmp(client[i].name, name)) 
            return i;//返回下标
    }
    return -1;
}

void *work(void *arg){
    int sub = *(int *)arg;
    int client_fd = client[sub].fd;
    struct RecvMsg rmsg;
    printf(GREEN"Login "NONE" : %s\n", client[sub].name);
    rmsg.msg.flag = 2;
    sprintf(rmsg.msg.message, "你的好友 %s 上线了，和他打个招呼吧😁", client[sub].name);
    send_all_ex(rmsg.msg, sub);
    while (1) {
        rmsg = chat_recv(client_fd);
        if (rmsg.retval < 0) {//没收到信息，客户的退出
            printf(PINK"Logout: "NONE" %s \n", client[sub].name);
            sprintf(rmsg.msg.message, "好友 %s 已下线.", client[sub].name);
            close(client_fd);
            client[sub].online = 0;//下线
            sum--;//在线人数减一
            rmsg.msg.flag = 2;
            send_all(rmsg.msg);
            return NULL;
        }

        if (rmsg.msg.flag == 0) {//判断消息类型，0公聊信息
            printf(BLUE"%s"NONE" : %s\n",rmsg.msg.from, rmsg.msg.message);
            if (!strlen(rmsg.msg.message)) continue;
            send_all(rmsg.msg);
        } else if (rmsg.msg.flag == 1) {//私聊
            if (rmsg.msg.message[0] == '@') {//正确私聊信息
                char to[20] = {0};//存私聊收信息人的名字
                int i = 1;
                for (; i <= 20; i++) {//找名字从@名字中
                    if (rmsg.msg.message[i] == ' ')//名字在空格的前一位
                        break;
                }
                strncpy(to, rmsg.msg.message + 1, i - 1);
                int ind;
                if ((ind = check_name(to)) < 0) {
                    //告知不在线
                    sprintf(rmsg.msg.message, "%s is not online.", to);
                    rmsg.msg.flag = 2;
                    //将收私信的人不在线的消息给发私信人
                    chat_send(rmsg.msg, client_fd);
                    continue;
                } else if (!strlen(rmsg.msg.message + i)) {
                    //消息不能为空
                    sprintf(rmsg.msg.message, "私聊消息不能为空");
                    rmsg.msg.flag = 2;
                    chat_send(rmsg.msg, client_fd);
                    continue;
                }
                printf(L_PINK"Note"NONE": %s 给 %s 发送了一条私密信息\n", rmsg.msg.from, to);
                chat_send(rmsg.msg, client[ind].fd);//发给收私信人
            }
        } else if (rmsg.msg.flag == 4 && rmsg.msg.message[0] == '#') {
            printf(L_PINK"Note"NONE": %s查询了在线人数\n", rmsg.msg.from);
            get_online(rmsg.msg.message);
            rmsg.msg.flag = 2;
            chat_send(rmsg.msg, client_fd); 
        }
    }
    return NULL;
}


int find_sub() {//给新的用户在client数组中找空位置
    for (int i = 0; i < MAX_CLIENT; i++) {
        if (!client[i].online) return i;
    }
    return -1;
}

bool check_online(char *name) {
    for (int i = 0; i < MAX_CLIENT; i++) {
        //在线 并且 名字不重复
        if (client[i].online && !strcmp(name, client[i].name)) {
            printf(YELLOW"WARNING"NONE": %s is online\n", name);
            return true;//不让连接避免重复登录
        }
    }
    return false;//让
}


int main() {
    int port, server_listen, fd;
    struct RecvMsg recvmsg;
    struct Msg msg;
    port = atoi(get_value(conf, "SERVER_PORT"));
    client = (struct User *)calloc(MAX_CLIENT, sizeof(struct User));

    if ((server_listen = socket_create(port)) < 0) {
        perror("socket_create");
        return 1;
    }
    while (1) { 
        if ((fd = accept(server_listen, NULL, NULL)) < 0) {
            perror("accept");
            continue;
        }
        
        recvmsg = chat_recv(fd);
        if (recvmsg.retval < 0) {
            close(fd);
            continue;
        }
        //已经在线，重复登录，拒绝连接
        if (check_online(recvmsg.msg.from)) {
            msg.flag = 3;//重复登录flag
            strcpy(msg.message, "You have Already Login System!");
            chat_send(msg, fd);
            close(fd);
            continue;
            //拒绝连接：
        } 
        msg.flag = 2;
        strcpy(msg.message, "Welcome to this chat room!");
        chat_send(msg, fd);

        int sub, ret;
        sum++;//来一个人登录时在线人数加一个
        sub = find_sub();
        client[sub].online = 1;
        client[sub].fd =fd;
        strcpy(client[sub].name, recvmsg.msg.from);
        ret = pthread_create(&client[sub].tid, NULL, work, (void *)&sub);
        if (ret != 0) {
            fprintf(stderr, "pthread_create");
        }
    }
    return 0;
}
