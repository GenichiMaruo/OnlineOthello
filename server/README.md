# OnlineOthello サーバーアプリケーション

このREADMEは、OnlineOthelloのサーバーアプリケーションの構成要素について説明します。
サーバーはC言語で実装されており、クライアントとの通信やゲームロジックを担当します。
フロントエンドの詳細については、以下のリンクを参照してください。

- [OnlineOthello クライアント・フロントエンド](../client/src/othello-front/README.md)
- [OnlineOthello クライアント・バックエンド](../client/README.md)

## サーバーアプリケーション本体（server_app.c）

`server/src/server_app.c`は、Othelloオンライン対戦サーバーの**エントリーポイント**となるC言語のメインプログラムです。  
クライアントからの接続受付、部屋・クライアント管理、各種スレッドの起動など、サーバー全体の制御を担います。

### 主な機能・構成

- **サーバーソケットの初期化**
  - TCPソケットを生成し、指定ポートでバインド・リッスン
  - アドレス再利用オプション（SO_REUSEADDR）も設定

- **部屋・クライアント管理の初期化**
  - サーバー起動時に部屋情報・クライアント情報の初期化処理を実行

- **クライアント接続受付ループ**
  - `accept`で新規クライアント接続を待ち受け、接続ごとに管理情報を追加
  - クライアントごとに専用のハンドラースレッド（`handle_client`）を生成し、非同期で通信処理を担当

- **エラーハンドリング・リソース管理**
  - ソケットやメモリのエラー時は適切にリソースを解放し、サーバーの安定稼働を維持
  - サーバー満員時の接続拒否や、異常時のクリーンアップ処理も実装

- **終了処理**
  - サーバー終了時には全クライアントへの通知やスレッド終了待ち、リソース解放などを行う設計

### 備考

- クライアントごとにスレッドを分離することで、複数人同時対戦やチャットなどのリアルタイム処理に対応
- 部屋管理やクライアント管理は専用モジュール（`room_management.c`, `client_management.c`）に分離されており、保守性・拡張性が高い構成です
- サーバー本体は、Othelloのゲームロジックや通信プロトコルの中核となる役割を担っています

## クライアントメッセージ処理・スレッド管理（client_handler.c）

`server/src/client_handler.c`は、Othelloサーバーに接続した各クライアントごとの**通信処理・ゲーム進行管理**を担うモジュールです。  
クライアントごとに専用スレッドを立ち上げ、メッセージの受信・解析・応答、ゲームロジックとの連携、切断処理などを行います。

### 主な機能・構成

- **クライアントごとのスレッド処理**
  - `handle_client`関数でクライアントごとにスレッドを生成し、メッセージ受信ループを実行
  - 受信したメッセージタイプに応じて、部屋作成・参加・ゲーム開始・コマ配置・再戦・チャットなどの処理関数を呼び出し

- **ゲーム進行・部屋管理**
  - 部屋作成・参加・ゲーム開始・コマ配置・再戦リクエストなど、Othelloのゲーム進行に必要な全てのクライアント操作を処理
  - ゲームロジックや部屋状態の更新は`game_logic.c`や`room_management.c`と連携

- **同期・排他制御**
  - クライアント・部屋情報へのアクセスはミューテックスで保護し、複数スレッド間の競合を防止

- **切断処理・リソース管理**
  - クライアント切断時には部屋の状態を適切に更新し、相手プレイヤーへの通知や部屋のクリーンアップも実施

- **エラーハンドリング**
  - 不正な操作や異常系メッセージにはエラー応答を返し、サーバーの安定稼働を維持

### 備考

- クライアントからの全てのリクエストはこのモジュールで受け付け、必要に応じて他の管理モジュールやゲームロジックと連携します。
- ゲーム進行・再戦・チャット・切断など、Othelloサーバーのリアルタイムな多人数対戦を支える中核的な役割を担っています。
- スレッドごとに独立した処理を行うことで、同時接続数の増加にも対応可能な設計です。
- 各種メッセージの解析や応答処理は、`message_parser.c`や`message_sender.c`に分離されており、保守性・拡張性が高い構成です。

## クライアント管理モジュール（client_management.c）

`server/src/client_management.c`は、Othelloサーバーに接続している全クライアントの情報を一元管理するモジュールです。  
クライアントの追加・削除、検索、初期化など、サーバーの基盤となる管理機能を提供します。

### 主な機能・構成

- **クライアントリストの初期化**
  - サーバー起動時に全クライアントスロットを初期化し、未使用状態にリセット

- **クライアントの追加・削除**
  - 新規接続時に空きスロットへクライアント情報（ソケットFD、アドレス、スレッドIDなど）を登録
  - 切断時には該当スロットをクリアし、再利用可能に

- **クライアント検索・取得**
  - ソケットFDからクライアントインデックスや情報構造体を取得
  - 必要に応じてミューテックスで排他制御し、スレッド安全にアクセス

- **排他制御**
  - クライアントリストへのアクセスは`clients_mutex`で保護し、複数スレッドからの同時操作による競合を防止

### 備考

- クライアント情報には、ソケットFD、所属ルームID、プレイヤーカラー、スレッドIDなどが含まれます
- サーバーの他モジュール（`client_handler.c`や`room_management.c`など）から呼び出され、全体の状態管理の基盤となっています
- サーバーの最大同時接続数（`MAX_CLIENTS`）を超える場合は新規接続を拒否し、安定運用を実現しています

## ゲームロジックモジュール（game_logic.c）

`server/src/game_logic.c`は、Othello（リバーシ）の**ゲームルール・盤面操作・勝敗判定**など、サーバー側のゲーム進行の根幹となるロジックを実装したモジュールです。

### 主な機能・構成

- **盤面操作・ルール判定**
  - 石を置けるかどうかの判定（有効手判定）
  - 石を置いた際の盤面更新（相手の石をひっくり返す処理）
  - 盤面内判定や方向ごとのひっくり返し判定など、Othelloのルールに忠実な実装

- **ゲーム進行管理**
  - ゲーム状態（盤面・ターン）の初期化
  - 各プレイヤーが置ける手があるかの判定（パス判定）
  - ゲーム終了条件の判定（両者パス or 盤面が埋まった場合）

- **勝敗判定**
  - ゲーム終了時に黒・白の石数をカウントし、勝者・引き分けを判定

- **補助関数**
  - 盤面範囲チェックや方向ごとのひっくり返し数計算など、ルール処理のための内部関数

### 備考

- サーバーの他モジュール（`client_handler.c`など）から呼び出され、クライアントの操作に応じて盤面やゲーム状態を更新します。
- Othelloの標準ルールに基づき、正確かつ効率的なゲーム進行を実現しています。
- 盤面サイズや石の色（1:黒, 2:白）などは定数・構造体で管理されており、拡張性にも配慮した設計です。

## 通信プロトコル実装モジュール（protocol.c）

`server/src/protocol.c`は、Othelloサーバーとクライアント間でやり取りされる**バイナリメッセージの送受信処理**を実装したモジュールです。  
このファイルはクライアント側と共通の`protocol.h`で定義された`Message`構造体を用い、ソケット通信の低レイヤを担います。

### 主な機能・構成

- **メッセージ送信（sendMessage）**
  - 指定したソケットに対して`Message`構造体全体をバイナリ形式で送信
  - 送信途中で切断やエラーが発生した場合はエラーを返す
  - 複数回の`send`呼び出しで全バイト送信を保証

- **メッセージ受信（receiveMessage）**
  - 指定したソケットから`Message`構造体全体をバイナリ形式で受信
  - 受信途中で切断やエラーが発生した場合はエラーや切断を返す
  - 複数回の`recv`呼び出しで全バイト受信を保証

### 備考

- 通信はバイナリ形式で行われ、`Message`構造体のサイズ分だけ送受信します。
- サーバー・クライアント双方で同じ実装を利用することで、通信仕様のズレや型不一致を防止しています。
- 通信エラーや切断時には標準エラー出力にエラーメッセージを出力し、上位層で適切にハンドリングできるようになっています。

## 部屋管理モジュール（room_management.c）

`server/src/room_management.c`は、Othelloサーバーにおける**部屋（ルーム）機能の管理**を担うモジュールです。  
部屋の作成・参加・閉鎖、チャット履歴の管理、部屋ごとの排他制御など、マルチプレイ対戦の基盤となる機能を実装しています。

### 主な機能・構成

- **部屋リストの初期化**
  - サーバー起動時に全部屋スロットを初期化し、未使用状態にリセット
  - 各部屋ごとに専用のミューテックスを初期化

- **部屋の作成・参加・検索**
  - 新規部屋の作成（部屋IDの割り当て、作成者をPlayer 1として登録）
  - 既存部屋への参加（Player 2として登録、チャット履歴の送信、参加通知）
  - 部屋IDからの検索や空き部屋の検索

- **チャット履歴管理**
  - 各部屋ごとにチャット履歴をリングバッファで保持
  - 新規参加者への過去チャット送信、チャットの全員へのブロードキャスト

- **部屋の状態管理・排他制御**
  - 部屋ごとに専用ミューテックスで排他制御し、複数スレッドからの同時操作を安全に処理
  - 部屋の状態（待機中・対戦中・再戦中など）や参加者情報の管理

- **部屋の閉鎖・通知**
  - ゲーム終了や切断時に部屋を閉鎖し、参加者に通知
  - 閉鎖時にはクライアント情報もリセット

- **ユーティリティ関数**
  - 部屋内の相手プレイヤーソケット取得、全員へのメッセージ送信（ブロードキャスト）など

### 備考

- クライアント管理（`client_management.c`）と連携し、部屋とクライアントの情報を一貫して管理
- マルチスレッド環境下でも安全に部屋機能を運用できるよう、細かなロック設計がなされています
- チャット・再戦・部屋閉鎖など、Othelloのオンライン対戦に必要な部屋機能を網羅しています

## 通信プロトコル定義ヘッダ（protocol.h）

`server/src/protocol.h`は、Othelloサーバーとクライアント間でやり取りされる**通信プロトコルの仕様（メッセージ型・構造体・定数）を定義した共通ヘッダファイル**です。  
このファイルはサーバー・クライアント双方でインクルードされ、通信内容の整合性を保証します。

### 主な内容

- **メッセージタイプ（MessageType）のenum定義**  
  - 部屋作成/参加/開始/コマ配置/再戦/チャット/エラーなど、全通信ケースを網羅

- **各メッセージタイプごとのペイロード構造体定義**  
  - 盤面情報、チャット内容、部屋情報、通知メッセージなど、やり取りされるデータの詳細を定義

- **共通通信メッセージ構造体（Message）**  
  - `type`でメッセージ種別を判別し、`union`で各種ペイロードを格納
  - サーバー・クライアント間の全通信はこの構造体で統一

- **通信補助関数のプロトタイプ宣言**  
  - `sendMessage`/`receiveMessage`でバイナリ送受信を行う

### 備考

- **クライアントとサーバーで同じファイルを共有**することで、通信仕様のズレや型不一致を防ぎます。
- 盤面サイズや最大文字数などの定数もここで一元管理されています。
- チャットや再戦、ゲーム進行などOthelloの全機能に対応した設計です。
- メッセージの送受信はバイナリ形式で行われ、`Message`構造体のサイズ分だけ送受信します。
- 通信エラーや切断時には標準エラー出力にエラーメッセージを出力し、上位層で適切にハンドリングできるようになっています。
- サーバー・クライアント双方で同じ実装を利用することで、通信仕様のズレや型不一致を防止しています。
- 通信エラーや切断時には標準エラー出力にエラーメッセージを出力し、上位層で適切にハンドリングできるようになっています。

## サーバー用Makefileについて

このディレクトリの`Makefile`は、Othelloサーバーアプリケーション（C言語）のビルドを自動化するためのものです。

### 主な特徴

- `make`コマンドでサーバーの全ソースコードをビルドし、`server_app.out`という実行ファイルを生成します。
- ソースファイルごとに`obj`ディレクトリにオブジェクトファイルを出力し、最終的にリンクしてサーバー本体を作成します。
- `make clean`でビルド生成物（オブジェクトファイル・実行ファイル）をまとめて削除できます。

### 備考

- サーバーはこの実行ファイル（`server_app.out`）を直接起動して利用します。
- クライアント用Makefileと異なり、フロントエンドとの連携やコピー処理はありません。サーバー単体で動作します。