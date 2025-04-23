#include <stdlib.h>

int main() {
    // Next.js サーバーを起動（バックグラウンドで）
    system("cd ./client/src/othello-front && npm run dev &");

    // or productionなら:
    // system("cd ./frontend && npm run build && npm start &");

    return 0;
}