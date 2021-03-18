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
    print();

    boardInit();
    read(sock, &buffer[0],BUFFSIZE);
    printf("Primer valor del buffer: %s\n", buffer);
    int turn = buffer[0]-'0';
    if (turn>1){
         printf("\nThe server is full, try later!\n");
         close(sock);
         exit(0);
    }
    while(1){
        if(turn==1){
            printf("It's your turn\n");
            while(1){
                printf("Enter row(1-5) and colum(1-5) separated by a space\n");
                scanf(" %d %d", &row, &colum);
                if (row<6&&row>0&&colum<6&&colum>0){
                    buffer[0]=row+'0';
                    buffer[1]=colum+'0';
                    write(sock,buffer, 2);
                    read(sock, &buffer[0],BUFFSIZE);
                    if((buffer[0]-'0')==0)
                        break;
                }
            }
            board[row-1][colum-1]='O';
	        print();
            turn=0;
        }
        else {
            read(sock, &buffer[0],BUFFSIZE);
            if((buffer[0]-'0')== 0){
                int value = (buffer[1]-'0');
                read(sock, &buffer[0],BUFFSIZE);
                board[(buffer[0]-'0')-1][(buffer[1]-'0')-1]='X';
                if (value== 2){print();printf("Game over! you lost :(\n");}
                else if (value== 1){ printf("Game over! ★★★★ you won ★★★★\n");}
                else {printf("Game over! Draw (ʘ_ʘ)");}
                break;
            }
            printf("player 2: %s\n", buffer);
            board[(buffer[0]-'0')-1][(buffer[1]-'0')-1]='X';
            print();
            turn=1;
        }
    }
    /* Close socket */
    close(sock);
    exit(0);
}

