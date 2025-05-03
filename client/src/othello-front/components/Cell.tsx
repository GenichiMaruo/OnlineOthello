import React from "react";
import { cn } from "@/lib/utils"; // shadcn/ui のユーティリティ関数
import { Button } from "@/components/ui/button"; // shadcn/ui の Button

interface CellProps {
  value: number; // 0: empty, 1: black, 2: white
  isClickable: boolean;
  onClick: () => void;
}

const Cell: React.FC<CellProps> = ({ value, isClickable, onClick }) => {
  const pieceColorClass =
    value === 1
      ? "bg-black"
      : value === 2
      ? "bg-white border border-gray-400"
      : "";
  const hoverClass = isClickable
    ? "hover:bg-green-700 hover:ring-2 hover:ring-yellow-300"
    : "";

  return (
    // Buttonコンポーネントをセルのベースとして使用
    <Button
      variant="outline" // または他のバリアント
      className={cn(
        "w-full h-full p-0 aspect-square rounded-none bg-green-600 border border-green-800 flex items-center justify-center", // 基本スタイル
        isClickable ? "cursor-pointer" : "cursor-default",
        hoverClass // ホバーエフェクト
      )}
      onClick={isClickable ? onClick : undefined}
      aria-label={`Cell at row ${0} col ${0}, value ${value}`} // Replace 0 with actual rowIndex and colIndex if available
      title={isClickable ? "Click to place piece" : ""}
      disabled={!isClickable && value === 0} // 空きマスだがクリック不可の場合もdisabledに
    >
      {/* 駒の表示 */}
      {value !== 0 && (
        <div
          className={cn(
            "w-[75%] h-[75%] rounded-full shadow-md", // サイズと形状
            pieceColorClass // 色
          )}
        />
      )}
    </Button>
  );
};

export default Cell;
