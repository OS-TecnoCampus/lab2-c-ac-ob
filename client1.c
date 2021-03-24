#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <stdbool.h>

#define BUFFSIZE 2
#define SIZE 5
char board[SIZE][SIZE];

/* err_sys - wrapper for perror */
void err_sys(char *msg) {perror(msg);exit(1);}

int main(int argc, char *argv[]) {
    struct sockaddr_in echoserver;
    char buffer[BUFFSIZE];
    unsigned int echolen;
    int sock, result, row, colum;
    int received = 0;

    /* socket: create the socket */
    sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0){
        err_sys("ERROR opening socket");
    }
    
    /* Set information for sockaddr_in */
    memset(&echoserver, 0, sizeof(echoserver));          /* reset memory */
    echoserver.sin_family = AF_INET;                     /* Internet/IP */
    echoserver.sin_addr.s_addr = inet_addr(argv[1]);     /* IP address */
    echoserver.sin_port = htons(atoi(argv[2]));          /* server port */

    /* connect: create a connection with the server */
    result = connect(sock, (struct sockaddr *) &echoserver, sizeof(echoserver));
    if (result < 0){
        err_sys("ERROR connecting");
    } 

    printf("\n================================\n");
    printf("Welcome to the tic-tac-toe game!\n");
    printf("================================\n");

    read(sock, &buffer,BUFFSIZE);                                                                  /* (Receive) reports the player's turn */
    int turn = buffer[0]-'0';           
    if (turn>1){                                                                                   /* If it is a 3 it means the server is full */                                                                                  
         printf("\nThe server is full, try later!\n");
         close(sock);                                                                              /* Close socket */
         exit(0);
    }
    while(1){                                                                                      /*\\\\\\\\\\GAME LOOP/////////*/ 
        if(turn==1){                                                                               /* my turn */
            printf("\nIt's your turn\n");   
            while(1){                                                                              /* coordinate validation loop */
                printf("Enter row(1-5) and colum(1-5) separated by a space\n");
                scanf(" %d %d", &row, &colum);
                if (row<6&&row>0&&colum<6&&colum>0){
                    buffer[0]=row+'0';
                    buffer[1]=colum+'0';
                    write(sock,buffer, 2);                                                          /*(Sent) sent coordinate*/
                    read(sock, &buffer[0],BUFFSIZE);                                                /* (Receive) If it is a 0 it means the coordinate is valid */ 
                    if((buffer[0]-'0')==0)
                        break;
                }
            }
            turn=0;
        }
        else {                                                                                       /* turn player2*/
            read(sock, &buffer[0],BUFFSIZE);                                                         /* (Receive) the position entered by the player2 */
            if((buffer[0]-'0')== 0){                                                                 /* If it is a 0 it means the game is over */ 
                int value = (buffer[1]-'0');
                read(sock, &buffer[0],BUFFSIZE);
                if (value== 2){printf("\nplayer 2: %s\n", buffer);printf("Game over! you lost :(\n");}  /* you lost*/ 
                else if (value== 1){ printf("Game over! ★★★★ you won ★★★★\n");}                       /* you won*/       
                else {printf("Game over! Draw (ʘ_ʘ)");}                                               /* you draw*/ 
                break;
            }
            printf("\nplayer 2: %s\n", buffer);
            turn=1;
        }
    }
    close(sock);                                                                           /* Close socket */
    exit(0);
}