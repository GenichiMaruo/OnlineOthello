"use client";

import { useState, useEffect, useRef, useCallback } from "react";

// Cクライアントから送られてくるJSONイベントの型 (必要に応じて拡張)
interface CEvent {
  type: string;
  state?: string; // stateChange
  roomId?: number; // stateChange, boardUpdate, etc.
  color?: number; // stateChange
  board?: number[][]; // boardUpdate
  message?: string; // serverMessage, error, log, gameOver
  level?: string; // log
  winner?: number; // gameOver
  result?: string; // rematchResult
}

// フックの状態
interface GameState {
  isConnected: boolean;
  isConnectedToServer: boolean; // サーバーに接続中かどうか
  clientState: string; // Cクライアントの状態名
  roomId: number | null;
  myColor: number | null; // 1: Black, 2: White
  board: number[][];
  lastMessage: string | null;
  errorMessage: string | null;
  isMyTurn: boolean;
  isGameOver: boolean;
  rematchOffered: boolean;
}

const WS_URL = "ws://localhost:8080"; // ミドルウェアのWebSocket URL

const initialBoard = () =>
  Array(8)
    .fill(0)
    .map(() => Array(8).fill(0));

export const useOthelloGame = () => {
  const [gameState, setGameState] = useState<GameState>({
    isConnected: false,
    isConnectedToServer: false,
    clientState: "Disconnected",
    roomId: null,
    myColor: null,
    board: initialBoard(),
    lastMessage: null,
    errorMessage: null,
    isMyTurn: false,
    isGameOver: false,
    rematchOffered: false,
  });
  const ws = useRef<WebSocket | null>(null);

  const connectWebSocket = useCallback(() => {
    if (ws.current && ws.current.readyState === WebSocket.OPEN) {
      console.log("WebSocket already connected.");
      return;
    }

    console.log(`Connecting to WebSocket: ${WS_URL}`);
    ws.current = new WebSocket(WS_URL);

    ws.current.onopen = () => {
      console.log("WebSocket connected");
      setGameState((prev) => ({
        ...prev,
        isConnected: true,
        clientState: "Lobby",
        errorMessage: null,
      }));

      ws.current?.send(JSON.stringify({ command: "getStatus" })); // 初期状態を取得
    };

    ws.current.onclose = () => {
      console.log("WebSocket disconnected");
      ws.current = null;

      setGameState((prev) => ({
        ...prev,
        isConnected: false,
        isConnectedToServer: false,
        clientState: "Disconnected",
        roomId: null,
        myColor: null,
        board: initialBoard(),
        errorMessage:
          prev.errorMessage === "WebSocket connection error." ||
          prev.errorMessage === "Failed to reconnect to the server."
            ? prev.errorMessage
            : "Disconnected from server.",
        isMyTurn: false,
        isGameOver: false,
        rematchOffered: false,
      }));
      // 再接続処理 (必要なら)
      // setTimeout(connectWebSocket, 5000);
    };

    ws.current.onerror = (error) => {
      console.error("WebSocket error:", error);
      // onclose も呼ばれるはずなので、状態リセットはそちらに任せる
      setGameState((prev) => ({
        ...prev,
        isConnected: false,
        isConnectedToServer: false,
        errorMessage: "WebSocket connection error.",
      }));
      if (ws.current && ws.current.readyState !== WebSocket.CLOSED) {
        ws.current.close();
      }
      ws.current = null;
    };

    ws.current.onmessage = (event) => {
      try {
        const data: CEvent = JSON.parse(event.data);
        console.log("Message from middleware:", data);

        setGameState((prev) => {
          // Deep copy of previous state to avoid mutation issues
          const newState: GameState = JSON.parse(JSON.stringify(prev)); // 型注釈追加
          let messageToShow: string | null = null; // 型注釈追加
          let errorToShow: string | null = null; // 型注釈追加

          switch (data.type) {
            case "stateChange":
              newState.clientState = data.state ?? prev.clientState;
              // roomId, myColor は number | null なので、0と比較して設定
              newState.roomId =
                typeof data.roomId === "number" ? data.roomId : prev.roomId;
              newState.myColor =
                typeof data.color === "number" ? data.color : prev.myColor;
              newState.isMyTurn = data.state === "MyTurn";
              newState.isGameOver = data.state === "GameOver";
              newState.isConnectedToServer = true; // サーバーに接続中
              if (data.state !== "GameOver") {
                newState.rematchOffered = false;
              }
              // エラー状態でない場合はエラーメッセージをクリア
              if (newState.errorMessage !== "WebSocket connection error.") {
                errorToShow = null;
              }
              console.log(
                `State changed to: ${newState.clientState}, Room: ${newState.roomId}, Color: ${newState.myColor}, MyTurn: ${newState.isMyTurn}`
              );
              break;
            case "boardUpdate":
              if (
                data.board &&
                Array.isArray(data.board) &&
                data.board.length === 8
              ) {
                // 簡単なバリデーション追加
                newState.board = data.board;
                console.log("Board updated");
              } else {
                console.warn("Received invalid board data:", data.board);
              }
              break;
            case "serverMessage":
              messageToShow = data.message ?? null;
              break;
            case "yourTurn":
              newState.isMyTurn = true;
              // サーバーから stateChange が来るはずなので、ここでは状態を変えない方が一貫性があるかも
              // newState.clientState = 'MyTurn';
              messageToShow = "It's your turn!";
              break;
            case "gameOver":
              newState.isGameOver = true;
              newState.isMyTurn = false;
              // newState.clientState = 'GameOver'; // stateChangeを待つ
              messageToShow = data.message ?? "Game Over!";
              break;
            case "rematchOffer":
              newState.rematchOffered = true;
              messageToShow = "Rematch offered.";
              break;
            case "rematchResult":
              newState.rematchOffered = false;
              messageToShow = `Rematch ${data.result}.`;
              if (data.result !== "agreed") {
                newState.clientState = "GameOver"; // ゲームオーバー状態に戻す
              } else {
                // newState.clientState = 'WaitingInRoom'; // 開始待ち状態へ
              }
              // stateChange イベントも来る想定なので、ここでは変えない方が良いかも
              break;
            case "error":
              errorToShow = data.message ?? "An unknown error occurred.";
              console.error("Received error event:", errorToShow);
              // エラー内容によっては状態を Disconnected や Lobby に戻す処理が必要かも
              if (errorToShow?.includes("client process exited")) {
                newState.clientState = "Disconnected";
              }
              if (errorToShow?.includes("room not found")) {
                newState.roomId = null;
                newState.clientState = "Lobby";
              }
              if (errorToShow?.includes("room full")) {
                newState.roomId = null;
                newState.clientState = "Lobby";
              }
              if (errorToShow?.includes("Waiting for opponent")) {
                newState.isMyTurn = false;
                newState.clientState = "WaitingInRoom";
              }
              if (errorToShow?.includes("game already started")) {
                newState.clientState = "GameOver";
              }
              if (errorToShow?.includes("game over")) {
                newState.isGameOver = true;
                newState.isMyTurn = false;
                newState.clientState = "GameOver";
              }
              if (errorToShow?.includes("rematch")) {
                newState.rematchOffered = false;
                newState.clientState = "GameOver";
              }
              if (errorToShow?.includes("not your turn")) {
                newState.isMyTurn = false;
                newState.clientState = "WaitingInRoom";
              }
              if (errorToShow?.includes("invalid move")) {
                newState.isMyTurn = false;
                newState.clientState = "WaitingInRoom";
              }
              if (errorToShow?.includes("not in a room")) {
                newState.roomId = null;
                newState.clientState = "Lobby";
              }
              if (errorToShow?.includes("not connected")) {
                newState.isConnected = false;
                newState.clientState = "Disconnected";
              }
              break;
            case "log":
              console.log(`[C:${data.level ?? "LOG"}] ${data.message ?? ""}`);
              break;
            case "info":
              messageToShow = data.message ?? null;
              break;
            case "rawLog":
              console.log("[C raw]", data.message);
              break;
            default:
              console.warn("Received unknown event type:", data.type);
          }

          // エラーメッセージを更新 (null でなければ上書き)
          if (errorToShow !== null) newState.errorMessage = errorToShow;
          // 通常メッセージを更新 (null でなければ上書き、エラーがあれば通常メッセージは表示しないことが多い)
          if (messageToShow !== null) newState.lastMessage = messageToShow;
          // エラーが発生したら、通常メッセージはクリアする（任意）
          if (errorToShow !== null) newState.lastMessage = null;

          // return する前に再度 isMyTurn, isGameOver を clientState に基づいて設定 (念のため)
          newState.isMyTurn = newState.clientState === "MyTurn";
          newState.isGameOver = newState.clientState === "GameOver";

          return newState;
        });
      } catch (e) {
        console.error(
          "Failed to parse message from middleware:",
          event.data,
          e
        );
        setGameState((prev) => ({
          ...prev,
          errorMessage: "Received invalid data from server.",
        }));
      }
    };
    // gameStateを依存配列から削除。再接続ロジックはonclose内で行う。
    // }, [gameState.isConnected]);
  }, []); // 初回のみ登録

  useEffect(() => {
    console.log("Attempting initial connection...");
    connectWebSocket();

    return () => {
      console.log("Closing WebSocket on unmount...");
      // reconnectAttempt.current = maxReconnectAttempts; // Unmount時は再接続しない
      ws.current?.close();
      ws.current = null;
    };
  }, [connectWebSocket]); // 依存配列は connectWebSocket のみでOK

  // コマンドをミドルウェアに送信する関数
  const sendCommand = useCallback((command: object) => {
    if (ws.current && ws.current.readyState === WebSocket.OPEN) {
      try {
        const commandString = JSON.stringify(command);
        console.log("Sending command to middleware:", commandString);
        ws.current.send(commandString);
        // コマンド送信時に一時的なエラー/メッセージはクリアする
        setGameState((prev) => ({
          ...prev,
          errorMessage: null,
          lastMessage: null,
        }));
      } catch (e) {
        console.error("Failed to stringify command:", command, e);
        setGameState((prev) => ({
          ...prev,
          errorMessage: "Failed to send command (invalid format).",
        }));
      }
    } else {
      console.error("WebSocket is not connected.");
      setGameState((prev) => ({
        ...prev,
        errorMessage: "Cannot send command: Not connected.",
      }));
    }
  }, []);

  const connectToServer = useCallback(
    (serverIp: string, serverPort: number) => {
      // ミドルウェアとサーバーの接続を確立するためのコマンドを送信
      sendCommand({ command: "connect", serverIp, serverPort });
      // サーバー接続後の状態を更新するための処理を追加
      setGameState((prev) => ({
        ...prev,
        isConnectedToServer: true,
        clientState: "Connecting",
        errorMessage: null,
      }));
    },
    [sendCommand]
  );

  // ゲーム操作用関数
  const createRoom = useCallback(
    (roomName: string = "DefaultRoom") => {
      sendCommand({ command: "create", roomName });
    },
    [sendCommand]
  );

  const joinRoom = useCallback(
    (roomId: number) => {
      sendCommand({ command: "join", roomId });
    },
    [sendCommand]
  );

  const startGame = useCallback(() => {
    // roomId は gameState から取得できるはず
    if (gameState.roomId !== null) {
      sendCommand({ command: "start", roomId: gameState.roomId });
    } else {
      console.error("Cannot start game: Not in a room.");
      setGameState((prev) => ({
        ...prev,
        errorMessage: "Cannot start game: Not in a room.",
      }));
    }
  }, [sendCommand, gameState.roomId]);

  const placePiece = useCallback(
    (row: number, col: number) => {
      if (gameState.roomId !== null) {
        sendCommand({ command: "place", roomId: gameState.roomId, row, col });
      } else {
        console.error("Cannot place piece: Not in a room.");
        setGameState((prev) => ({
          ...prev,
          errorMessage: "Cannot place piece: Not in a room.",
        }));
      }
    },
    [sendCommand, gameState.roomId]
  );

  const sendRematchResponse = useCallback(
    (agree: boolean) => {
      if (gameState.roomId !== null) {
        sendCommand({ command: "rematch", roomId: gameState.roomId, agree });
      } else {
        console.error("Cannot send rematch response: Not in a room.");
        setGameState((prev) => ({
          ...prev,
          errorMessage: "Cannot respond to rematch: Not in a room.",
        }));
      }
    },
    [sendCommand, gameState.roomId]
  );

  const quitGame = useCallback(() => {
    sendCommand({ command: "quit" });
    // WebSocket接続も閉じる（サーバー側で閉じるのを待っても良い）
    // ws.current?.close();
  }, [sendCommand]);

  return {
    gameState,
    sendCommand,
    createRoom,
    joinRoom,
    startGame,
    placePiece,
    sendRematchResponse,
    quitGame,
    connectToServer,
  };
};
