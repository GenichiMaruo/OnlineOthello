# コンテナ名
$CONTAINER_NAME = "networkA2"

# イメージ名
$IMAGE_NAME = "ubuntu_net:24.04"

# ホスト側のマウントディレクトリ（スクリプトのあるフォルダを自動取得）
$HOST_DIR = $PSScriptRoot

# Dockerfile を使用してカスタムイメージをビルド
Write-Host "Checking for custom image: $IMAGE_NAME"
docker image inspect $IMAGE_NAME > $null 2>&1
if (-not $?) {
    Write-Host "Building custom image: $IMAGE_NAME"
    docker build -t $IMAGE_NAME $HOST_DIR
    if (-not $?) { throw "Failed to build image $IMAGE_NAME" }
}
else {
    Write-Host "Custom image $IMAGE_NAME already exists."
}

# 既存の同名コンテナがある場合、削除
Write-Host "Checking for existing container: $CONTAINER_NAME"
$existingContainer = docker ps -aq -f name="^/$($CONTAINER_NAME)$"
if ($existingContainer) {
    Write-Host "Removing existing container: $CONTAINER_NAME"
    docker rm -f $CONTAINER_NAME
}

# コンテナ起動
Write-Host "Starting container: $CONTAINER_NAME"
docker run -itd --name $CONTAINER_NAME `
    -p 10000:10000 `
    -p 3000:3000 `
    -p 8080:8080 `
    -v "${HOST_DIR}:/root/OnlineOthello" `
    $IMAGE_NAME /bin/bash
if (-not $?) { throw "Failed to start container $CONTAINER_NAME" }

# 必要な処理を一括で実行
Write-Host "Running initial setup in $CONTAINER_NAME..."
$runCommands = @"
cd /root/OnlineOthello/client/src/othello-front && \
npm install -g npm@latest && \
npm i && \
cd /root/OnlineOthello/client && \
make && \
cd /root/OnlineOthello/server && \
make && \
cd /root/OnlineOthello && \
echo 'Setup complete. Entering container shell...' && \
exec /bin/bash
"@

Write-Host "Executing final setup and entering container shell for $CONTAINER_NAME..."
docker exec -it $CONTAINER_NAME bash -c $runCommands

Write-Host "PowerShell script finished execution (This message might not appear if exec bash was successful)."