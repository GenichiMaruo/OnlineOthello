# コンテナ名
$CONTAINER_NAME = "networkA"

# イメージ名
$IMAGE_NAME = "ubuntu_net:24.04"
$BASE_IMAGE = "ubuntu:24.04"

# ホスト側のマウントディレクトリ（スクリプトのあるフォルダを自動取得）
# PowerShellでは $PSScriptRoot でスクリプトの親ディレクトリを取得できます
$HOST_DIR = $PSScriptRoot

# スクリプトのパスにスペースが含まれる場合を考慮して引用符で囲む
$QuotedHostDir = '"' + $HOST_DIR + '"'

# Ubuntuのbase imageが存在しない場合、pull
Write-Host "Checking for base image: $BASE_IMAGE"
docker image inspect $BASE_IMAGE > $null 2>&1
if (-not $?) {
  Write-Host "Pulling base image: $BASE_IMAGE"
  docker pull $BASE_IMAGE
} else {
  Write-Host "Base image $BASE_IMAGE already exists."
}

# カスタムイメージが存在しない場合、作成
Write-Host "Checking for custom image: $IMAGE_NAME"
docker image inspect $IMAGE_NAME > $null 2>&1
if (-not $?) {
  Write-Host "Creating custom image: $IMAGE_NAME"
  $TMP_CONTAINER = "tmp_${CONTAINER_NAME}_setup"

  # 一時コンテナをバックグラウンドで起動
  Write-Host "Starting temporary container $TMP_CONTAINER..."
  docker run -itd --name $TMP_CONTAINER $BASE_IMAGE /bin/bash
  if (-not $?) { throw "Failed to start temporary container $TMP_CONTAINER" }

  # 一時コンテナ内でセットアップコマンドを実行
  # PowerShellのヒアストリングを使用して複数行コマンドを定義
  $setupCommands = @"
apt update && \
apt install -y build-essential curl && \
curl -fsSL https://deb.nodesource.com/setup_lts.x | bash - && \
apt update && \
apt install -y nodejs && \
node -v && npm -v
"@
  Write-Host "Running setup commands in $TMP_CONTAINER..."
  # docker exec -it で実行。完了を待つ。
  docker exec -i $TMP_CONTAINER bash -c $setupCommands
  # `-i` のみでもバックグラウンドタスクとしては十分だが、-it でないと TTY 関連の警告が出る可能性がある
  # 元のスクリプトに合わせて `-it` を使う場合: docker exec -it $TMP_CONTAINER bash -c $setupCommands
  # ただし、非対話的実行なので `-i` が適切かもしれない。ここではより安全な `-i` を使用。
  if (-not $?) {
      Write-Host "Error during setup in temporary container. Cleaning up..."
      docker rm -f $TMP_CONTAINER
      throw "Failed to setup temporary container"
  }

  # イメージをコミット
  Write-Host "Committing image $IMAGE_NAME from $TMP_CONTAINER..."
  docker commit $TMP_CONTAINER $IMAGE_NAME

  # 一時コンテナを削除
  Write-Host "Removing temporary container $TMP_CONTAINER..."
  docker rm -f $TMP_CONTAINER
} else {
  Write-Host "Custom image $IMAGE_NAME already exists."
}

# 既存の同名コンテナがある場合、削除
Write-Host "Checking for existing container: $CONTAINER_NAME"
# docker ps -a -q で全コンテナIDを取得し、-f name= でフィルタリング
$existingContainer = docker ps -aq -f name="^/$($CONTAINER_NAME)$"
if ($existingContainer) {
  Write-Host "Removing existing container: $CONTAINER_NAME"
  docker rm -f $CONTAINER_NAME
}

# コンテナ起動
Write-Host "Starting container: $CONTAINER_NAME"
# PowerShellでは行継続にバッククォート ` を使用
# マウントパスを正しく処理するために $QuotedHostDir を使用
docker run -itd --name $CONTAINER_NAME `
  -p 10000:10000 `
  -v "${HOST_DIR}:/root/OnlineOthello" `
  $IMAGE_NAME /bin/bash
if (-not $?) { throw "Failed to start container $CONTAINER_NAME" }

# 必要な処理を一括で実行
Write-Host "Running initial setup in $CONTAINER_NAME..."
# コンテナ内で実行するコマンド
$runCommands = @"
cd /root/OnlineOthello/client/src/othello-front && \
npm install -g npm@latest && \
npm i && \
cd /root/OnlineOthello && \
gcc client/src/client_app.c -o client_app.out && \
gcc server/src/server_app.c -o server_app.out && \
echo 'Setup complete. Entering container shell...' && \
exec /bin/bash
"@

# docker exec -it を使用して対話的にコマンドを実行し、最後にbashに入る
# このコマンドが実行されると、PowerShellスクリプト自体はこの exec の完了を待機（または終了）します。
Write-Host "Executing final setup and entering container shell for $CONTAINER_NAME..."
docker exec -it $CONTAINER_NAME bash -c $runCommands

# 上記の docker exec -it ... exec /bin/bash が実行されると、
# 制御がコンテナ内のBashシェルに移るため、以下の行は通常実行されません。
Write-Host "PowerShell script finished execution (This message might not appear if exec bash was successful)."