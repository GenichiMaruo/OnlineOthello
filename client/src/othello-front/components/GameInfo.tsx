import React from "react";
import {
  Card,
  CardContent,
  CardDescription,
  CardHeader,
  CardTitle,
} from "@/components/ui/card";
import { Alert, AlertDescription, AlertTitle } from "@/components/ui/alert";
import { Info, AlertTriangle } from "lucide-react"; // アイコン例

interface GameInfoProps {
  isConnected: boolean;
  clientState: string;
  roomId: number | null;
  myColor: number | null;
  isMyTurn: boolean;
  isGameOver: boolean;
  lastMessage: string | null;
  errorMessage: string | null;
}

const GameInfo: React.FC<GameInfoProps> = ({
  isConnected,
  clientState,
  roomId,
  myColor,
  isMyTurn,
  isGameOver,
  lastMessage,
  errorMessage,
}) => {
  const colorString =
    myColor === 1
      ? "Black (X)"
      : myColor === 2
      ? "White (O)"
      : "Spectator/None";
  const turnString = isGameOver
    ? "Game Over"
    : isMyTurn
    ? "Your Turn"
    : "Opponent's Turn";
  const connectionStatus = isConnected ? "Connected" : "Disconnected";

  return (
    <Card className="w-full max-w-md mx-auto my-4">
      <CardHeader>
        <CardTitle>Game Status</CardTitle>
        <CardDescription>Current state of the Othello game.</CardDescription>
      </CardHeader>
      <CardContent className="space-y-3">
        <div className="flex items-center justify-between">
          <span className="text-sm font-medium text-muted-foreground">
            Connection
          </span>
          <span
            className={`text-sm font-semibold ${
              isConnected ? "text-green-600" : "text-red-600"
            }`}
          >
            {connectionStatus} ({clientState})
          </span>
        </div>
        {roomId !== null && (
          <div className="flex items-center justify-between">
            <span className="text-sm font-medium text-muted-foreground">
              Room ID
            </span>
            <span className="text-sm font-semibold">{roomId}</span>
          </div>
        )}
        {myColor !== null &&
          roomId !== null && ( // 部屋にいる場合のみ色表示
            <div className="flex items-center justify-between">
              <span className="text-sm font-medium text-muted-foreground">
                Your Color
              </span>
              <span className="text-sm font-semibold">{colorString}</span>
            </div>
          )}
        {roomId !== null &&
          !isGameOver &&
          clientState !== "WaitingInRoom" && ( // ゲーム中のみターン表示
            <div className="flex items-center justify-between">
              <span className="text-sm font-medium text-muted-foreground">
                Turn
              </span>
              <span
                className={`text-sm font-bold ${
                  isMyTurn ? "text-blue-600 animate-pulse" : "text-gray-500"
                }`}
              >
                {turnString}
              </span>
            </div>
          )}
        {isGameOver && roomId !== null && (
          <div className="flex items-center justify-between">
            <span className="text-sm font-medium text-muted-foreground">
              Game Result
            </span>
            <span className="text-sm font-bold text-purple-600">
              {turnString}
            </span>
          </div>
        )}

        {/* メッセージ表示 */}
        {lastMessage &&
          !errorMessage && ( // エラーがない場合のみ通常メッセージ表示
            <Alert variant="default">
              <Info className="h-4 w-4" />
              <AlertTitle>Info</AlertTitle>
              <AlertDescription>{lastMessage}</AlertDescription>
            </Alert>
          )}
        {/* エラーメッセージ表示 */}
        {errorMessage && (
          <Alert variant="destructive">
            <AlertTriangle className="h-4 w-4" />
            <AlertTitle>Error</AlertTitle>
            <AlertDescription>{errorMessage}</AlertDescription>
          </Alert>
        )}
      </CardContent>
    </Card>
  );
};

export default GameInfo;
