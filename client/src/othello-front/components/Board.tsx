import React from "react";
import Cell from "./Cell";

interface BoardProps {
  board: number[][];
  myColor: number | null;
  isMyTurn: boolean;
  onPlacePiece: (row: number, col: number) => void;
  disabled: boolean;
}

const Board: React.FC<BoardProps> = ({
  board,
  isMyTurn,
  onPlacePiece,
  disabled,
}) => {
  const handleClick = (row: number, col: number) => {
    if (disabled || !isMyTurn || board[row]?.[col] !== 0) {
      console.log(
        `Click ignored: disabled=${disabled}, isMyTurn=${isMyTurn}, cellValue=${board[row]?.[col]}`
      );
      return;
    }
    console.log(`Board: Placing piece at (${row}, ${col})`);
    onPlacePiece(row, col);
  };

  const boardSize = board.length;

  return (
    <div
      // 親要素で幅が制御されるため、ここでは w-full を指定
      className={`board grid gap-0.5 border bg-background p-1 rounded-md shadow w-full ${
        disabled ? "opacity-50 cursor-not-allowed" : ""
      }`}
      style={{
        display: "grid",
        gridTemplateColumns: `repeat(${boardSize}, minmax(0, 1fr))`,
        aspectRatio: "1 / 1",
      }}
    >
      {board.map((row, rowIndex) =>
        row.map((cell, colIndex) => (
          <Cell
            key={`${rowIndex}-${colIndex}`}
            value={cell}
            isClickable={!disabled && isMyTurn && cell === 0}
            onClick={() => handleClick(rowIndex, colIndex)}
            // Cell に rowIndex, colIndex を渡す場合
            // rowIndex={rowIndex}
            // colIndex={colIndex}
          />
        ))
      )}
    </div>
  );
};

export default Board;
