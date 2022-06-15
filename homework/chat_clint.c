#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define BUF_SIZE 100
#define NAME_SIZE 20

void * send_msg(void * arg);
void * recv_msg(void * arg);
void error_handling(char * msg);

char name[NAME_SIZE]="[DEFAULT]";
char msg[BUF_SIZE];

int main(int argc, char *argv[]){
        int sock;
        struct sockaddr_in serv_addr;
        pthread_t snd_thread, rcv_thread;  //메시지를 보내는 역할을 하는 쓰레드와 메시지를 받는 역할을 하는 쓰레드 변수 선언

        void * thread_return;
        if(argc!=4){
                printf("Usage : %s <IP> <port> <name>\n",argv[0]);
                exit(1);
        }

        sprintf(name, "[%s]", argv[3]);
        sock=socket(PF_INET, SOCK_STREAM, 0);

        memset(&serv_addr,0,sizeof(serv_addr));
        serv_addr.sin_family=AF_INET;
        serv_addr.sin_addr.s_addr=inet_addr(argv[1]);
        serv_addr.sin_port=htons(atoi(argv[2]));

        if(connect(sock, (struct sockaddr*)&serv_addr,sizeof(serv_addr))==-1)
                error_handling("connect() error");

        pthread_create(&snd_thread,NULL,send_msg,(void*)&sock);  //메시지를 보내는 역할을 하는 쓰레드 생성 - 아래에 선언한 send_msg 함수를 동작하는 쓰레드 생성
        pthread_create(&rcv_thread,NULL,recv_msg,(void*)&sock);  //메시지를 받는 역할을 하는 쓰레드 생성 - 아래에 선언한 recv_msg 함수를 동작하는 쓰레드

        //쓰레드가 종료될 때까지 대기 상태에 돌입(&thread_return - 쓰레드가 종료 시 반환하는 값에 접근할 수 있는 포인터)
        pthread_join(snd_thread, &thread_return);
        pthread_join(rcv_thread, &thread_return);
        close(sock);
        return 0;
}

void * send_msg(void * arg){
        int sock=*((int*)arg);
        char name_msg[NAME_SIZE+BUF_SIZE];  //입력받은 메시지를 저장할 변수
        while(1){
                fgets(msg,BUF_SIZE,stdin);  //입력받은 메시지 저장
                if(!strcmp(msg,"q\n")||!strcmp(msg,"Q\n")){ //입력받은 값이 대소문자 q인지를 확인하는 조건문 
                        close(sock);  //입력받은 값이 대소문자 q인 경우 소켓 종료
                        exit(0);
                }
                sprintf(name_msg,"%s %s",name,msg);  //이름과 메시지를 출력하고 이 값을 name_msg 변수에 저장
                write(sock,name_msg,strlen(name_msg));  //name_msg에 값을 소켓을 통해 전달
        }
        return NULL;
}

void * recv_msg(void * arg){
        int sock=*((int*)arg);
        char name_msg[NAME_SIZE+BUF_SIZE];
        int str_len;  //소켓을 통해 전달받은 것이 있는지에 대한 여부를 저장할 변수
        while(1){
                str_len=read(sock,name_msg,NAME_SIZE+BUF_SIZE-1); //소켓을 통해 전달받았는지 여부를 저장
                if(str_len==-1){  //전달받은 것이 없다면 동작하는 조건문
                        return (void*)-1; 
                }
                name_msg[str_len]=0;
                fputs(name_msg,stdout);  //전달받은 메시지를 출력
        }
        return NULL;
}

void error_handling(char *msg){
        fputs(msg,stderr);
        fputc('\n',stderr);
        exit(1);
}
