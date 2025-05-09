import { WebSocketServer, WebSocket } from "ws";
import { spawn, ChildProcessWithoutNullStreams } from "child_process";
import path from "path";

const C_CLIENT_EXECUTABLE = path.join(__dirname, "bin", "client_app.out"); // Cクライアントの実行ファイルパス
const WS_PORT = 8080; // WebSocketサーバーのポート

let cProcess: ChildProcessWithoutNullStreams | null = null;
let wss: WebSocketServer | null = null;
const clients = new Set<WebSocket>();

console.log("Starting middleware server...");

function startCProcess() {
  if (cProcess) {
    console.log("C client process already running.");
    return;
  }

  try {
    console.log(`Spawning C client: ${C_CLIENT_EXECUTABLE}`);
    cProcess = spawn(C_CLIENT_EXECUTABLE, [], {
      stdio: ["pipe", "pipe", "pipe"],
      // cwd: path.dirname(C_CLIENT_EXECUTABLE) // 必要に応じて作業ディレクトリ指定
    });

    cProcess.stdout.on("data", (data) => {
      const lines = data.toString().split("\n");
      lines.forEach((line: string) => {
        if (line.trim()) {
          console.log("[C stdout]", line);
          try {
            // JSONとしてパースできるか試す (必須ではないが、形式チェックになる)
            JSON.parse(line);
            // 全クライアントにブロードキャスト
            broadcast(line);
          } catch {
            console.warn("Received non-JSON line from C stdout:", line);
            // JSONでないログなどもそのまま送る場合
            broadcast(JSON.stringify({ type: "rawLog", message: line }));
          }
        }
      });
    });

    cProcess.stderr.on("data", (data) => {
      console.error(`[C stderr]: ${data}`);
      broadcast(
        JSON.stringify({
          type: "error",
          message: `C stderr: ${data.toString().trim()}`,
        })
      );
    });

    cProcess.on("close", (code) => {
      console.log(`C client process exited with code ${code}`);
      cProcess = null;
      broadcast(
        JSON.stringify({
          type: "error",
          message: `C client process exited unexpectedly (code: ${code})`,
        })
      );

      // 再起動処理
      console.log("Attempting to restart C client process...");
      startCProcess(); // 再起動
    });

    cProcess.on("error", (err) => {
      console.error("Failed to spawn C client process:", err);
      cProcess = null;
      broadcast(
        JSON.stringify({
          type: "error",
          message: "Failed to start C client process",
        })
      );
    });

    console.log("C client process spawned successfully.");
  } catch (error) {
    console.error("Error spawning C process:", error);
    broadcast(
      JSON.stringify({ type: "error", message: "Failed to spawn C process" })
    );
    cProcess = null;
  }
}

function startWebSocketServer() {
  wss = new WebSocketServer({ port: WS_PORT });
  console.log(`WebSocket server listening on ws://localhost:${WS_PORT}`);

  wss.on("connection", (ws) => {
    console.log("Client connected via WebSocket");
    clients.add(ws);

    // 接続時に現在の状態を送る (Cプロセスが起動していれば)
    if (cProcess) {
      // Cプロセスに状態問い合わせコマンドを送る? (C側に実装が必要)
      // または、ミドルウェア側で最後に受信した状態を送る
      ws.send(
        JSON.stringify({ type: "info", message: "Connected to middleware." })
      );
    } else {
      ws.send(
        JSON.stringify({
          type: "error",
          message: "C client process not running.",
        })
      );
    }

    ws.on("message", (message) => {
      const messageString = message.toString();
      console.log("Received from WebSocket client:", messageString);
      try {
        // JSON形式のコマンドとしてCプロセスに送信
        if (cProcess && cProcess.stdin && !cProcess.stdin.destroyed) {
          // 簡単なバリデーション (JSONかどうか)
          JSON.parse(messageString); // パースできなければエラー
          cProcess.stdin.write(messageString + "\n");
          console.log("Sent to C stdin:", messageString);
        } else {
          console.error(
            "Cannot send command: C process not running or stdin closed."
          );
          ws.send(
            JSON.stringify({
              type: "error",
              message: "Cannot send command to C process.",
            })
          );
        }
      } catch (e) {
        console.error(
          "Invalid JSON command from WebSocket client:",
          messageString,
          e
        );
        ws.send(
          JSON.stringify({ type: "error", message: "Invalid command format." })
        );
      }
    });

    ws.on("close", () => {
      console.log("Client disconnected");
      clients.delete(ws);
    });

    ws.on("error", (error) => {
      console.error("WebSocket error:", error);
      clients.delete(ws); // エラーが発生した接続は削除
    });
  });

  wss.on("error", (error) => {
    console.error("WebSocket Server error:", error);
    wss = null; // 再起動などが必要かも
  });
}

function broadcast(message: string) {
  clients.forEach((client) => {
    if (client.readyState === WebSocket.OPEN) {
      client.send(message, (err) => {
        if (err) {
          console.error("Error sending message to client:", err);
          // エラーが発生したクライアントは切断された可能性があるので削除
          clients.delete(client);
        }
      });
    } else {
      // 接続が確立していない、または閉じているクライアントは削除
      clients.delete(client);
    }
  });
}

// --- サーバー起動 ---
startCProcess();
startWebSocketServer();

// --- 安全な終了処理 (任意) ---
process.on("SIGINT", () => {
  console.log("SIGINT received. Shutting down...");
  if (cProcess) {
    console.log("Killing C process...");
    cProcess.kill("SIGTERM"); // または SIGINT
  }
  if (wss) {
    console.log("Closing WebSocket server...");
    wss.close();
  }
  process.exit(0);
});
