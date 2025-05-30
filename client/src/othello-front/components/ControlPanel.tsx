import React, { useState } from "react";
import { Button } from "@/components/ui/button";
import { Input } from "@/components/ui/input";
import { Card, CardContent, CardHeader, CardTitle } from "@/components/ui/card";
import { Loader2 } from "lucide-react"; // ローディングアイコン

interface ControlPanelProps {
  clientState: string;
  roomId: number | null;
  myColor: number | null;
  rematchOffered: boolean;
  onCreateRoom: (roomName?: string) => void;
  onJoinRoom: (roomId: number) => void;
  onStartGame: () => void;
  onRematchResponse: (agree: boolean) => void;
  onQuit: () => void;
  onConnect: (ip: string, port: number) => void;
}

const ControlPanel: React.FC<ControlPanelProps> = ({
  clientState,
  roomId,
  myColor,
  rematchOffered,
  onCreateRoom,
  onJoinRoom,
  onStartGame,
  onRematchResponse,
  onQuit,
  onConnect,
}) => {
  const [joinRoomId, setJoinRoomId] = useState("");
  const [createRoomName, setCreateRoomName] = useState(""); // オプション: 部屋名入力
  const [serverIp, setServerIp] = useState("");
  const [serverPort, setServerPort] = useState("");

  const isLoading =
    clientState === "Connecting" ||
    clientState === "CreatingRoom" ||
    clientState === "JoiningRoom" ||
    clientState === "StartingGame" ||
    clientState === "PlacingPiece" ||
    clientState === "SendingRematch";

  const handleJoin = () => {
    const id = parseInt(joinRoomId, 10);
    if (!isNaN(id)) {
      onJoinRoom(id);
      setJoinRoomId("");
    } else {
      // TODO: shadcn/ui の Toast などでエラー表示
      alert("Please enter a valid Room ID (number).");
    }
  };

  const handleCreate = () => {
    onCreateRoom(createRoomName || undefined); // 空ならデフォルト名
    setCreateRoomName("");
  };

  const handleConnect = () => {
    const portNumber = serverPort ? parseInt(serverPort, 10) : 10000;
    if (!serverIp) {
      alert("Please enter a valid server IP.");
      return;
    }
    onConnect(serverIp, isNaN(portNumber) ? 10000 : portNumber);
  };

  return (
    <Card className="w-full max-w-md mx-auto my-4">
      <CardHeader>
        <CardTitle>Controls</CardTitle>
      </CardHeader>
      <CardContent className="space-y-4">
        {/* 接続画面 */}
        {clientState === "Disconnected" && (
          <div className="space-y-2">
            <Input
              type="text"
              placeholder="Server IP (e.g. 127.0.0.1)"
              value={serverIp}
              onChange={(e) => setServerIp(e.target.value)}
              disabled={isLoading}
            />
            <Input
              type="number"
              placeholder="Port (default: 10000)"
              value={serverPort}
              onChange={(e) => setServerPort(e.target.value)}
              disabled={isLoading}
            />
            <Button onClick={handleConnect} disabled={isLoading || !serverIp}>
              {isLoading && clientState === "Connecting" ? (
                <Loader2 className="mr-2 h-4 w-4 animate-spin" />
              ) : null}
              Connect to Server
            </Button>
          </div>
        )}

        {/* 以下は既存のロビーなどの表示（省略せずに残す） */}
        {clientState === "Lobby" && (
          <div className="space-y-2">
            <div className="flex space-x-2">
              <Input
                type="text"
                placeholder="Room Name (optional)"
                value={createRoomName}
                onChange={(e) => setCreateRoomName(e.target.value)}
                disabled={isLoading}
              />
              <Button onClick={handleCreate} disabled={isLoading}>
                {isLoading && clientState === "CreatingRoom" ? (
                  <Loader2 className="mr-2 h-4 w-4 animate-spin" />
                ) : null}
                Create Room
              </Button>
            </div>
            <div className="flex space-x-2">
              <Input
                type="number"
                placeholder="Enter Room ID to Join"
                value={joinRoomId}
                onChange={(e) => setJoinRoomId(e.target.value)}
                disabled={isLoading}
              />
              <Button onClick={handleJoin} disabled={!joinRoomId || isLoading}>
                {isLoading && clientState === "JoiningRoom" ? (
                  <Loader2 className="mr-2 h-4 w-4 animate-spin" />
                ) : null}
                Join Room
              </Button>
            </div>
          </div>
        )}

        {/* その他のUI（WaitingInRoom、GameOverなど）はそのまま残す */}

        {clientState === "WaitingInRoom" &&
          myColor === 1 &&
          roomId !== null && (
            <Button onClick={onStartGame} disabled={isLoading}>
              {isLoading && clientState === "StartingGame" ? (
                <Loader2 className="mr-2 h-4 w-4 animate-spin" />
              ) : null}
              Start Game
            </Button>
          )}

        {clientState === "WaitingInRoom" &&
          myColor === 2 &&
          roomId !== null && (
            <p className="text-sm text-muted-foreground">
              Waiting for the host to start...
            </p>
          )}

        {clientState === "GameOver" && rematchOffered && roomId !== null && (
          <div className="space-y-2">
            <p className="text-sm font-medium">Rematch?</p>
            <div className="flex space-x-2">
              <Button
                onClick={() => onRematchResponse(true)}
                disabled={isLoading}
              >
                {isLoading && clientState === "SendingRematch" ? (
                  <Loader2 className="mr-2 h-4 w-4 animate-spin" />
                ) : null}
                Yes
              </Button>
              <Button
                onClick={() => onRematchResponse(false)}
                disabled={isLoading}
                variant="secondary"
              >
                {isLoading && clientState === "SendingRematch" ? (
                  <Loader2 className="mr-2 h-4 w-4 animate-spin" />
                ) : null}
                No
              </Button>
            </div>
          </div>
        )}

        {isLoading && (
          <div className="flex items-center justify-center text-sm text-muted-foreground">
            <Loader2 className="mr-2 h-4 w-4 animate-spin" />
            Processing...
          </div>
        )}

        {clientState !== "Disconnected" &&
          clientState !== "Connecting" &&
          clientState !== "Quitting" && (
            <Button
              onClick={onQuit}
              variant="destructive"
              className="w-full mt-4"
              disabled={isLoading}
            >
              Quit / Leave Room
            </Button>
          )}
      </CardContent>
    </Card>
  );
};

export default ControlPanel;
