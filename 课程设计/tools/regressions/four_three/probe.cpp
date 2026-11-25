#include "ai/Search.h"
#include "cases.h"
#include <algorithm>
#include <exception>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace {
const auto& Fixture=FourThreeFixtures::Cases[0];
const auto& Prefix=Fixture.prefix;
constexpr int Depth=6;
const Point Losing=Fixture.losing, Attack=Fixture.attack, Block=Fixture.block, Fork=Fixture.fork;
bool inject=false, targetLayer=false;
int inserted=0, completed=0, originalScore=0, originalAlpha=0;
bool originalSearched=false, opponentMateInWindow=false;

void require(bool ok,const char* message) {
    if(!ok) throw std::runtime_error(message);
}

bool atTarget(const Board& board,int ply) {
    return targetLayer && ply==1 && board.getHistory().size()==std::size(Prefix)+1 &&
           board.getLastMove()==Losing;
}

std::vector<Point> winningPoints(const Board& board,Stone side) {
    Board trial=board;
    std::vector<Point> points;
    for(int y=0;y<15;++y) for(int x=0;x<15;++x) {
        const Point point{x,y};
        if(!trial.place(point,side)) continue;
        if(trial.checkWin()==side) points.push_back(point);
        require(trial.undo(),"winning-point check did not undo");
    }
    return points;
}

void place(Board& board,Point p) {
    require(board.place(p,board.getCurrentPlayer()),"illegal fixture move");
    require(board.checkWin()==Stone::Empty,"fixture reached five too early");
}

void verifyCertificate(Board board) {
    require(board.getCurrentPlayer()==Stone::White,"wrong root side");
    place(board,Losing);
    require(board.getCurrentPlayer()==Stone::Black,"wrong attacking side");
    require(winningPoints(board,Stone::Black).empty() && winningPoints(board,Stone::White).empty(),
            "unexpected immediate win before the attack");
    place(board,Attack);
    require(winningPoints(board,Stone::Black)==std::vector<Point>{Block} &&
            winningPoints(board,Stone::White).empty(),"four does not force the recorded block");
    place(board,Block);
    place(board,Fork);
    require(winningPoints(board,Stone::Black)==Fixture.ends &&
            winningPoints(board,Stone::White).empty(),"fork does not force a win");
}
}

void fourThreeArm(int depth,Point move) {
    targetLayer=depth==Depth && move==Losing;
}

void fourThreeValue(int value,int alpha) {
    if(!targetLayer) return;
    originalSearched=true;
    originalScore=value;
    originalAlpha=alpha;
}

void fourThreeInclude(const Board& board,int ply,std::vector<Point>& moves) {
    // 只补第六层原着之后这一手；随后照常做必堵过滤、排序和搜索。
    if(!inject || !atTarget(board,ply) || !board.isCandidate(Attack) ||
       std::find(moves.begin(),moves.end(),Attack)!=moves.end()) return;
    moves.push_back(Attack);
    ++inserted;
}

void fourThreeReply(const Board& board,int ply,Point move,int score,int alpha,int beta) {
    if(!atTarget(board,ply) || move!=Attack) return;
    ++completed;
    opponentMateInWindow=opponentMateInWindow ||
        (score>=SearchScore::MateThreshold && score>alpha && score<beta);
}

int main(int argc,char** argv) {
    try {
        require(argc==2,"expected off or on");
        const std::string_view mode=argv[1];
        require(mode=="off" || mode=="on","expected off or on");
        inject=mode=="on";
        Board root;
        for(Point p:Prefix) place(root,p);
        verifyCertificate(root);

        Board child=root;
        place(child,Losing);
        Evaluator evaluation;
        MoveGenerator generator(evaluation);
        const auto capped=generator.generateMoves(child,Stone::Black,12);
        const auto all=generator.generateMoves(child,Stone::Black,225);
        const auto it=std::find(all.begin(),all.end(),Attack);
        const auto shape=evaluation.analyzeMove(child,Attack);
        std::cout<<"candidate "<<(it==all.end()?0:it-all.begin()+1)<<' '<<capped.size()<<' '
                 <<(std::find(capped.begin(),capped.end(),Attack)!=capped.end())<<' '
                 <<int(shape[0].fours)<<' '<<int(shape[0].openFours)<<' '
                 <<int(shape[0].threes)<<' '<<int(shape[0].five)<<' '
                 <<int(shape[1].fours)<<' '<<int(shape[1].five)<<'\n';

        const auto hash=root.getHash();
        const auto history=root.getHistory();
        Evaluator searchEvaluation;
        MoveGenerator searchGenerator(searchEvaluation);
        Searcher search(searchEvaluation,searchGenerator);
        SearchOptions options;
        options.maxDepth=Depth;
        const auto result=search.search(root,root.getCurrentPlayer(),45000,nullptr,options);
        require(root.getHistory()==history && root.getHash()==hash && root.getCurrentPlayer()==Stone::White,
                "root board was changed");
        require(result.status==SearchStatus::Move && result.move.isValid() &&
                root.at(result.move)==Stone::Empty,"search returned no legal move");
        require(result.depth==Depth || (!inject && result.depth>0 &&
                result.score>=SearchScore::Mate-3),"search did not complete the requested depth");
        std::cout<<"root "<<result.move.x<<' '<<result.move.y<<' '<<result.depth<<' '<<result.score<<' '
                 <<result.nodes<<' '<<result.quiescenceNodes<<'\n';
        std::cout<<"branch "<<originalSearched<<' '<<originalScore<<' '<<originalAlpha<<' '
                 <<inserted<<' '<<completed<<' '<<opponentMateInWindow<<'\n';
        return 0;
    } catch(const std::exception& e) {
        std::cerr<<e.what()<<'\n';
        return 1;
    }
}
