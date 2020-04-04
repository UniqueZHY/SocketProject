/*************************************************************************
	> File Name: recver.c
	> Author: 
	> Mail: 
	> Created Time: 2020年04月04日 星期六 12时03分04秒
 ************************************************************************/

#include "./common/head.h"
#include "./common/tcp_server.h"
#include "./common/common.h"

struct FileMsg{
    long size;
    char name[50];
    char buf[4096];

};
void signal_process(int sig) {
    wait(NULL);
}//信号函数等待
//子进程应该做的事情
void child_do(int fd) {
    size_t recv_size, size = 0;
    struct FileMsg packet_t, packet, packet_pre;
    int packet_size = sizeof(struct FileMsg);//FileMsg 的大小
    int offset = 0, flag = 0, cnt = 0;//flag表示有没有粘包，拆包的情况
    long filesize;
    printf("Before recv!\n");
    FILE *fp = NULL;
    while (1) {
        if (flag) {//flag不为0表示存在粘包问题
            memcpy(&packet, &packet_pre, flag);//
            offset = flag;//
        }
        flag = 0;
        while ((recv_size = recv(fd, (void *)&packet_t, packet_size, 0)) > 0) {
            if (offset + recv_size == packet_size) {
                memcpy((char *)&packet + offset, &packet_t, recv_size);
                offset = 0;
                printf("整包 size = %d!\n", packet_size);
                break;
            } else if (offset + recv_size < packet_size) {
                memcpy((char *)&packet + offset, &packet_t, recv_size);
                printf("拆包 size = %ld!\n", offset + recv_size);
                offset += recv_size;
            } else {
                memcpy((char *)&packet + offset, &packet_t, packet_size - offset);
                flag = recv_size - (packet_size - offset);
                memcpy(&packet_pre, (char *)&packet_t + packet_size - offset, flag);
                printf("粘包 size = %d!\n", flag);
                offset = 0;
                break;
            }
        }
        //cnt为假的时候表示第一次
        if (!cnt) {
            printf("recv %s with size = %ld \n", packet.name, packet.size);
            char t_name[100] = {0};
            sprintf(t_name, "./data/%s", packet.name);
            if ((fp = fopen(t_name, "wb")) == NULL) {
                perror("fopen");
                break;
            }
        }
        cnt++;//到这为止每次收到的是整包 
        //写文件保存
        //size已经写的大小
        size_t write_size;
        if (packet.size - size >= packet_size) {
            write_size = fwrite(packet.buf, 1, sizeof(packet.buf), fp);
        } else {
            write_size = fwrite(packet.buf, 1, packet.size - size, fp);
        }
        size += write_size;
        if (write_size < 0) {
            perror("write");
        }
        if (write_size <= 0) {
            break;
        }
        if (size >= packet.size) {//写完的大小大于文件的大小
            printf("Finish!\n");
            break;
        }
    }
    close(fd);
    fclose(fp);
}


int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s port!\n", argv[0]);
        return 1;
    }
    int server_listen, port, fd;
    pid_t pid;
    port = atoi(argv[1]);
    if ((server_listen = socket_create(port)) < 0) {//监听的一个套接字
        perror("socket_create");
        return 1;
    }

    signal(SIGCHLD, signal_process);
    while (1) {
        if ((fd = accept(server_listen, NULL, NULL)) < 0) {//收文件
            perror("accept");
            continue;
        }
        printf("After accept\n");
        if ((pid = fork()) < 0) {
            perror("fork");
            continue;
        }
        if (pid == 0) {
            close(server_listen);
            child_do(fd);
            //child
            exit(0);
        } else {
            //parent
            close(fd);
        }
    }

    return 0;
}
