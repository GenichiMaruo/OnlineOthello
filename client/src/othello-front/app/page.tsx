"use client";

import Head from "next/head";
import { useOthelloGame } from "../hooks/useOthelloGame";
import Board from "../components/Board";
import GameInfo from "../components/GameInfo";
import ControlPanel from "../components/ControlPanel";

export default function Home() {
  const {
    gameState,
    createRoom,
    joinRoom,
    startGame,
    placePiece,
    sendRematchResponse,
    quitGame,
  } = useOthelloGame();

  // 操作不能にする状態かを判定 (isDisabled の定義は変更なし)
  const isDisabled =
    gameState.clientState === "Connecting" ||
    gameState.clientState === "CreatingRoom" ||
    // ...(省略)...
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

      {/* ゲームエリア全体を囲むコンテナ */}
      <div className="flex flex-col lg:flex-row gap-6 w-full max-w-5xl">
        {" "}
        {/* 横幅を広げる max-w-5xl */}
        {/* 左側: 情報とコントロール (小画面では縦積み) */}
        <div className="flex flex-col gap-4 lg:w-1/3">
          {" "}
          {/* lg以上で幅を指定 */}
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
          />
        </div>
        {/* 右側: 盤面エリア (ゲームがアクティブな場合) */}
        <div className="flex-1 flex flex-col items-center justify-center lg:w-2/3">
          {" "}
          {/* 残りの幅を使う */}
          {isGameActive ? (
            <div className="w-full max-w-lg">
              {" "}
              {/* 盤面の最大幅を設定 (例: max-w-lg) */}
              <Board
                board={gameState.board}
                myColor={gameState.myColor}
                isMyTurn={gameState.isMyTurn}
                onPlacePiece={placePiece}
                disabled={isDisabled || !gameState.isMyTurn}
              />
            </div>
          ) : (
            // ゲームが始まっていない場合のメッセージ
            <div className="flex items-center justify-center h-64 bg-gray-200 dark:bg-gray-800 rounded-lg w-full max-w-lg">
              {gameState.isConnected && gameState.clientState === "Lobby" ? (
                <p className="text-muted-foreground">
                  Create or join a room to start playing.
                </p>
              ) : gameState.isConnected ? (
                <p className="text-muted-foreground">Waiting for game...</p> // 接続中など
              ) : (
                <p className="text-muted-foreground">Connecting to server...</p>
              )}
            </div>
          )}
        </div>
      </div>
    </div>
  );
}
