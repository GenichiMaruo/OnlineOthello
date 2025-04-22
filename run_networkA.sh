#!/bin/bash

# コンテナ名
CONTAINER_NAME="networkA"

# ホスト側のマウントディレクトリ（スクリプトのあるフォルダを自動取得）
HOST_DIR="$(cd "$(dirname "$0")" && pwd)"

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
  -v "$HOST_DIR":/root/OnlineOthello \
  $IMAGE_NAME /bin/bash

# コンテナ内のバッシュに入る
docker exec -it $CONTAINER_NAME /bin/bash -c "cd /root/OnlineOthello && exec /bin/bash"