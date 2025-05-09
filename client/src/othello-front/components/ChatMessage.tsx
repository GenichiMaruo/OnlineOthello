// components/ChatMessage.tsx
"use client";

import { ChatMessage } from "../hooks/useOthelloGame"; // useOthelloGameから型をインポート
import { Avatar, AvatarFallback } from "@/components/ui/avatar"; // shadcn/ui
import { cn } from "@/lib/utils"; // shadcn/ui のユーティリティ

interface ChatMessageProps {
  message: ChatMessage;
}

const ChatMessageItem: React.FC<ChatMessageProps> = ({ message }) => {
  const date = new Date(message.timestamp * 1000); // Unix timestamp (秒) をミリ秒に変換
  const timeString = date.toLocaleTimeString([], {
    hour: "2-digit",
    minute: "2-digit",
  });

  const getSenderInitial = (name: string) => {
    if (!name) return "S"; // System
    const parts = name.split(" ");
    if (parts.length > 1) {
      return `${parts[0][0]}${parts[parts.length - 1][0]}`.toUpperCase();
    }
    return name.substring(0, 2).toUpperCase();
  };

  // プレイヤーカラーに基づいてアバターのスタイルを変える例
  const getAvatarColorClass = (color: number) => {
    if (color === 1) return "bg-black text-white"; // Black
    if (color === 2) return "bg-white text-black border border-gray-400"; // White
    return "bg-gray-500 text-white"; // System or unknown
  };

  // senderDisplayNameとsenderColorに応じて表示名を調整
  let displayName = message.senderDisplayName;
  if (message.senderColor === 0) {
    displayName = "System";
  } else if (!displayName || displayName.trim() === "") {
    displayName =
      message.senderColor === 1 ? "Player 1 (Black)" : "Player 2 (White)";
  }

  return (
    <div
      className={cn(
        "flex items-start gap-3 p-2 rounded-lg",
        message.isMine ? "justify-end" : "justify-start"
      )}
    >
      {!message.isMine && (
        <Avatar className="h-8 w-8">
          <AvatarFallback
            className={cn("text-xs", getAvatarColorClass(message.senderColor))}
          >
            {getSenderInitial(displayName)}
          </AvatarFallback>
        </Avatar>
      )}
      <div
        className={cn(
          "max-w-[70%] break-words rounded-lg px-3 py-2 text-sm",
          message.isMine
            ? "bg-blue-500 text-white"
            : "bg-gray-200 dark:bg-gray-700 text-gray-900 dark:text-gray-100"
        )}
      >
        {!message.isMine && (
          <p className="text-xs font-semibold mb-0.5">{displayName}</p>
        )}
        <p>{message.message}</p>
        <p className="text-xs opacity-70 mt-1 text-right">{timeString}</p>
      </div>
      {message.isMine && (
        <Avatar className="h-8 w-8">
          <AvatarFallback
            className={cn("text-xs", getAvatarColorClass(message.senderColor))}
          >
            {getSenderInitial(displayName)}
          </AvatarFallback>
        </Avatar>
      )}
    </div>
  );
};

export default ChatMessageItem;
