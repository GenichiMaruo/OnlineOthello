#include <stdio.h>
#include <memory.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "othello.h"

void initialize_board(char b[OTHELLO_ROW][OTHELLO_COL]){
    int i,j;
    for(i=0;i<OTHELLO_ROW;i++){
        for(j=0;j<OTHELLO_COL;j++){
            b[i][j]=' ';
        }
    }
    b[3][3]=BLACK;
    b[4][4]=BLACK;
    b[3][4]=WHITE;
    b[4][3]=WHITE;
}
void print_board(char b[OTHELLO_ROW][OTHELLO_COL]){
    int i,j;
    printf(" ");
    for(i=0;i<OTHELLO_COL;i++){
        printf(" %d",i);
    }
    printf("\n");
    printf(" ");
    for(i=0;i<OTHELLO_COL*2+1;i++){
        printf("-");
    }
    printf("\n");
    for(i=0;i<OTHELLO_ROW;i++){
        printf("%d",i);
        for(j=0;j<OTHELLO_COL;j++){
            printf("|%c",b[i][j]);
        }
        printf("|\n");
    }
    printf(" ");
    for(i=0;i<OTHELLO_COL*2+1;i++){
        printf("-");
    }
    printf("\n");
}

int count_flip_stone(char turn, int row, int col, int d_row, int d_col, char b[OTHELLO_ROW][OTHELLO_COL]){
    if(row+d_row<0 || col+d_col<0){
        return 0;
    }
    if(row+d_row>7 || col+d_col>7){
        return 0;
    }
    if(b[row+d_row][col+d_col]==turn){
        return 0;
    }
    int t_row=row, t_col=col;
    int flip=0;
    int find_mystone=0;
    while(!(t_row>7) || !(t_col>7)||!(t_row<0)||!(t_col<0)){
        t_row=t_row+d_row;
        t_col=t_col+d_col;
        if(b[t_row][t_col]==' '){
            break;
        }
        if(b[t_row][t_col]==turn){
            find_mystone=1;
            break;
        }
        flip++;
    }
    return (find_mystone==1)?flip:0;
}
void flip_stone(char turn, int row, int col, int d_row, int d_col, char b[OTHELLO_ROW][OTHELLO_COL]){
    int t_row=row, t_col=col;
    int flip=0;
    int find_mystone=0;
    while(!(t_row>7) || !(t_col>7)||!(t_row<0)||!(t_col<0)){
        t_row=t_row+d_row;
        t_col=t_col+d_col;
        if(b[t_row][t_col]==turn){
            break;
        }
        b[t_row][t_col]=turn;
    }
}
int flip(char turn, int row, int col, char b[OTHELLO_ROW][OTHELLO_COL]){
    b[row][col]=turn;
    int count=0;
    int d_row=-1, d_col=-1;
    for(d_row=-1;d_row<=1;d_row++){
        for(d_col=-1;d_col<=1;d_col++){
            count=count_flip_stone(turn,row,col,d_row,d_col,b);
            if(count>0){
                flip_stone(turn,row,col,d_row,d_col,b);
            }
        }
    }
    return count;
}
int check_position(char turn, int row, int col, char b[OTHELLO_ROW][OTHELLO_COL]){
    if(b[row][col]!=NO_STONE){
        return 0;
    }
    /*
    [-1,-1]左上　[-1, 0]上
    [-1, 1]右上　[ 0, 1]右
    [ 1, 1]右下　[ 1, 0]下
    [ 1,-1]左下　[ 0,-1]左
    */
    int count=0;
    int d_row=-1, d_col=-1;
    for(d_row=-1;d_row<=1;d_row++){
        for(d_col=-1;d_col<=1;d_col++){
            count+=count_flip_stone(turn,row,col,d_row,d_col,b);
        }
    }
    return count;
}
int make_connection(char* addr, int *sock){
    struct in_addr dest;
    inet_aton(addr, &dest);
    struct sockaddr_in srv_addr;

    // 接続先アドレスを設定する
    memcpy(&(srv_addr.sin_addr.s_addr),(char*)&(dest.s_addr),4);
    srv_addr.sin_port=htons(PORT);
    srv_addr.sin_family=AF_INET;

    *sock = socket(AF_INET, SOCK_STREAM, 0);
    
    
    if (*sock < 0) {
        perror("ERROR opening socket");
        exit(1);
    }

    // サーバーのホスト名を取得する
    /*
    struct hostent *server;
    server = gethostname(addr);
    if (server == NULL) {
        fprintf(stderr, "ERROR, no such host\n");
        exit(0);
    }
    */

    // サーバーに接続する
    if (connect(*sock, (struct sockaddr *)&srv_addr, sizeof(srv_addr)) < 0) {
        perror("ERROR connecting");
        exit(1);
    }
}

char initialize_game(int sock, char *user_name){
    int n=0;
    msg m;
    memset(&m,0,MESSAGE_LENGTH);
    memcpy(m.name,user_name,strlen(user_name));
    m.type=CONN_REQ;
    /*
    msg.type: CONN_REQ
    msg.name: my user name
    msg.color: N/A
    msg.row: N/A
    msg.col: N/A
    */
    n=send(sock, &m, MESSAGE_LENGTH, 0);
    if (n < 0) {
        perror("ERROR sending from socket");
        exit(1);
    }
    memset(&m,0,MESSAGE_LENGTH);
    n=recv(sock, &m, MESSAGE_LENGTH, 0);
    if (n < 0) {
        perror("ERROR reading from socket");
        exit(1);
    }
    if(m.type!=CONN_RES){
        printf("initialize process is not done.\n");
        exit(1);
    }
    /*
    msg.type: CONN_RES
    msg.name: a name of opponent
    msg.color: my color
    msg.row: N/A
    msg.col: N/A
    */
    printf("%s vs %s\n",user_name,m.name);
    printf("My color is %c\n",(m.color));
    return m.color;
    
}
int main(int argc, char** argv){
    const char* optstring = "s:n:";
    int c;
    char *srv_addr=NULL;
    char *user_name=NULL;

    if(argc!=5){
        printf("args:%d\n",argc);
        printf(" arg -s server_address, -n user_name\n");
        exit(0);
    }
    while ((c=getopt(argc, argv, optstring)) != -1) {
        //printf("opt=%c ", c);
        switch(c){
            case 's':{
                srv_addr=optarg;
                break;
            }
            case 'n':{
                user_name=optarg;
                break;
            }
            default:{
                printf(" arg -s server_address, -n user_name\n");
                exit(0);
            }
        }
    }
    printf("server address:%s\n",srv_addr);
    if(strlen(user_name)>20){
        printf("the length of the input user name must be less than 20 characters\n.");
        exit(0);
    }
    printf("user name:%s\n",user_name);

    int sock;
    make_connection(srv_addr,&sock);// make socket communicating with a server by using an IP address
    char mycolor=initialize_game(sock,user_name);// sending a message to connect the server and receive my color

    char board[OTHELLO_ROW][OTHELLO_COL];
    initialize_board(board);
    

    char turn=BLACK;
    int row=0,col=0,rcv_cnt;
    msg m;
    while(1){
        memset(&m,0,MESSAGE_LENGTH);
        print_board(board);
        if(turn!=mycolor){
            printf("--- waiting for my turn ---\n");
            rcv_cnt=recv(sock,&m,MESSAGE_LENGTH,0);
            /*
            msg.type: PUT_OPP_STONE
            msg.name: a name of opponent
            msg.color: opponent color
            msg.row: a row index of the opponent stone
            msg.col: a col index of the opponent stone
            */
            if(rcv_cnt<0){
                perror("ERROR reading a message including an opponet stone from socket");
                exit(1);
            }
            if(m.type!=PUT_OPP_STONE){
                printf("received message does not contain an opponent stone info\n");
                exit(1);
            }
            flip(turn,m.row,m.col,board);
            print_board(board);
            turn=mycolor;
        }
        printf("My turn \"%c\"\n",mycolor);
        printf("input row(0-7):");
        scanf("%d",&row);
        printf("input col(0-7):");
        scanf("%d",&col);
        printf("(row, col)=(%d,%d)\n",row,col);
        
        if(check_position(turn,row,col,board)==0){
            printf("you cannot put your stone at (%d, %d).\n",row,col);
            continue;
        }
        memset(&m,0,MESSAGE_LENGTH);
        m.type=PUT_MY_STONE;
        memcpy(m.name,user_name,NAME_LENGTH);
        m.color=mycolor;
        m.row=row;
        m.col=col;
        send(sock,&m,MESSAGE_LENGTH,0);
        flip(turn,row,col,board);

        turn=(turn==BLACK)?WHITE:BLACK;
    }
}