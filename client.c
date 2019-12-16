#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/un.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <assert.h>
#include "pkt_process.h"

char recv_buf[1500];
char* send_buf;

void pthread_function(void* sock_fd){
    int sockfd=*(int*)sock_fd;
    long recvbytes;
	char operation;
	char *tmp;
    while(1) {
        if((recvbytes=recv(sockfd,recv_buf,1500,0))==-1){
            printf("recv error");
            exit(1);
        }
		tmp = recv_buf;
		operation = readChar(&tmp);
		printf("operation: %d\n", operation);
		
		if ((int)operation == Packet_RC_pub) {
			printf("publish message received:\n");
			printf("\t%s\n", tmp);
			tmp = send_buf;
			writeChar(&tmp, Packet_RC_puback);
			int ret = send(sockfd, send_buf, strlen(send_buf), 0);
			printf("publish ack %d bytes sent\n", ret);
		} else if ((int)operation == Packet_RC_unsuback) {
			printf("unsubscription success!\n");
		} else if ((int)operation == Packet_RC_suback) {
			printf("subscription success!\n");
		} else if ((int)operation == Packet_RC_errfull) {
			printf("ERROR: channel full\n");
		} else if ((int)operation == Packet_RC_errmax) {
			printf("ERROR: max topic number reached\n");
		} else if ((int)operation == Packet_RC_errno) {
			printf("ERROR: not subscribe to the channel\n");
		} else {
			printf("ERROR: bad recv packet\n");
		}

    }
} 
int main(void){
    pthread_t id;
    int sockfd;
	int len;
    struct sockaddr_in sever_addr;
    //struct sockaddr_in sever_addr, client_addr;
	//char client_port[20];

	send_buf = (char *)malloc(1024);
	char *message  = (char *)malloc(1024);
	char *tmp;

    sever_addr.sin_family=AF_INET;
    sever_addr.sin_port=htons(11211);
    sever_addr.sin_addr.s_addr=inet_addr("127.0.0.1");

	//printf("input client port:\n");
	//scanf("%s", client_port);
	//client_addr.sin_family=AF_INET;
	//client_addr.sin_port=htons(atoi(client_port));
	//client_addr.sin_addr.s_addr=htonl(INADDR_ANY);

    if((sockfd=socket(AF_INET,SOCK_STREAM,0))==-1){
        printf("socket error");
        exit(1);
    }

	//if(bind(sockfd, (struct sockaddr*)&client_addr, sizeof(struct sockaddr))==-1){
	//	printf("bidn error\n");
	//	exit(1);
	//}

    if(connect(sockfd,(struct sockaddr*)&sever_addr,sizeof(sever_addr))==-1){
        printf("connect error");
        exit(1);
    }
    char name[20];

	if(pthread_create(&id,NULL,(void*)pthread_function,(void*)&sockfd)!=0)
        printf("create thread error\n");
	
	printf("Client initialization success\n");
    while (1) {
		printf("\nPlease input operation:\n---------->");
		len = scanf("%s", name);
		assert(len > 0);
		if (strcmp(name, "SUB") == 0) {
			do {
				printf("please input topic you wan to subscribe.\nIt has to be 16 bytes long\n---------->");
				len = scanf("%s", name);
				assert(len > 0);
				if (strlen(name) != 16) printf("Error, input len: %d\n", strlen(name));
			} while (strlen(name) != 16);

			tmp = send_buf;
			writeChar(&tmp, Packet_RC_sub);
			memcpy(send_buf+1, name, 16);
			//printf("%s\n", send_buf);
			send(sockfd,send_buf,strlen(send_buf),0);
		} else if (strcmp(name, "PUB") == 0) {
			printf("input publish message\n---------->");
			len = scanf("%s", message);
			assert(len > 0);
			//fflush(stdin);
			tmp = send_buf;
			writeChar(&tmp, Packet_RC_pub);
			memcpy(send_buf+1, message, strlen(message));
			if(send(sockfd,send_buf,strlen(send_buf),0)==-1){
				printf("send error");
				exit(1);
			}
		} else if (strcmp(name, "USUB") == 0){
			do {
				printf("please input topic you wan to unsubscribe.\nIt has to be 16 bytes long\n---------->");
				len = scanf("%s", name);
				assert(len > 0);
				if (strlen(name) != 16) printf("Error, input len: %d\n", strlen(name));
			} while (strlen(name) != 16);

			tmp = send_buf;
			writeChar(&tmp, Packet_RC_unsub);
			memcpy(send_buf+1, name, 16);
			//printf("%s\n", send_buf);
			send(sockfd,send_buf,strlen(send_buf),0);
		} else {
			printf("BAD operation\n");
			printf("input SUB for subscription\n");
			printf("input PUB for publication\n");
			printf("input USUB for unsubscription\n");
		}
		sleep(2);
    }
	free(message);
	free(send_buf);
    close(sockfd);
    pthread_cancel(id);
    return 0;
}
