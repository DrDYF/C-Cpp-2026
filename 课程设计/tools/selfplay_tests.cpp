#include "ai/AIEngine.h"
#include <iostream>
#include <stdexcept>
int main() {
    AIEngine engine;
    for(int game=0;game<5;++game) {
        Board board;
        // Different balanced openings avoid repeating exactly one deterministic game.
        board.place(Point(7,7),Stone::Black);
        board.place(Point(8+(game%2),6+(game%3)),Stone::White);
        while(board.checkWin()==Stone::Empty && board.getHistory().size()<225) {
            auto result=engine.think(board,20);
            if(!board.place(result.move,board.getCurrentPlayer())) throw std::runtime_error("illegal self-play move");
        }
        std::cout<<"Self-play "<<game+1<<" plies="<<board.getHistory().size()
                 <<" winner="<<static_cast<int>(board.checkWin())<<'\n';
    }
}
