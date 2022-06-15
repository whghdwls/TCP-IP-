#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define BUF_SIZE 100
#define MAX_CLNT 256

void * handle_clnt(void * arg);
void send_msg(char * msg, int len);
void error_handling(char * msg);

int clnt_cnt = 0;
int clnt_socks[MAX_CLNT];
pthread_mutex_t mutex; //뮤텍스 선언

int main(int argc, char *argv[]){
        int serv_sock, clnt_sock;
        struct sockaddr_in serv_adr,clnt_adr;
        int clnt_adr_sz;
        pthread_t t_id;  //쓰레드의 ID를 저장할 변수 선언  

        if(argc!=2){  
                printf("Usage : %s <port> \n",argv[0]);
                exit(1);
        }

        pthread_mutex_init(&mutex, NULL);  //뮤텍스 생성
        serv_sock=socket(PF_INET, SOCK_STREAM, 0);  

        memset(&serv_adr,0,sizeof(serv_adr));
        serv_adr.sin_family=AF_INET;
        serv_adr.sin_addr.s_addr=htonl(INADDR_ANY);
        serv_adr.sin_port=htons(atoi(argv[1]));
        
        if(bind(serv_sock, (struct sockaddr*) &serv_adr, sizeof(serv_adr))==-1){
                error_handling("bind() error");
        }
        if(listen(serv_sock,5)==-1){
                error_handling("listen() error");
        }

        while(1){
                clnt_adr_sz=sizeof(clnt_adr);
                clnt_sock=accept(serv_sock, (struct sockaddr*)&clnt_adr,&clnt_adr_sz);

                //뮤텍스 기반 동기화의 기본 구성
                //임계 영역이란 두 개 이상의 쓰레드에 의해서 동시에 실행되면 안되는 영역 - 두 개 이상의 쓰레드가 동 실행하면 문제 발생
                pthread_mutex_lock(&mutex); //뮤텍스 획득 - 임계 영역의 시작
                clnt_socks[clnt_cnt++]=clnt_sock;  //클라이언트 소켓을 배열에 저장
                pthread_mutex_unlock(&mutex);  //뮤텍스 반환 - 임계 영역의 끝

                pthread_create(&t_id,NULL,handle_clnt,(void*)&clnt_sock);  //thread(쓰레드) 생성 
                pthread_detach(t_id); //쓰레드 분리 - 쓰레드 종료 시 자원 반환
                printf("Connected client IP: %s \n",inet_ntoa(clnt_adr.sin_addr));
        }

        close(serv_sock);
        return 0;
}

void * handle_clnt(void * arg){  //클라이언트를 관리하는 함수
        int clnt_sock=*((int*)arg);
        int str_len=0,i;
        char msg[BUF_SIZE];
        while((str_len=read(clnt_sock,msg,sizeof(msg)))!=0) //메시지 길이가 0이 아닌 경우 반복
                send_msg(msg,str_len); //메시지 전송

        pthread_mutex_lock(&mutex);  //뮤텍스 획득
        for(i=0; i<clnt_cnt; i++){  //클라이언트 소켓을 저장한 배열을 순환하기 위한 반복문
                if(clnt_sock==clnt_socks[i]){ //입력받은 클라이언트 소켓이 클라이언트 소켓을 저장한 배열에 존재하는 경우
                        while(i++<clnt_cnt-1)   //클라이언트 소켓을 저장한 배열에 마지막 인덱스보다 i가 작은 동안 동작하는 반복문
                                clnt_socks[i]=clnt_socks[i+1];  //배열에 값들을 한 칸씩 앞으로 이동시켜 입력받은 클라이언트 소켓과 같은 값을 가지고 있는 인덱스의 값을 삭제(큐 삭제 연산)
                        break;
                }
        }

        clnt_cnt--;
        pthread_mutex_unlock(&mutex);  //뮤텍스 반환
        close(clnt_sock);
        return NULL;
}

void send_msg(char * msg, int len){
        int i;
        pthread_mutex_lock(&mutex);  //뮤텍스 획득
        for(i=0; i<clnt_cnt; i++){ 
                write(clnt_socks[i],msg,len);
        }
        pthread_mutex_unlock(&mutex);  //뮤텍스 반환
}

void error_handling(char *msg){
        fputs(msg,stderr);
        fputc('\n',stderr);
        exit(1);
}
