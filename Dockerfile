# ベースイメージとして Ubuntu 24.04 を使用
FROM ubuntu:24.04

# 環境変数を設定
ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=Asia/Tokyo

# 必要なパッケージをインストール
RUN apt update && \
    apt install -y build-essential curl tzdata && \
    ln -fs /usr/share/zoneinfo/Asia/Tokyo /etc/localtime && \
    dpkg-reconfigure -f noninteractive tzdata && \
    curl -fsSL https://deb.nodesource.com/setup_lts.x | bash - && \
    apt update && \
    apt install -y nodejs && \
    node -v && npm -v

# 作業ディレクトリを設定
WORKDIR /root/OnlineOthello

# 必要に応じてホストからファイルをコピー
# COPY . /root/OnlineOthello