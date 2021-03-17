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

const char PLAYER1 = 'X';
const char PLAYER2 = 'O';

char buffer[BUFFSIZE];
char board[SIZE][SIZE];
int serversock, clientsock1,clientsock2,clientsock3;


void err_sys(char *mess) {perror(mess);exit(1);}

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
int isFinish(char C){                         
    int total=0;
    board[(buffer[0]-'0')-1][(buffer[1]-'0')-1]=C;                      /* Update board */
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
void controler(int sock1, int sock2){
    /*board*/
    boardInit();
    int row, colum, finish=0;;
    char p[2];
    p[0]=1+'0';
    write(sock1,p, 2);
    p[0]=0+'0';
    write(sock2,p, 2);
    while(1){                                                                                  /* LOOP */
        while(1){                                                              /* ///////////////CLIENT1 TURN\\\\\\\\\\\\\\\\\\ */ 
            read(sock1, &buffer[0], 2);                                        /* (Receive) the position entered by the client */
            if(board[(buffer[0]-'0')-1][(buffer[1]-'0')-1]=='-'){             /* Validate client coordinates  */
                p[0]=0+'0';
                write(sock1,p, 2);                                             /*(Sent) Report that the given value is correct */
                break;
            }
            p[0]=1+'0';
            write(sock1,p, 2);                                                 /*(Sent) report that the given value is not correct */
        }
        printf("player 1: %s\n", buffer);        
        finish=isFinish(PLAYER1);
        if(finish!=0){                                                        /* Check if the game is over */             
            p[0]=0+'0';
            p[1]=finish+'0';
            write(sock1,p, 2);                                                /* (Sent) to client 1 report that it is over */ 
            p[1]+=1; 
            write(sock2,p, 2);                                                /* (Sent) to client 2 report that it is over  */ 
            write(sock2,buffer, 2);                                           /* (Sent) to client 2 the last coordinates of client 1 */ 
            break;  
        }
        write(sock2,buffer, 2);
        while(1){                                                              /* ///////////////CLIENT2 TURN\\\\\\\\\\\\\\\\\\ */
            read(sock2, &buffer[0], 2);                                        /* (Receive) the position entered by the client */
            if(board[(buffer[0]-'0')-1][(buffer[1]-'0')-1]=='-'){             /* Validate client coordinates  */
                p[0]=0+'0';
                write(sock2,p, 2);                                             /*(Sent) Report that the given value is correct */
                break;
            }
            p[0]=1+'0';
            write(sock2,p, 2);                                                 /*(Sent) report that the given value is not correct */
        }
        printf("player 2: %s\n", buffer);        
        finish=isFinish(PLAYER2);
        if(finish!=0){                                                        /* Check if the game is over */             
            p[0]=0+'0';
            p[1]=finish+'0';
            write(sock2,p, 2);                                                /* (Sent) to client 2 report that it is over */
            p[1]+=1;  
            write(sock1,p, 2);                                                /* (Sent) to client 1 report that it is over and */ 
            write(sock1,buffer, 2);                                           /* (Sent) to client 1 the last coordinates of client 2 */ 
            break;
        }
        write(sock1,buffer, 2);
    }                                                                          /* END LOOP */
    close(sock1);                                                              /* Close socket1 and socket2 but wait for new connections */
    close(sock2);                                                            
}
void *clientThread1(void *vargp){
    /*int *s = (int *)vargp;
    int state = *s;*/
    controler(clientsock1,clientsock2);
    return((void*)NULL);
}
void *clientThread2(void *vargp){
    /*int *s = (int *)vargp;
    int state = *s;*/
    char p[2];
    p[0]=3+'0';
    write(clientsock3,p, 2); 
    close(clientsock3);
    return((void*)NULL);
}
int main(int argc, char *argv[]){
    struct sockaddr_in echoserver, echoclient;
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
    echoserver.sin_addr.s_addr = htonl(INADDR_ANY);   /* ANY address */
    echoserver.sin_port = htons(atoi(argv[1]));          /* server port */
    /*Bind socket*/
    if (bind(serversock, (struct sockaddr *) &echoserver, sizeof(echoserver)) < 0){
      err_sys("Error bind");
    }
    /*Listen socket*/
    if(listen(serversock, MAXPENDING)<0)
        err_sys("Error listen");
    /*loop*/
    int state;
    while(1){
        unsigned int clientlen = sizeof(echoclient);
        /*wait for a connection from a client*/
        clientsock1 = accept(serversock, (struct sockaddr *)&echoclient, &clientlen);
        if(clientsock1 < 0){
            err_sys("Error accept");
        }
        fprintf(stdout, "Client: %s\n\n", inet_ntoa(echoclient.sin_addr));

        clientsock2 = accept(serversock, (struct sockaddr *)&echoclient, &clientlen);
        if(clientsock2 < 0){
            err_sys("Error accept");
        }
        fprintf(stdout, "Client: %s\n\n", inet_ntoa(echoclient.sin_addr));
        pthread_create(&handleThreadId[1], NULL, clientThread1, (void *)&state);
        /*
        clientsock3 = accept(serversock, (struct sockaddr *)&echoclient, &clientlen);
        if(clientsock1 < 0){
            err_sys("Error accept");
            }
            pthread_create(&handleThreadId[2], NULL, clientThread2, (void *)&state);*/
    }
}