#include "ai/AIEngine.h"
#include "ai/SearchText.h"
#include <charconv>
#include <chrono>
#include <fstream>
#include <string_view>

int main(int argc,char** argv) {
    if(argc!=4) return 2;
    const std::string_view budgetArg(argv[3]);
    int budget=0;
    const auto parsed=std::from_chars(budgetArg.data(),budgetArg.data()+budgetArg.size(),budget);
    if(parsed.ec!=std::errc{} || parsed.ptr!=budgetArg.data()+budgetArg.size() || budget<=0) return 2;
    std::ifstream input(argv[1]);
    if(!input) return 3;
    Board board;
    int x,y;
    while(input>>std::ws && !input.eof()) {
        if(!(input>>x>>y) || !board.place(Point(x,y),board.getCurrentPlayer())) return 3;
    }
    if(input.bad() || board.checkWin()!=Stone::Empty || board.getHistory().size()==225) return 4;
    AIEngine engine;
    const auto start=std::chrono::steady_clock::now();
    const auto result=engine.think(board,budget);
    const auto ms=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-start).count();
    if(result.status!=SearchStatus::Move || !result.move.isValid()) return 5;
    std::ofstream output(argv[2]);
    output<<result.move.x<<' '<<result.move.y<<' '<<result.depth<<' '<<result.nodes<<' '
          <<result.score<<' '<<searchReasonName(result.reason)<<' '<<searchStatusName(result.status)<<' '<<ms<<'\n';
    output.close();
    return output?0:6;
}
