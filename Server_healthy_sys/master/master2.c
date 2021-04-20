
#include <stdio.h>
#include "master2.h"

int main() {
    pid_t pid = fork();//守护进程
    if (pid > 0) {
        exit(0);
    }
    setsid();//变会长
    umask(0);
    for (int i = 0; i < NR_OPEN; i++) {
        close(i);
    }
    open("/dev/null", O_RDWR);
    open("/dev/null", O_RDWR);
    open("/dev/null", O_RDWR);
    chdir("/");

    //获取配置文件中的信息;
    char *config = "/opt/pi_master/master.conf"; 
    char startIp[20] ={0}, endIp[20] = {0}; 
    int Ins, heartport, dataport, listenport, ctlport;
    char reval[20] = {0};
    long timeout;

    get_conf_value(config, "INS", reval);
    Ins = atoi(reval);
    memset(reval, 0, sizeof(reval));
    
    get_conf_value(config, "HeartPort", reval);
    heartport = atoi(reval);
    memset(reval, 0, sizeof(reval));

    get_conf_value(config, "DataPort", reval);
    dataport = atoi(reval);
    memset(reval, 0, sizeof(reval));

    get_conf_value(config, "ListenPort", reval);
    listenport = atoi(reval);
    memset(reval, 0, sizeof(reval));
    
    get_conf_value(config, "CtlPort", reval);
    ctlport = atoi(reval);
    memset(reval, 0, sizeof(reval));
    
    get_conf_value(config, "StartIp", reval);
    strcpy(startIp, reval);
    memset(reval, 0, sizeof(reval));
    
    get_conf_value(config, "EndIp", reval);
    strcpy(endIp, reval);
    memset(reval, 0, sizeof(reval)); 
    
    get_conf_value(config, "TimeOut", reval);
    timeout = atol(reval);
    memset(reval, 0, sizeof(reval)); 
    
    LinkList *linklist = (LinkList*) malloc (sizeof(LinkList) * Ins); //创建一个链表数组;
    int *sum = (int*) malloc (sizeof(int) * Ins); //sum存储每个链表的长度;
    memset(sum, 0, sizeof(int) * Ins); 
    
    //定义一个sockaddr_in类型的变量，初始化IP家族，端口，IP地址;
    struct sockaddr_in initaddr;
    initaddr.sin_family = AF_INET;
    initaddr.sin_port = htons(0);
    initaddr.sin_addr.s_addr = inet_addr("0.0.0.0");
    
    //设置并发度;
    for (int i = 0; i < Ins; i++) {
        Node *p = (Node *) malloc (sizeof(Node));
        p->addr = initaddr;
        p->fd = -1;
        p->next = NULL;
        linklist[i] = p;
    }
    
    unsigned int sip, eip;
    sip = ntohl(inet_addr(startIp));
    eip = ntohl(inet_addr(endIp));
    for (unsigned int i = sip; i <= eip; i++) {
        if (i % 256 == 0 || i % 256 == 255) continue;
        //广播域 和 网络号 不需要遍历
        Node *p = (Node*) malloc (sizeof(Node));
        initaddr.sin_port = htons(heartport);
        initaddr.sin_addr.s_addr = htonl(i);
        p->addr = initaddr;
        p->fd = -1;
        p->next = NULL;
        int sub = find_min(sum, Ins);
        insert(linklist[sub], p);
        sum[sub] += 1;
    }
    
    for (int i = 0; i < Ins; i++) {
        printf("<%d>\n", i);
        output(linklist[i]);
    }
    
    pthread_t pth_heart, pth_data[Ins];
    struct Heart heart;
    heart.ins = Ins;
    heart.linklist = linklist;
    heart.sum = sum;
    heart.timeout = timeout;
    pthread_create(&pth_heart, NULL, do_heart, (void*)&heart);
    
    struct Data darg[Ins];
    for (int i = 0; i < Ins; i++) {
        darg[i].head = linklist[i];
        darg[i].ind = i;
        darg[i].dataport = dataport;
        darg[i].ctlport = ctlport;
        //创建一个线程，发送请求数据；
        pthread_create(&pth_data[i], NULL, do_data, (void*)&darg[i]);
    }
    
    int listenfd = socket_create(listenport);
    listen_epoll(listenfd, linklist, sum, Ins, heartport);
     
     
    for (int i = 0; i < Ins; i++) {
        //等待线程结束
        pthread_join(pth_data[i], NULL);
    }
    pthread_join(pth_heart, NULL); 
    return 0;
}
