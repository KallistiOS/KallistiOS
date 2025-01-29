/* KallistiOS ##version##

   examples/dreamcast/raylib/tetris/src/blocks/block.cpp
   Copyright (C) 2024 Cole Hall

*/

#include "block.h"
#include "../constants/constants.h"
#include "../constants/vmuIcons.h"

Block::Block(){
    cellSize = Constants::cellSize;
    rotationState = 0;
    colors = GetCellColors();
    rowOffset = 0;
    columnOffset = 0;
    vmuIcon = vmuNULL;
}

void Block::Draw(int offsetX, int offsetY){
    std::vector<Position> tiles = GetCellPositions();
    for(Position item: tiles){
        DrawRectangle(
            item.column * cellSize + offsetX, 
            item.row * cellSize + offsetY,
            cellSize - 1, 
            cellSize -1,
            colors[id]
        );
    }
}

void Block::Move(int rows, int columns){
    rowOffset += rows;
    columnOffset += columns;
}

void Block::Reset(){
    rowOffset = 0;
    columnOffset = (Constants::numCols / 2) - 1;
}

std::vector<Position> Block::GetCellPositions(){
    std::vector<Position> tiles = cells[rotationState];
    std::vector<Position> movedTiles;
    for(Position item: tiles){
        Position newPos = Position(item.row + rowOffset, item.column + columnOffset);
        movedTiles.push_back(newPos);
    }

    return movedTiles;
}

// Rotate clockwise
void Block::Rotate(){
    rotationState ++;
    if(rotationState == static_cast<int>(cells.size())){
        rotationState = 0;
    }
}

// Rotate counter clockwise
void Block::UndoRotation(){
    rotationState --;
    if(rotationState == -1){
        rotationState = cells.size() -1;
    }
}
