"use client";

import Head from "next/head";
import { useOthelloGame } from "../hooks/useOthelloGame";
import Board from "../components/Board";
import GameInfo from "../components/GameInfo";
import ControlPanel from "../components/ControlPanel";
import ChatWindow from "../components/ChatWindow";

export default function Home() {
  const {
    gameState,
    createRoom,
    joinRoom,
    startGame,
    placePiece,
    sendRematchResponse,
    quitGame,
    connectToServer,
    sendChatMessage,
  } = useOthelloGame();

  // 操作不能にする状態かを判定 (isDisabled の定義は変更なし)
  const isDisabled =
    gameState.clientState === "Connecting" ||
    gameState.clientState === "CreatingRoom" ||
    gameState.clientState === "JoiningRoom" ||
    gameState.clientState === "StartingGame" ||
    gameState.clientState === "PlacingPiece" ||
    gameState.clientState === "SendingRematch" ||
    gameState.clientState === "GameOver" || // ゲームオーバー中も盤面操作は不可
    gameState.clientState === "OpponentTurn" || // 相手ターン中も操作不可
    !gameState.isConnected || // 未接続
    gameState.clientState === "Quitting" ||
    gameState.clientState === "Disconnected" ||
    gameState.clientState === "ConnectionClosed";

  // ゲームがアクティブかどうか (盤面を表示するかどうか)
  const isGameActive =
    gameState.roomId !== null &&
    gameState.clientState !== "Lobby" &&
    gameState.clientState !== "Disconnected" &&
    gameState.clientState !== "Connecting" &&
    gameState.clientState !== "CreatingRoom" &&
    gameState.clientState !== "JoiningRoom" &&
    gameState.clientState !== "Quitting" &&
    gameState.clientState !== "ConnectionClosed";

  return (
    <div className="flex flex-col items-center min-h-screen bg-gray-100 dark:bg-gray-900 text-gray-900 dark:text-gray-100 p-4">
      <Head>
        <title>Othello Online</title>
        <meta name="description" content="Online Othello Game" />
        <link rel="icon" href="/favicon.ico" />
      </Head>

      <h1 className="text-3xl sm:text-4xl lg:text-5xl font-bold my-6">
        Othello Game
      </h1>

      {/* ゲームエリア全体を囲むコンテナ: 盤面とチャットウィンドウを横並びにする */}
      <div className="grid grid-cols-1 lg:grid-cols-3 gap-6 w-full max-w-7xl">
        {" "}
        {/* 最大幅を広げる max-w-7xl */}
        {/* 左側: 情報とコントロール (lg未満では1列、lg以上で1列目) */}
        <div className="lg:col-span-1 flex flex-col gap-4">
          <GameInfo
            isConnected={gameState.isConnected}
            clientState={gameState.clientState}
            roomId={gameState.roomId}
            myColor={gameState.myColor}
            isMyTurn={gameState.isMyTurn}
            isGameOver={gameState.isGameOver}
            lastMessage={gameState.lastMessage}
            errorMessage={gameState.errorMessage}
          />
          <ControlPanel
            clientState={gameState.clientState}
            roomId={gameState.roomId}
            myColor={gameState.myColor}
            rematchOffered={gameState.rematchOffered}
            onCreateRoom={createRoom}
            onJoinRoom={joinRoom}
            onStartGame={startGame}
            onRematchResponse={sendRematchResponse}
            onQuit={quitGame}
            onConnect={(ip, port) => {
              console.log("Connect to server", ip, port);
              connectToServer(ip, port);
            }}
          />
        </div>
        {/* 中央: 盤面エリア (lg未満では次の行、lg以上で2列目) */}
        <div className="lg:col-span-1 flex flex-col items-center justify-center">
          {isGameActive ? (
            <div className="w-full max-w-lg aspect-square">
              {" "}
              {/* アスペクト比を維持 */}
              <Board
                board={gameState.board}
                myColor={gameState.myColor}
                isMyTurn={gameState.isMyTurn}
                onPlacePiece={placePiece}
                disabled={isDisabled || !gameState.isMyTurn}
              />
            </div>
          ) : (
            <div className="flex items-center justify-center h-64 sm:h-80 md:h-96 lg:aspect-square lg:h-auto bg-gray-200 dark:bg-gray-800 rounded-lg w-full max-w-lg">
              {gameState.isConnected && gameState.clientState === "Lobby" ? (
                <p className="text-muted-foreground">
                  Create or join a room to start playing.
                </p>
              ) : gameState.isConnected ? (
                <p className="text-muted-foreground">Waiting for game...</p>
              ) : (
                <p className="text-muted-foreground">Connecting to server...</p>
              )}
            </div>
          )}
        </div>
        {/* 右側: チャットウィンドウ (lg未満では次の行、lg以上で3列目) */}
        <div className="lg:col-span-1">
          <ChatWindow
            messages={gameState.chatMessages}
            onSendMessage={sendChatMessage}
            roomId={gameState.roomId}
            currentClientState={gameState.clientState}
          />
        </div>
      </div>
    </div>
  );
}
