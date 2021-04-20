
#include <stdio.h>
#include "client2.h"

int main() {
    pid_t tpid = fork();
    if (tpid > 0) {
        exit(0);
    }
    setsid();
    umask(0); //设置允许当前进程创建文件或者目录最大操作的权限
    for (int i = 0; i < NR_OPEN; i++) {
        close(i);
    }
    open("/dev/null", O_RDWR);
    open("/dev/null", O_RDWR);
    open("/dev/null", O_RDWR);
    chdir("/");

    int heartPort, dataPort, loadPort, ctlPort;
    char tmp[20] = {0};
    char *config = "/opt/pi_client/client.conf";
    char ip[20] = {0}; 

    get_conf_value(config, "IP", tmp);
    strcpy(ip, tmp);
    memset(tmp, 0, sizeof(tmp));

    get_conf_value(config, "HeartPort", tmp);
    heartPort = atoi(tmp);
    memset(tmp, 0, sizeof(tmp));

    get_conf_value(config, "DataPort", tmp);
    dataPort = atoi(tmp);
    memset(tmp, 0, sizeof(tmp));

    get_conf_value(config, "LoadPort", tmp);
    loadPort = atoi(tmp);
    memset(tmp, 0, sizeof(tmp));
    
    get_conf_value(config, "CtlPort", tmp);
    ctlPort = atoi(tmp);
    memset(tmp, 0, sizeof(tmp));
    
    pthread_mutexattr_t mattr; //一条性质
    pthread_condattr_t cattr;
    pthread_mutexattr_init(&mattr);
    pthread_condattr_init(&cattr);
    pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
    pthread_condattr_setpshared(&cattr, PTHREAD_PROCESS_SHARED);
    
    struct Share *share = NULL;
    int shmid = shmget(IPC_PRIVATE, sizeof(struct Share), IPC_CREAT | 0666);
    if (shmid < 0) {
        perror("shmget");
        write_log(Error_log, "[共享内存] [error] [process : %d] [message : %s]", getpid(), strerror(errno)); 
        exit(1);
    } 
    share = (struct Share*) shmat (shmid, NULL, 0);//将这个内存区域映射到本进程的虚拟地址空间
    if (share == (void *)-1) {
        perror("shmat");
        write_log(Error_log, "[共享内存] [error] [process : %d] [message : %s]", getpid(), strerror(errno)); 
        exit(1);
    }
    pthread_mutex_init(&share->smutex, &mattr);
    pthread_cond_init(&share->sready, &cattr);

    pid_t pid;//第一个线程
    pid = fork();
    if (pid < 0) {
        write_log(Error_log, "[开辟进程] [error] [process : %d] [message : %s]", getpid(), strerror(errno)); 
    }
    if (pid > 0) {//进程1中的父进程用于接收数据
        recv_data(dataPort, ctlPort, share);
        wait(&pid); 
    } else {
        //进程1中子进程在fork
        pid_t pid_1;
        pid_1 = fork();
        if (pid_1 < 0) {
            write_log(Error_log, "[开辟进程] [error] [process : %d] [message : %s]", getpid(), strerror(errno)); 
        }
        if(pid_1 > 0) { //原来的子进程
            recv_heart(heartPort, share);
            wait(&pid_1);
            exit(0);
        } else if (pid_1 == 0) { //原来子进程fork出的子子进程
            pid_t pid_2;
            int inx;
            for (int i = 0; i < 6; i++) {
                inx = i;
                pid_2 = fork();//fork出六个子进程
                if (pid_2 < 0) {
                    write_log(Error_log, "[开辟进程] [error] [process : %d] [message : %s]", getpid(), strerror(errno)); 
                }
                if (pid_2 == 0) break;//六个pid_2的子进程break
            }
            if (pid_2 > 0) {
                //刚才的pid_2中的父进程
                while (1) {
                    //条件触发
                    pthread_mutex_lock(&share->smutex);
                    pthread_cond_wait(&share->sready, &share->smutex); 
                    pthread_mutex_unlock(&share->smutex);

                    do_load(ip, loadPort, share);
                
                    pthread_mutex_lock(&share->smutex);
                    share->shareCnt = 0;
                    pthread_mutex_unlock(&share->smutex);

                }
                for (int i = 0; i < 6; i++) {
                    wait(&pid_2);
                }
                exit(0);
            }
            if (pid_2 == 0) {
                int cnt = 0;
                while(1) {
                    cnt += 1;
                    do_check(inx, share, cnt);
                    //一直调用6个脚本
                    if (cnt == 5) cnt = 0;
                    sleep(2);
                } 
                exit(0);
            }
        }
    }

    return 0;
}
