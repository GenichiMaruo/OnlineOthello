#!/bin/bash

# コンテナ名
CONTAINER_NAME="networkA"

# ホスト側のマウントディレクトリ（必要に応じてパスを調整）
HOST_DIR="/Users/maruogenichi/grad_school/networkA"

# Dockerイメージ名
IMAGE_NAME="ubuntu_net:24.04"

# コンテナがすでに存在する場合、削除
if [ "$(docker ps -aq -f name=$CONTAINER_NAME)" ]; then
  echo "Removing existing container: $CONTAINER_NAME"
  docker rm -f $CONTAINER_NAME
fi

# コンテナ起動
docker run -itd --name $CONTAINER_NAME \
  -p 10000:10000 \
  -v "$HOST_DIR":/root/networkA \
  $IMAGE_NAME /bin/bash
