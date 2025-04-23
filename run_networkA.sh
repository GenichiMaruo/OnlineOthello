#!/bin/bash

# コンテナ名
CONTAINER_NAME="networkA"

# イメージ名
IMAGE_NAME="ubuntu_net:24.04"
BASE_IMAGE="ubuntu:24.04"

# ホスト側のマウントディレクトリ（スクリプトのあるフォルダを自動取得）
HOST_DIR="$(cd "$(dirname "$0")" && pwd)"

# Ubuntuのbase imageが存在しない場合、pull
if ! docker image inspect $BASE_IMAGE > /dev/null 2>&1; then
  echo "Pulling base image: $BASE_IMAGE"
  docker pull $BASE_IMAGE
fi

# カスタムイメージが存在しない場合、作成
if ! docker image inspect $IMAGE_NAME > /dev/null 2>&1; then
  echo "Creating custom image: $IMAGE_NAME"
  TMP_CONTAINER="tmp_${CONTAINER_NAME}_setup"

  docker run -itd --name $TMP_CONTAINER $BASE_IMAGE /bin/bash

  docker exec -it $TMP_CONTAINER bash -c "
    apt update &&
    apt install -y build-essential curl &&
    curl -fsSL https://deb.nodesource.com/setup_lts.x | bash - &&
    apt update &&
    apt install -y nodejs &&
    node -v && npm -v
  "

  docker commit $TMP_CONTAINER $IMAGE_NAME
  docker rm -f $TMP_CONTAINER
fi

# 既存の同名コンテナがある場合、削除
if [ "$(docker ps -aq -f name=^/${CONTAINER_NAME}$)" ]; then
  echo "Removing existing container: $CONTAINER_NAME"
  docker rm -f $CONTAINER_NAME
fi

# コンテナ起動
docker run -itd --name $CONTAINER_NAME \
  -p 10000:10000 \
  -v "$HOST_DIR":/root/OnlineOthello \
  $IMAGE_NAME /bin/bash

# 必要な処理を一括で実行
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
