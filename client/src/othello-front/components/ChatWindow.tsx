// components/ChatWindow.tsx
"use client";

import { useState, useRef, useEffect, FormEvent } from "react";
import { ChatMessage } from "../hooks/useOthelloGame"; // useOthelloGameから型をインポート
import ChatMessageItem from "./ChatMessage";
import { Input } from "@/components/ui/input";
import { Button } from "@/components/ui/button";
import { ScrollArea } from "@/components/ui/scroll-area";
import { PaperPlaneIcon } from "@radix-ui/react-icons"; // 例: 送信アイコン

interface ChatWindowProps {
  messages: ChatMessage[];
  onSendMessage: (message: string) => void;
  roomId: number | null;
  currentClientState: string; // チャットを無効化するために使用
}

const ChatWindow: React.FC<ChatWindowProps> = ({
  messages,
  onSendMessage,
  roomId,
  currentClientState,
}) => {
  const [newMessage, setNewMessage] = useState("");
  const scrollAreaRef = useRef<HTMLDivElement>(null); // ScrollArea内のdiv要素への参照

  useEffect(() => {
    // 新しいメッセージが追加されたら一番下までスクロール
    if (scrollAreaRef.current) {
      const viewport = scrollAreaRef.current.querySelector(
        'div[style*="overflow: scroll;"]'
      );
      if (viewport) {
        viewport.scrollTop = viewport.scrollHeight;
      } else {
        // Fallback if the specific viewport div is not found (e.g. ScrollArea structure changed)
        scrollAreaRef.current.scrollTop = scrollAreaRef.current.scrollHeight;
      }
    }
  }, [messages]);

  const handleSubmit = (e: FormEvent) => {
    e.preventDefault();
    if (newMessage.trim() && roomId !== null) {
      onSendMessage(newMessage.trim());
      setNewMessage("");
    }
  };

  const isChatDisabled =
    roomId === null ||
    currentClientState === "Disconnected" ||
    currentClientState === "Connecting" ||
    currentClientState === "Quitting";
  // 必要に応じて他の状態でも無効化

  if (!isChatDisabled && roomId === null) {
    // ルームにいないが、チャットウィンドウ自体は表示する場合のプレースホルダー
    // (このケースは isChatDisabled でカバーされるはずだが念のため)
    return (
      <div className="flex flex-col h-full border rounded-lg bg-gray-50 dark:bg-gray-800 p-4">
        <div className="flex-grow flex items-center justify-center">
          <p className="text-muted-foreground">Join a room to chat.</p>
        </div>
      </div>
    );
  }

  return (
    <div className="flex flex-col h-full border rounded-lg bg-gray-50 dark:bg-gray-800">
      <div className="p-3 border-b dark:border-gray-700">
        <h2 className="text-lg font-semibold">
          {roomId !== null ? `Room Chat (ID: ${roomId})` : "Chat"}
        </h2>
      </div>

      <div className="flex-grow overflow-hidden">
        <ScrollArea className="h-full px-2" ref={scrollAreaRef}>
          <div className="flex flex-col gap-2 pb-4">
            {messages.length === 0 && !isChatDisabled && (
              <div className="flex items-center justify-center h-full">
                <p className="text-sm text-muted-foreground">
                  No messages yet.
                </p>
              </div>
            )}
            {messages.map((msg) => (
              <ChatMessageItem key={msg.id} message={msg} />
            ))}
            {isChatDisabled && roomId === null && (
              <div className="flex items-center justify-center h-full">
                <p className="text-sm text-muted-foreground">
                  Connect to a server and join a room to chat.
                </p>
              </div>
            )}
          </div>
        </ScrollArea>
      </div>

      <form
        onSubmit={handleSubmit}
        className="p-3 border-t dark:border-gray-700 flex gap-2"
      >
        <Input
          type="text"
          value={newMessage}
          onChange={(e) => setNewMessage(e.target.value)}
          placeholder={isChatDisabled ? "Chat disabled" : "Type a message..."}
          disabled={isChatDisabled}
          className="flex-grow"
        />
        <Button type="submit" disabled={isChatDisabled || !newMessage.trim()}>
          <PaperPlaneIcon className="h-4 w-4" />
          <span className="sr-only">Send</span>
        </Button>
      </form>
    </div>
  );
};

export default ChatWindow;
