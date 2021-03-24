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
#include <stdint.h>

#define MAXPENDING 5
#define SIZE 5
#define BUFFSIZE 2

sem_t* psem1;
sem_t* psem2;
const char PLAYER1 = 'X';  /* global variables */
const char PLAYER2 = 'O';
char board[SIZE][SIZE];
char buffer[BUFFSIZE];
int nPlayer=0,finish=0;    /* nPlayers = number of players// finish saves the value of the game state */

void err_sys(char *mess) {perror(mess);exit(1);}

void openSem(){                                           
    psem1 = (sem_t*)sem_open("/sem1", O_CREAT,0644,0);    /* Creating sem1 */
    if (psem1 == SEM_FAILED) {
        err_sys("Open psem1");
    }
    psem2 = (sem_t*)sem_open("/sem2", O_CREAT,0644,0);     /* Creating sem2 */
    if (psem2 == SEM_FAILED) {
        err_sys("Open psem2");
    }
}
void clearSem(){                /* ClearSem sets the values ​​to 0 of all sems */
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
void boardInit(){               /* Fill and restart the board */  
    for (int x=0;x<SIZE;x++){
        for (int y=0;y<SIZE;y++){
            board[x][y]='-';
        }
    }
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
bool clientCommunication(int sock,const char PLAYER,sem_t* psem){

    char p[2];
    if(finish!=0){                                                         /* validates that the game is not over, in case yes, the current player has lost or drawn */
        p[0]=0+'0';
        p[1]=finish+1+'0';
        write(sock,p,2);                                                   /*(Sent) the game is over and result*/
        return true;
    } 
    write(sock,buffer, 2);                                                                                                                             
    while(1){                                                             
        read(sock, &buffer[0], 2);                                        /* (Receive) the position entered by the client */

        if(board[(buffer[0]-'0')-1][(buffer[1]-'0')-1]=='-'){             /* Validate client coordinates  */
            p[0]=0+'0';
            p[1]=0+'0';
            write(sock,p, 2);                                             /*(Sent) Report that the given value is correct */
            break;
        }
        p[0]=1+'0';
        p[1]=0+'0';
        write(sock,p, 2);                                                 /*(Sent) report that the given value is not correct */
    } 
    board[(buffer[0]-'0')-1][(buffer[1]-'0')-1]=PLAYER;                      /* Update board */     
        
    finish=isFinish(PLAYER);
    if(finish!=0){                                                        /* Check if the game is over */             
        p[0]=0+'0';
        p[1]=finish+'0';
        write(sock,p,2);                                                   /* (Sent) the game is over and result*/
        sem_post(psem);                                                    /* Activate the other player's sem to validate his result */
        return true;  
        }
    return false;
}
void *clientThread_1(void *vargp){                                          /*//////THREAD 1\\\\\\*/
    int sock = (uintptr_t)vargp;
    buffer[0]=1+'0';
    buffer[1]='\0';                                                         /* resets the buffer */
    while(1){                                                               /* LOOP PLAYER 1 */                                                                                
        sem_wait(psem1);                                                    /* Wait for player2 to finish */     
        if(clientCommunication(sock,PLAYER1,psem2)) break;                  /* Player turn 1 */
        sem_post(psem2);                                                    /* Activate player2 turn */ 
    }
    close(sock);                                                            /* Close socket */
    return((void*)NULL);
}
void *clientThread_2(void *vargp){                                          /*////// THREAD 2\\\\\\*/
    sem_post(psem1);                                                      /* Sart game and activate player1 turn */
    int sock = (uintptr_t)vargp;
    char p[2];
    p[0]=0+'0';
    p[1]='\0';
    write(sock,p, 2);                                                       /*(Sent) report that you are player 2*/
    while(1){                                                                         /* LOOP PLAYER 2*/  
        sem_wait(psem2);                                                    /* Wait for player1 to finish */ 
       if(clientCommunication(sock,PLAYER2,psem1)) break;                   /* Player turn 2 */
        sem_post(psem1);                                                    /* Activate player1 turn */
    }
    close(sock);                                                            /* Close socket */
    nPlayer = nPlayer-2;                                                    /* the game is over and there are 2 fewer players */
    return((void*)NULL);
}
void *clientThread_3(void *vargp){                                          /*////// THREAD 3\\\\\\*/
    int sock = (uintptr_t)vargp;
    char pa[2]={'3','0'};                                                   /*(Sent) reports that the server is full*/
    write(sock,pa,2);
    close(sock);                                                            /* Close socket */
    return((void*)NULL);
}

int main(int argc, char *argv[]){
    struct sockaddr_in echoserver, echoclient;
    int serversock, clientsock;
    int result;
    pthread_t handleThreadId[3];
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
    
    openSem();  /* create sem1 and sem2 */
    unsigned int clientlen = sizeof(echoclient);
    while(1){                                                                               /* INFINITE SERVER LOOP */
        clientsock= accept(serversock, (struct sockaddr *)&echoclient, &clientlen);          /*wait for a connection from a client*/
        if(clientsock < 0){
            err_sys("Error accept");
        }
        if (nPlayer==0){
            clearSem();
            boardInit();            /* resets the variables */
            finish=0;  
            pthread_create(&handleThreadId[0], NULL, clientThread_1, (void *)(uintptr_t)clientsock); /* Create a new thread for player 1 */
            nPlayer++;
        }
        else if (nPlayer==1){pthread_create(&handleThreadId[1], NULL, clientThread_2, (void *)(uintptr_t)clientsock);nPlayer++;}  /* Create a new thread for player 2 */
        else{pthread_create(&handleThreadId[2], NULL, clientThread_3, (void *)(uintptr_t)clientsock);}  /* Create a new thread for player 3 */
    }
}