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
#include <pthread.h>
#include "pkt_process.h"

typedef int (*pf)(char*, int, int, char*);

#define COUNT 5
#define TOPIC_NUM 10
#define TOPIC_LEN 16
int *socket_fd[TOPIC_NUM][COUNT];
int free_head[TOPIC_NUM];
char* topic_name[TOPIC_NUM];
int free_topic_head;

char* send_buf;

int empty=5;

/*static inline int ps_faa(int* variable, int value)
{
	__asm__ volatile("lock; xaddl %0, %1"
					: "+r" (value), "+m" (*variable)
					:
					: "memory"
	);
	return value;
}*/

static inline int ps_faa(int* Ptr, int Addend)
{
	__asm__ __volatile__("LOCK XADDL %[Addend], %[Ptr]" 
					:[Ptr]"+m"(*Ptr), [Addend]"+r"(Addend) 
					: 
					:"memory", "cc");
	return Addend;
}

void Packet_error(int sock, char* send_buf, char ERROR_CODE) {
	char* tmp;

	tmp = send_buf;
	writeChar(&tmp, ERROR_CODE);
	if (send(sock, send_buf, 1, 0)<0) {
		printf("send error\n");
		exit(1);
	}
	printf("ERROR sent.\n");
}

int Packet_sub(char* data, int len, int sock, char* send_buf)
{
	int i;
	int curr;
	char* tmp;

	printf("this is Packet_sub\n");
	for (i = 0; i < TOPIC_NUM; i++) {
		if (memcmp(data, topic_name[i], TOPIC_LEN) == 0) {
			curr = ps_faa(&free_head[i], 1);
			if (curr >= COUNT) {
				printf("ERROR1\n");
				Packet_error(sock, send_buf, Packet_RC_errfull);
				//exit(1);
				return 0;
			}
			*socket_fd[i][curr] = sock;
			printf("socket_df[0][1]: %d; sock: %d\n", *socket_fd[i][curr], sock);
			printf("Clientid: [%s] Subscription success1, len: %d, location: [%d,%d]\n", data+1, strlen(data)-1, i, curr);
			goto send_exit;
		}
	}
	curr = ps_faa(&free_topic_head, 1);
	if (curr < TOPIC_NUM) {
		int temp = ps_faa(&free_head[curr], 1);
		*socket_fd[curr][temp] = sock;
		printf("socket_df[0][0]: %d; sock: %d\n", *socket_fd[curr][temp], sock);
		memcpy(topic_name[curr], data, TOPIC_LEN);
		printf("Clientid: [%s] Subscription success2, len: %d, location: [%d,%d]\n", topic_name[curr], strlen(data)-1, curr, temp);
		goto send_exit;
	} else {
		printf("ERROR2\n");
		Packet_error(sock, send_buf, Packet_RC_errmax);
		//exit(1);
		return 0;
	}

send_exit:
	tmp = send_buf;
	writeChar(&tmp, Packet_RC_suback);
	int ret = send(sock, send_buf, strlen(send_buf), 0);
	printf("%d bytes message sent\n", ret);

	return 0;
}

int Packet_unsub(char* data, int len, int sock, char* send_buf)
{	
	int i = 0, j = 0;
	char* tmp;
	
	printf("this is Packet_unsub\n");

	for (i = 0; i < TOPIC_NUM; i++) {
		if (memcmp(data, topic_name[i], TOPIC_LEN) == 0) {
			for (j = 0; j < COUNT; j++) {
				if (*socket_fd[i][j] == sock) {
					*socket_fd[i][j] = -1;
					printf("Clientid: [%s] Unsubscription success2, len: %d, location: [%d,%d], value: %d\n", topic_name[i], strlen(data)-1, i, j, *socket_fd[i][j]);

					tmp = send_buf;
					writeChar(&tmp, Packet_RC_unsuback);
					int ret = send(sock, send_buf, strlen(send_buf), 0);
					printf("%d bytes message sent\n", ret);
					return 0;
				}
			}
		}
	}
	Packet_error(sock, send_buf, Packet_RC_errno);
	return 0;
}

int Packet_unsuback(char* data, int len, int sock, char* send_buf)
{	
	printf("this is Packet_unsuback\n");
	return 0;
}

int Packet_suback(char* data, int len, int sock, char* send_buf)
{	
	printf("this is Packet_RC_suback\n");
	return 0;
}

void Packet_pub_send(char* data, int len, int num, char* send_buf)
{
	int i = 0, j = 0;
	int sock;
	char* tmp;

	for (i = 0; i < COUNT; i++) {
		sock = *socket_fd[num][i];
		for(j = 0; j < COUNT; j++) {
			printf("%d, ", *socket_fd[num][j]);
		}
		printf("\n");
		if (sock != -1) {
			tmp = send_buf;
			writeChar(&tmp, Packet_RC_pub);
			memcpy(send_buf+1, data, len);
			if (send(sock, send_buf, len+1, 0)<0)
				printf("send error1\n");
			//if (recv(sock, send_buf, 1024, 0)<0)
			//	printf("recv error1\n");
			//printf("puback recved");
			//tmp = send_buf;
			//operation = readChar(&tmp);
			//if ((int)operation == Packet_RC_puback)
				printf("publish success!\n");
		} else {
			printf("\t[%d]\n", *socket_fd[num][1]);
		}
	}
}

int Packet_pub(char* data, int len, int sock, char* send_buf)
{
	int i = 0;
	int j = 0;

	printf("this is Packet_pub\n");
	for (i = 0; i < TOPIC_NUM; i++) {
		//printf("data: %s, topic_name: %s\n", data, topic_name[i]);
		if (memcmp(data, topic_name[i], TOPIC_LEN) == 0) {
			printf("compare success: %d\n", i);
			for (j = 0; j < COUNT; j++) {
				if (*socket_fd[i][j] == sock) {
					printf("topicnum: %d\n", i);
					Packet_pub_send(data+TOPIC_LEN, len-TOPIC_LEN, i, send_buf);
					return 0;
				}
			}
		}
	}
	Packet_error(sock, send_buf, Packet_RC_errno);
	printf("ERROR3\n");
	//exit(1);
	return 0;
}

int Packet_puback(char* data, int len, int sock, char* send_buf)
{
	printf("this is Packet_puback\n");
	return 0;
}

static pf proc_packet[] = {
	NULL,
	Packet_sub,
	Packet_suback,
	Packet_pub,
	Packet_puback,
	Packet_unsub,
	Packet_unsuback
};

void pthread_function(void* clientfd){
    char buf[1024];
    long recvbytes;
    int client_fd=*(int*)clientfd;
	int header;
	char* curtdata;
	char* send_buf;

    /* process pakcets */
    //recvbytes=recv(client_fd, client_id, 20, 0);
    //client_id[recvbytes]=':';
    //client_id[recvbytes+1]='\0';
    //send(client_fd,"welcome to this chatroom,enjoy chatting:",100,0);

	send_buf = (char *)malloc(1024);
    while(1){
        if((recvbytes=recv(client_fd,buf,1024,0))==-1){
            perror("recv");
            pthread_exit(NULL);
        }
        
		curtdata = buf;
		header = readChar(&curtdata);
		printf("\n\tbefore calling proc_packet: %d\n", (int)header);
		if (proc_packet[header](curtdata, recvbytes-1, client_fd, send_buf) != 0) {
			printf("Error\n");
			exit(1);
		}
		printf("thread: %x, clientfd: %d, socket_fd[0][0]: %d, socket_fd[0][1]: %d\n", (unsigned int)pthread_self(), client_fd, *socket_fd[0][0], *socket_fd[0][1]);
    }
}

void channel_init(void)
{
	int i, j;

	for (i = 0; i < TOPIC_NUM; i++) {
		for (j = 0; j < COUNT; j++) {
			socket_fd[i][j] = (int *)malloc(sizeof(int));
			*socket_fd[i][j] = -1;
		}
	}

	for (i = 0; i < TOPIC_NUM; i++) {
		topic_name[i] = (char *)malloc(TOPIC_LEN);
		bzero((topic_name[i]), TOPIC_LEN);
	}

	for (i = 0; i < TOPIC_NUM; i++) {
		free_head[i] = 0;
	}
	free_topic_head = 0;
}

int main(void)
{
    pthread_t id;
    int sockfd,client_fd;
    socklen_t sin_size;
    struct sockaddr_in my_addr;
    struct sockaddr_in remote_addr;


    if((sockfd=socket(AF_INET,SOCK_STREAM,0))==-1){
        perror("socket");
        exit(1);
    }

    my_addr.sin_family=AF_INET;
    my_addr.sin_port=htons(11211);
    my_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    bzero(&(my_addr.sin_zero),8);


    if(bind(sockfd,(struct sockaddr*)&my_addr,sizeof(struct sockaddr))==-1){
        perror("bind");
        exit(1);
    }

	channel_init();
	printf("Initializing Server...\n");

    if(listen(sockfd,10)==-1){
        perror("listen");
        exit(1);
    }

	printf("Server Initialization Success.\n");
	printf("Server listening on port 11211...\n");

    while(1){
        sin_size=sizeof(struct sockaddr_in);
        if((client_fd=accept(sockfd,(struct sockaddr*)&remote_addr,&sin_size))==-1){
            perror("accept");
            exit(1);
        }
        pthread_create(&id,NULL,(void*)pthread_function,(void*)&client_fd);
        /*if(empty>0){    
            while(socket_fd[i]==-1)
                i=(i+1)%COUNT;
            socket_fd[i]=client_fd;
        else break;*/
    }
    return 0;    
}
