# OnlineOthello 環境構築手順

このプロジェクトは、フロントエンドに Node.js を使用しています。実行には Node.js 環境が必要となるため、**事前に Docker を使ったセットアップ**が必要です。

---

## ✅ 初回セットアップ

1. **Docker Desktop を起動**してください。
2. 過去に作成された以下の Docker リソースがある場合は **削除してください**:
   - コンテナ：`networkA`
   - イメージ：`ubuntu_net:24.04`

3. 本リポジトリのルートにあるシェルスクリプト、または PowerShell スクリプトを実行して環境を構築します。

### Mac / Linux の場合:
```bash
./run_networkA.sh
```
### Windows (PowerShell) の場合:
```bash
.\run_networkA.ps1
```

## 🚀 起動方法
セットアップスクリプトを実行すると、以下が自動的に行われます：

必要な Docker イメージの構築

Node.js / npm のインストール

`npm install` による依存関係のインストール

C言語アプリケーションのコンパイル

実行ファイル：
クライアントアプリ：`client_app`

サーバーアプリ：`server_app`

どちらも `/root/OnlineOthello/` に配置されています。
```bash
# コンテナ内で
./client_app
# または
./server_app
```

## 🔁 再コンパイルしたいとき
コンテナ内で次のスクリプトを実行することで、アプリの再コンパイルが可能です。

```bash
/root/OnlineOthello/build_apps.sh
```

## 📁 ディレクトリ構成（抜粋）
```bash
OnlineOthello/
├── client/
│   └── src/
│       └── othello-front/   # Next.js frontend
│       └── client_app.c     # C client source
├── server/
│   └── src/
│       └── server_app.c     # C server source
├── build_apps.sh            # 再コンパイル用スクリプト
├── run_networkA.sh          # Unix系起動スクリプト
├── run_networkA.ps1         # Windows用起動スクリプト
```

## 🛠️ 前提
Docker Desktop がインストールされていること

```run_networkA.sh``` または ```run_networkA.ps1``` が実行可能であること