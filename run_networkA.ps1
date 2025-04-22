# PowerShell スクリプト

# コンテナ名
$ContainerName = "networkA"

# スクリプトが存在するディレクトリを取得（PowerShell版）
$HostDir = Split-Path -Parent $MyInvocation.MyCommand.Definition

# Dockerイメージ名
$ImageName = "ubuntu_net:24.04"

# コンテナがすでに存在する場合、削除
$ExistingContainer = docker ps -aq -f "name=$ContainerName"
if ($ExistingContainer) {
    Write-Host "Removing existing container: $ContainerName"
    docker rm -f $ContainerName
}

# コンテナ起動
docker run -itd --name $ContainerName `
  -p 10000:10000 `
  -v "${HostDir}:/root/OnlineOthello" `
  $ImageName /bin/bash

# コンテナ内のbashに入る（cdしてからbash起動）
docker exec -it $ContainerName /bin/bash -c "cd /root/OnlineOthello && exec /bin/bash"
