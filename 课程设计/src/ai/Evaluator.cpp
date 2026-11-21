#include "Evaluator.h"
#include <cassert>

namespace {
constexpr auto& dirs=Board::LandingDirections;
constexpr MoveAnalysis invalidMove{{{-1,false,0,0,0},{-1,false,0,0,0}}};

// 四条方向线相加时，每个棋形计数最多为 4，彼此不进位。
const auto shapeData=[] {
    std::array<std::uint64_t,static_cast<std::size_t>(ShapeType::FIVE)+1> data{};
    for(std::size_t i=0;i<data.size();++i) {
        const auto s=static_cast<ShapeType>(i);
        data[i]=static_cast<std::uint64_t>(PatternRecognizer::getScore(s));
        if(s==ShapeType::BLOCKED_FOUR || s==ShapeType::FOUR) data[i]+=1ULL<<32;
        if(s==ShapeType::FOUR) data[i]+=1ULL<<36;
        if(s==ShapeType::THREE) data[i]+=1ULL<<40;
        if(s==ShapeType::FIVE) data[i]+=1ULL<<44;
    }
    return data;
}();
LandingAnalysis unpack(std::uint64_t sum) {
    const bool five=(sum>>44)!=0;
    const auto fours=static_cast<std::uint8_t>((sum>>32)&15);
    const auto open=static_cast<std::uint8_t>((sum>>36)&15);
    const auto threes=static_cast<std::uint8_t>((sum>>40)&15);
    int score=five?ShapeScore::FIVE:static_cast<int>(sum&0xffffffffULL);
    if(!five) {
        if(fours>=2) score+=ShapeScore::DOUBLE_FOUR;
        else if(fours && threes) score+=ShapeScore::FOUR_THREE;
        else if(threes>=2) score+=ShapeScore::DOUBLE_THREE;
    }
    return {score,five,fours,open,threes};
}
int combine(const std::array<ShapeType,4>& shapes) {
    int score=0,fours=0,threes=0;
    for(ShapeType shape:shapes) {
        if(shape==ShapeType::FIVE) return ShapeScore::FIVE;
        if(shape==ShapeType::FOUR || shape==ShapeType::BLOCKED_FOUR) ++fours;
        if(shape==ShapeType::THREE) ++threes;
        score+=PatternRecognizer::getScore(shape);
    }
    if(fours>=2) score+=ShapeScore::DOUBLE_FOUR;
    else if(fours && threes) score+=ShapeScore::FOUR_THREE;
    else if(threes>=2) score+=ShapeScore::DOUBLE_THREE;
    return score;
}
int readEvalLine(const Board& board,int id,int8_t* line) {
    int x=0,y=0,dx=1,dy=0;
    if(id<15) y=id;
    else if(id<30) { x=id-15; dx=0; dy=1; }
    else if(id<59) { int d=id-44; x=d>0?d:0; y=d<0?-d:0; dy=1; }
    else { int sum=id-59; x=sum>14?sum-14:0; y=sum<14?sum:14; dy=-1; }
    int n=0;
    for(Point p(x,y);p.isValid();p=Point(p.x+dx,p.y+dy)) line[n++]=static_cast<int8_t>(board.at(p));
    return n;
}
}
Evaluator::Evaluator() = default;
int Evaluator::lineScore(const Board& board,int id,Stone color) const {
    int8_t line[15];
    const int n=readEvalLine(board,id,line);
    return pattern_.scoreLine(line,n,static_cast<int8_t>(color));
}
int Evaluator::evaluateColor(const Board& board,Stone color) const {
    int score=0;
    for(int id=0;id<88;++id) score+=lineScore(board,id,color);
    return score;
}
int Evaluator::evaluateFull(const Board& board,Stone color) const {
    return evaluateColor(board,color)-evaluateColor(board,color==Stone::Black?Stone::White:Stone::Black);
}
void Evaluator::syncMoveCache(const Board& board) {
    if(!moveCacheValid_ || moveHash_!=board.getHash()) {
        moveValid_.fill(0); moveHash_=board.getHash(); moveCacheValid_=true;
    }
}
void Evaluator::invalidateMoveCache(Point p) {
    for(int d=0;d<4;++d) for(int i=-4;i<=4;++i) {
        Point q(p.x+dirs[d][0]*i,p.y+dirs[d][1]*i);
        if(q.isValid()) moveValid_[q.y*15+q.x]&=static_cast<uint8_t>(~(1u<<d));
    }
}
void Evaluator::beginPosition(const Board& board) {
    undo_.clear(); totals_={0,0};
    for(int id=0;id<88;++id) for(int c=0;c<2;++c) {
        scores_[id][c]=lineScore(board,id,c==0?Stone::Black:Stone::White);
        totals_[c]+=scores_[id][c];
    }
    hash_=board.getHash(); valid_=true;
    syncMoveCache(board);
}
void Evaluator::makeMove(const Board& board,Point p) {
    Undo saved;
    saved.ids={p.y,15+p.x,44+p.x-p.y,59+p.x+p.y};
    saved.totals=totals_; saved.hash=hash_; saved.move=p;
    for(int k=0;k<4;++k) {
        int id=saved.ids[k]; saved.scores[k]=scores_[id];
        int8_t line[15];
        const int n=readEvalLine(board,id,line);
        // 两边评分共用本次落子后的盘面，避免重复取线和编码空位。
        const auto updated=pattern_.scoreLinePair(line,n);
        for(int c=0;c<2;++c) totals_[c]+=updated[c]-scores_[id][c];
        scores_[id]=updated;
    }
    if(!moveCacheValid_ || moveHash_!=hash_) moveValid_.fill(0);
    else invalidateMoveCache(p);
    moveHash_=board.getHash(); moveCacheValid_=true;
    undo_.push_back(saved); hash_=board.getHash();
}
void Evaluator::unmakeMove() {
    const Undo saved=undo_.back(); undo_.pop_back();
    for(int k=0;k<4;++k) scores_[saved.ids[k]]=saved.scores[k];
    totals_=saved.totals; hash_=saved.hash;
    invalidateMoveCache(saved.move); moveHash_=hash_;
}
int Evaluator::evaluate(const Board& board,Stone color) {
    if(!valid_ || hash_!=board.getHash()) beginPosition(board);
    int score=totals_[0]-totals_[1];
    return color==Stone::Black?score:-score;
}
const MoveAnalysis& Evaluator::analyzeMove(const Board& board,Point p) {
    if(!p.isValid() || board.at(p)!=Stone::Empty) return invalidMove;
    syncMoveCache(board);
    return analyzeEmptySynced(board,p);
}
const MoveAnalysis& Evaluator::analyzeEmptySynced(const Board& board,Point p) {
    const int index=p.y*15+p.x;
    if(moveValid_[index]==0xF) {
        moveStats_.cacheHits+=4;
        return moveAnalysis_[index];
    }
    for(int d=0;d<4;++d) {
        if(!(moveValid_[index]&(1u<<d))) {
            int8_t line[9];
            board.readLandingLine(p,d,line);
            moveShapes_[index][d]=pattern_.recognizeLandingLine(line);
            moveValid_[index]|=static_cast<uint8_t>(1u<<d);
            ++moveStats_.lineScans;
        } else ++moveStats_.cacheHits;
    }
    std::array<std::uint64_t,2> totals{};
    for(const auto& dir:moveShapes_[index]) {
        totals[0]+=shapeData[static_cast<std::size_t>(dir[0])];
        totals[1]+=shapeData[static_cast<std::size_t>(dir[1])];
    }
    return moveAnalysis_[index]={unpack(totals[0]),unpack(totals[1])};
}
std::vector<Point> Evaluator::collectCandidateScores(const Board& board,std::array<std::array<int,2>,225>& scores) {
    std::vector<Point> candidates;
    board.appendCandidates(candidates);
    if(candidates.empty()) return candidates;
    syncMoveCache(board);
    for(Point move:candidates) {
        assert(board.at(move)==Stone::Empty);
        const auto& analysis=analyzeEmptySynced(board,move);
        scores[move.y*15+move.x]={analysis[0].score,analysis[1].score};
    }
    return candidates;
}
std::array<int,2> Evaluator::evaluateMovePair(const Board& board,Point p) {
    const auto& a=analyzeMove(board,p);
    return {a[0].score,a[1].score};
}
std::array<MoveThreat,2> Evaluator::moveThreats(const Board& board,Point p) {
    const auto& a=analyzeMove(board,p);
    return {{{a[0].five,a[0].fours,a[0].openFours,a[0].threes},
             {a[1].five,a[1].fours,a[1].openFours,a[1].threes}}};
}
int Evaluator::evaluateMove(const Board& board,Point p,Stone color) {
    return evaluateMovePair(board,p)[color==Stone::Black?0:1];
}
int Evaluator::evaluateMoveReference(const Board& board,Point p,Stone color) const {
    if(!p.isValid() || board.at(p)!=Stone::Empty) return -1;
    std::array<ShapeType,4> shapes;
    for(int d=0;d<4;++d) {
        int8_t line[9];
        for(int i=-4;i<=4;++i) {
            Point q(p.x+dirs[d][0]*i,p.y+dirs[d][1]*i);
            line[i+4]=!q.isValid()?-1:i==0?static_cast<int8_t>(color):static_cast<int8_t>(board.at(q));
        }
        shapes[d]=pattern_.recognizeLine(line,static_cast<int8_t>(color));
    }
    return combine(shapes);
}
