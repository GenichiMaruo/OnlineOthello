#include <stdio.h>
#define BLACK '*'
#define WHITE 'o'
#define NO_STONE ' '

#define OTHELLO_ROW 8
#define OTHELLO_COL 8



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

int main(int argc, char** argv){
    
    char board[OTHELLO_ROW][OTHELLO_COL];
    initialize_board(board);
    int sock;

    char turn=BLACK;
    int row=0,col=0;
    while(1){
        print_board(board);
        printf("Turn \"%c\"\n",turn);
        printf("input row(0-7):");
        scanf("%d",&row);
        printf("input col(0-7):");
        scanf("%d",&col);
        printf("(row, col)=(%d,%d)\n",row,col);
        if(check_position(turn,row,col,board)==0){
            printf("you cannot put your stone at (%d, %d).\n",row,col);
            continue;
        }
        flip(turn,row,col,board);
        turn=(turn==BLACK)?WHITE:BLACK;
    }
}