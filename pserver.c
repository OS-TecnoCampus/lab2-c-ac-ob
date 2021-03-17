#include <stdio.h>
#include <sys/socket.h> 
#include <arpa/inet.h>
#include <stdlib.h> 
#include <string.h>
#include <unistd.h> 
#include <netinet/in.h> 
#include <stdbool.h>
#include <pthread.h>
#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#include <semaphore.h>

#define MAXPENDING 5
#define SIZE 5
#define BUFFSIZE 2

sem_t* psem1;
sem_t* psem2;
const char PLAYER1 = 'X';
const char PLAYER2 = 'O';
char board[SIZE][SIZE];
char buffer[BUFFSIZE];
int nPlayer=0,finish=0;
int game=0;

void err_sys(char *mess) {perror(mess);exit(1);}

void openSem(){    
    psem1 = (sem_t*)sem_open("/sem1", O_CREAT,0644,0);
    if (psem1 == SEM_FAILED) {
    err_sys("Open psem1");
    }
    psem2 = (sem_t*)sem_open("/sem2", O_CREAT,0644,0);
    if (psem2 == SEM_FAILED) {
    err_sys("Open psem2");
    }
}
void clearSem(){
    int sem_value;
    sem_getvalue(psem1, &sem_value);
    while (sem_value > 0) {
    sem_wait(psem1);
    sem_value--;
    }
    sem_getvalue(psem2, &sem_value);
    while (sem_value > 0) {
    sem_wait(psem2);
    sem_value--;
    }
}
void boardInit(){
    /*L'omplim de buit*/
    for (int x=0;x<SIZE;x++){
        for (int y=0;y<SIZE;y++){
            board[x][y]='-';
        }
    }
}
void print(){
    for (int x=0;x<SIZE;x++){
         printf("\n");
        for (int y=0;y<SIZE;y++){
            printf("%c ",board[x][y]);
        }
    }
    printf("\n\n");
}
int isFinish(const char C){                         
    int total=0;
    for (int x=0;x<SIZE;x++){
        for (int y=0;y<SIZE;y++){
            if(board[x][y]!='-'){
                /* Check colums */
                if(x<SIZE-2){
                    if(board[x][y]==C &&board[x+1][y]==C &&board[x+2][y]==C) return 1;
                }/* Check rows */
                if(y<SIZE-2){
                    if(board[x][y]==C &&board[x][y+1]==C &&board[x][y+2]==C) return 1;
                }/* Check left diagonal */
                if(y>2&&x<SIZE-2){
                    if(board[x][y]==C &&board[x+1][y-1]==C &&board[x+2][y-2]==C) return 1;
                }/* Check right diagonal */
                if(y<SIZE-2&&x<SIZE-2){
                    if(board[x][y]==C &&board[x+1][y+1]==C &&board[x+2][y+2]==C) return 1;
                }
                total++;
            }
        }
    }
    return (total==25)? 2:0;           /* Won player1(X) and return 1 - Draw return 2 - Not finish return 0 */ 
}
bool clientCommunication(int *sock,const char PLAYER){

    char p[2];
    if(finish!=0){
        p[0]=0+'0';
        p[1]=finish+1+'0';
        write(*sock,p,2);
        return true;
    } 
    write(*sock,buffer, 2);                                                                              
    while(1){                                                              /* ///////////////CLIENT1 TURN\\\\\\\\\\\\\\\\\\ */
        read(*sock, &buffer[0], 2);                                        /* (Receive) the position entered by the client */

        if(board[(buffer[0]-'0')-1][(buffer[1]-'0')-1]=='-'){             /* Validate client coordinates  */
            p[0]=0+'0';
            write(*sock,p, 2);                                             /*(Sent) Report that the given value is correct */
            break;
        }
        p[0]=1+'0';
        write(*sock,p, 2);                                                 /*(Sent) report that the given value is not correct */
    } 
    board[(buffer[0]-'0')-1][(buffer[1]-'0')-1]=PLAYER;                      /* Update board */    
    print();   
        
    finish=isFinish(PLAYER);
    if(finish!=0){                                                        /* Check if the game is over */             
        p[0]=0+'0';
        p[1]=finish+'0';
        write(*sock,p,2);
        sem_post(psem2);
        return true;  
        }
    return false;
}
void *clientThread_1(void *vargp){
    int *sock = (int *)vargp;
    buffer[0]=1+'0';
    while(1){                                                                                      /* LOOP */ 
        sem_wait(psem1);
        if(clientCommunication(sock,PLAYER1))break;
        sem_post(psem2);
    }
    close(*sock);
    nPlayer--;
    return((void*)NULL);
}
void *clientThread_2(void *vargp){
    sem_post(psem1);
    int *sock = (int *)vargp;
    char p[2];
    p[0]=0+'0';
    write(*sock,p, 2);
    while(1){                                                                                         /* LOOP */  
        sem_wait(psem2);
       if(clientCommunication(sock,PLAYER2))break;
        sem_post(psem1);
    }
    close(*sock);
    nPlayer--;
    return((void*)NULL);
}

int main(int argc, char *argv[]){
    struct sockaddr_in echoserver, echoclient;
    int serversock, clientsock[4];
    int result;
    pthread_t handleThreadId[2];
    /*Check input arguments*/
    if (argc != 2) {
       fprintf(stderr,"Usage: %s <port>\n", argv[0]);
       exit(1);
    }
    /*Create TCP socket*/
    serversock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
     if (serversock < 0)
        err_sys("ERROR socket");
    /*Set information for sockaddr_in structure */
    memset(&echoserver, 0, sizeof(echoserver));          /* reset memory */
    echoserver.sin_family = AF_INET;                     /* Internet/IP */
    echoserver.sin_addr.s_addr = htonl(INADDR_ANY);      /* ANY address */
    echoserver.sin_port = htons(atoi(argv[1]));          /* server port */
    /*Bind socket*/
    if (bind(serversock, (struct sockaddr *) &echoserver, sizeof(echoserver)) < 0){
      err_sys("Error bind");
    }
    /*Listen socket*/
    if(listen(serversock, MAXPENDING)<0)
        err_sys("Error listen");
    
    openSem();
    /*loop*/
    while(1){
        unsigned int clientlen = sizeof(echoclient);
        clientsock[nPlayer]= accept(serversock, (struct sockaddr *)&echoclient, &clientlen);          /*wait for a connection from a client*/
        if(clientsock < 0){
            err_sys("Error accept");
        }
        if (nPlayer==0){
            clearSem();
            boardInit();
            finish=0;  
            buffer[0]='\0';buffer[1]='\0';
            pthread_create(&handleThreadId[nPlayer], NULL, clientThread_1, (void *)&clientsock[nPlayer]);
            nPlayer++;
        }
        else if (nPlayer==1){pthread_create(&handleThreadId[nPlayer], NULL, clientThread_2, (void *)&clientsock[nPlayer]);
            nPlayer++;
            game++;} 
        else{char pa[2]={'3','0'}; write(clientsock[nPlayer],pa,2);}   
    }
}