#include "Evaluator.h"

namespace {
constexpr int dirs[4][2]={{1,0},{0,1},{1,1},{1,-1}};
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
}
Evaluator::Evaluator() = default;
int Evaluator::lineScore(const Board& board,int id,Stone color) const {
    int x=0,y=0,dx=1,dy=0;
    if(id<15) y=id;
    else if(id<30) { x=id-15; dx=0; dy=1; }
    else if(id<59) { int d=id-44; x=d>0?d:0; y=d<0?-d:0; dy=1; }
    else { int sum=id-59; x=sum>14?sum-14:0; y=sum<14?sum:14; dy=-1; }
    int8_t line[15]; int n=0;
    for(Point p(x,y);p.isValid();p=Point(p.x+dx,p.y+dy)) line[n++]=static_cast<int8_t>(board.at(p));
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
        for(int c=0;c<2;++c) {
            int updated=lineScore(board,id,c==0?Stone::Black:Stone::White);
            totals_[c]+=updated-scores_[id][c]; scores_[id][c]=updated;
        }
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
std::array<int,2> Evaluator::evaluateMovePair(const Board& board,Point p) {
    if(!p.isValid() || board.at(p)!=Stone::Empty) return {-1,-1};
    syncMoveCache(board);
    const int index=p.y*15+p.x;
    std::array<std::array<ShapeType,4>,2> shapes{};
    for(int d=0;d<4;++d) {
        if(!(moveValid_[index]&(1u<<d))) {
            int8_t line[9];
            for(int i=-4;i<=4;++i) {
                Point q(p.x+dirs[d][0]*i,p.y+dirs[d][1]*i);
                line[i+4]=q.isValid()?static_cast<int8_t>(board.at(q)):-1;
            }
            for(int c=0;c<2;++c) {
                line[4]=static_cast<int8_t>(c+1);
                moveShapes_[index][d][c]=pattern_.recognizeLine(line,static_cast<int8_t>(c+1));
            }
            moveValid_[index]|=static_cast<uint8_t>(1u<<d);
            ++moveStats_.lineScans;
        } else ++moveStats_.cacheHits;
        for(int c=0;c<2;++c) shapes[c][d]=moveShapes_[index][d][c];
    }
    return {combine(shapes[0]),combine(shapes[1])};
}
std::array<MoveThreat,2> Evaluator::moveThreats(const Board& board,Point p) {
    std::array<MoveThreat,2> result{};
    if(!p.isValid() || board.at(p)!=Stone::Empty) return result;
    evaluateMovePair(board,p);
    for(int c=0;c<2;++c) for(int d=0;d<4;++d) {
        const auto shape=moveShapes_[p.y*15+p.x][d][c];
        result[c].five |= shape==ShapeType::FIVE;
        result[c].fours += shape==ShapeType::FOUR || shape==ShapeType::BLOCKED_FOUR;
        result[c].openFours += shape==ShapeType::FOUR;
        result[c].threes += shape==ShapeType::THREE;
    }
    return result;
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
