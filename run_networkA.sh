#!/bin/bash

# コンテナ名
CONTAINER_NAME="networkA"

# イメージ名
IMAGE_NAME="ubuntu_net:24.04"

# ホスト側のマウントディレクトリ（スクリプトのあるフォルダを自動取得）
HOST_DIR="$(cd "$(dirname "$0")" && pwd)"

# Dockerfile を使用してカスタムイメージをビルド
if ! docker image inspect $IMAGE_NAME > /dev/null 2>&1; then
  echo "Building custom image: $IMAGE_NAME"
  docker build -t $IMAGE_NAME "$HOST_DIR"
  if [ $? -ne 0 ]; then
    echo "Failed to build image $IMAGE_NAME"
    exit 1
  fi
else
  echo "Custom image $IMAGE_NAME already exists."
fi

# 既存の同名コンテナがある場合、削除
if [ "$(docker ps -aq -f name=^/${CONTAINER_NAME}$)" ]; then
  echo "Removing existing container: $CONTAINER_NAME"
  docker rm -f $CONTAINER_NAME
fi

# コンテナ起動
echo "Starting container: $CONTAINER_NAME"
docker run -itd --name $CONTAINER_NAME \
  -p 10000:10000 \
  -v "$HOST_DIR":/root/OnlineOthello \
  $IMAGE_NAME /bin/bash
if [ $? -ne 0 ]; then
  echo "Failed to start container $CONTAINER_NAME"
  exit 1
fi

# 必要な処理を一括で実行
echo "Running initial setup in $CONTAINER_NAME..."
docker exec -it $CONTAINER_NAME /bin/bash -c "
  cd /root/OnlineOthello/client/src/othello-front &&
  npm install -g npm@latest &&
  npm i &&
  cd /root/OnlineOthello &&
  gcc client/src/client_app.c -o client_app.out &&
  gcc server/src/server_app.c -o server_app.out &&
  echo 'Setup complete. Entering container shell...' &&
  exec /bin/bash
"