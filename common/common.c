/*************************************************************************
	> File Name: common.c
	> Author: suyelu
	> Mail: suyelu@haizeix.com
	> Created Time: Sat 28 Mar 2020 08:41:00 PM CST
 ************************************************************************/

#include "head.h"
//用来获取配置信息，path从哪，key 取哪个
char *get_value(char *path, char *key) {
    FILE *fp = NULL;
    ssize_t nrd;
    char *line = NULL, *sub = NULL;
    extern char conf_ans[50];//全局变量，为了ans能return出去
    size_t linecap;
    if (path == NULL || key == NULL) {
        fprintf(stderr, "Error in argument!\n");
        return NULL;
    }
    if ((fp = fopen(path, "r")) == NULL) {
        perror("fopen");
        return NULL;
    }
    while ((nrd = getline(&line, &linecap, fp)) != -1) {
        if ((sub = strstr(line, key)) == NULL) 
            continue;
        else {
            if (line[strlen(key)] == '=') {
                //分隔符也读进去了，所以-2
                strncpy(conf_ans, sub + strlen(key) + 1, nrd - strlen(key) - 2);
                *(conf_ans + nrd - strlen(key) - 2) = '\0';
                break;
            }
        }
    }
    free(line);
    fclose(fp);
    if (sub == NULL) {
        return NULL;
    }
    return conf_ans;
}



void make_nonblock_ioctl(int fd){
    unsigned long ul = 1;
    ioctl(fd, FIONBIO, &ul);
}


void make_block_ioctl(int fd) {
    unsigned long ul = 0;
    ioctl(fd, FIONBIO, &ul);
}


void make_nonblock(int fd) {
    int flag;
    if ((flag = fcntl(fd, F_GETFL)) < 0) {
        return;
    }
    flag |= O_NONBLOCK;
    fcntl(fd, F_SETFL, flag);
}

void make_block(int fd) {
    int flag;
    if ((flag = fcntl(fd, F_GETFL)) < 0) {
        return ;
    }
    flag &= ~O_NONBLOCK;
    fcntl(fd, F_SETFL, flag);
}
