/*************************************************************************
	> File Name: sender.c
	> Author: suyelu
	> Mail: suyelu@haizeix.com
	> Created Time: Thu 02 Apr 2020 06:39:54 PM CST
 ************************************************************************/

#include "./common/head.h"
#include "./common/tcp_client.h"
#include "./common/common.h"

struct FileMsg {
    long size; //文件大小
    char name[50];
    char buf[4096];
};

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s ip port!\n", argv[0]);
        return 1;
    }

    int sockfd, port;
    char buff[100] = {0};//存放指令
    char name[50] = {0};
    struct FileMsg filemsg;

    port = atoi(argv[2]);
    if ((sockfd = socket_connect(argv[1], port)) < 0) {
        perror("socket_connect");
        return 1;
    }
    
    while (1) {
        scanf("%[^\n]s", buff);
        getchar();
        if (!strncmp("send ", buff, 5)) {
            strcpy(name, buff + 5);//从命令中拷贝出文件名
        } else {
            fprintf(stderr, "invalid command!\n");
            continue;
        }
        FILE *fp = NULL;//文件指针
        size_t size;
        if ((fp = fopen(name, "rb")) == NULL) {//代开文件失败
        //rb 以二进制文件形式打开，不在关注文件类型
            perror("fopen");
            continue;
        }
        //seek函数是从文件流的头移动到尾部
        //fseek函数将文件指针指向任何位置，是个字节流
        //SEEK_SET文件开头 SEEK_END文件末尾 SEEK_CUR当前位置 
        //从头移动到到文件结尾
        fseek(fp, 0L, SEEK_END);//文件大小
        //获取fp长度
        filemsg.size = ftell(fp);//用ftell获取长度
        strcpy(filemsg.name, name);
        //将指针回到文件头部，否则只会读到EOF
        fseek(fp, 0L, SEEK_SET);
        //从 fp读1次大小为4096到filemsg.buf中
        while ((size = fread(filemsg.buf, 1, 4096, fp))) {
            //                  发的数据                   没有紧急处理
            send(sockfd, (void *)&filemsg, sizeof(filemsg), 0);
            //清空buf
            memset(filemsg.buf, 0, sizeof(filemsg.buf));
        }
        printf("After Send!\n");
    }
    close(sockfd);
    return 0;
}
