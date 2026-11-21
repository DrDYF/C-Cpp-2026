#include "Board.h"
#include "Zobrist.h"
#include <cassert>

namespace {
constexpr auto buildLandingIndices() {
    constexpr int size=BoardConst::SIZE;
    constexpr int padding=BoardConst::PADDING;
    constexpr int stride=BoardConst::STRIDE;
    static_assert(padding>0 && stride*stride<=65536);
    std::array<std::array<std::array<uint16_t,9>,4>,size*size> table{};
    for(int y=0;y<size;++y) for(int x=0;x<size;++x) {
        auto& entries=table[y*size+x];
        for(int d=0;d<4;++d) for(int k=-4;k<=4;++k) {
            const int xx=x+k*Board::LandingDirections[d][0], yy=y+k*Board::LandingDirections[d][1];
            if(xx>=0 && xx<size && yy>=0 && yy<size)
                entries[d][k+4]=static_cast<uint16_t>((yy+padding)*stride+xx+padding);
        }
    }
    return table;
}
// 盘外坐标映射到 reset 后始终为 -1 的左上填充格。
constexpr auto landingIndices=buildLandingIndices();
}

Board::Board() : lastMove_(-1, -1), winner_(Stone::Empty) {
    reset();
}

void Board::reset() {
    cells_.fill(-1);
    for(int y=0;y<BoardConst::SIZE;++y)
        for(int x=0;x<BoardConst::SIZE;++x)
            cells_[idx(x,y)]=static_cast<int8_t>(Stone::Empty);
    history_.clear();
    hash_=0;
    neighborCounts_.fill(0);
    candidateBits_.fill(0);
    setCandidate(cellId(7,7),true);
    lastMove_=Point(-1,-1);
    winner_=Stone::Empty;
}

void Board::setCandidate(int id,bool enabled) {
    if(id<0 || id>=BoardConst::SIZE*BoardConst::SIZE) return;
    const uint64_t mask=1ULL<<(id&63);
    if(enabled) candidateBits_[id>>6]|=mask;
    else candidateBits_[id>>6]&=~mask;
}

void Board::updateCandidate(Point p) {
    if(!p.isValid()) return;
    const int id=cellId(p.x,p.y);
    if(at(p)!=Stone::Empty) { setCandidate(id,false); return; }
    if(history_.empty()) {
        setCandidate(id,id==cellId(7,7));
        return;
    }
    setCandidate(id,neighborCounts_[id]>0);
}

void Board::appendCandidates(std::vector<Point>& out) const {
    out.clear();
    out.reserve(225);
    forEachCandidate([&](Point p) { out.push_back(p); });
}

bool Board::isCandidate(Point p) const {
    return p.isValid() && candidateBit(cellId(p.x,p.y));
}

uint8_t Board::nearbyStoneCount(Point p) const {
    return p.isValid() ? neighborCounts_[cellId(p.x,p.y)] : 0;
}

bool Board::place(Point p,Stone color) {
    if(!p.isValid() || color==Stone::Empty || at(p)!=Stone::Empty || winner_!=Stone::Empty) return false;
    cells_[idx(p.x,p.y)]=static_cast<int8_t>(color);
    hash_^=Zobrist::getValue(p.x,p.y,color);
    history_.push_back(p);
    for(int dy=-BoardConst::CANDIDATE_RADIUS;dy<=BoardConst::CANDIDATE_RADIUS;++dy)
        for(int dx=-BoardConst::CANDIDATE_RADIUS;dx<=BoardConst::CANDIDATE_RADIUS;++dx) {
            if(dx==0 && dy==0) continue;
            Point q(p.x+dx,p.y+dy);
            if(!q.isValid()) continue;
            ++neighborCounts_[cellId(q.x,q.y)];
            updateCandidate(q);
        }
    updateCandidate(p);
    if(history_.size()==1) updateCandidate(Point(7,7));
    lastMove_=p;
    winner_=checkWin();
    return true;
}

bool Board::undo() {
    if(history_.empty()) return false;
    Point p=history_.back();
    history_.pop_back();
    hash_^=Zobrist::getValue(p.x,p.y,at(p));
    cells_[idx(p.x,p.y)]=static_cast<int8_t>(Stone::Empty);
    for(int dy=-BoardConst::CANDIDATE_RADIUS;dy<=BoardConst::CANDIDATE_RADIUS;++dy)
        for(int dx=-BoardConst::CANDIDATE_RADIUS;dx<=BoardConst::CANDIDATE_RADIUS;++dx) {
            if(dx==0 && dy==0) continue;
            Point q(p.x+dx,p.y+dy);
            if(!q.isValid()) continue;
            uint8_t& count=neighborCounts_[cellId(q.x,q.y)];
            if(count>0) --count;
            updateCandidate(q);
        }
    updateCandidate(p);
    if(history_.empty()) {
        candidateBits_.fill(0);
        setCandidate(cellId(7,7),true);
    }
    lastMove_=history_.empty()?Point(-1,-1):history_.back();
    winner_=checkWin();
    return true;
}

Stone Board::at(Point p) const {
    if(!p.isValid()) return Stone::Empty;
    return static_cast<Stone>(cells_[idx(p.x,p.y)]);
}

void Board::readLandingLine(Point center,int direction,int8_t* line) const {
    assert(center.isValid() && direction>=0 && direction<4);
    const auto& offsets=landingIndices[cellId(center.x,center.y)][direction];
    for(std::size_t i=0;i<offsets.size();++i) line[i]=cells_[offsets[i]];
}

Stone Board::getCurrentPlayer() const {
    return history_.size()%2==0?Stone::Black:Stone::White;
}

Stone Board::checkWin() const {
    if(history_.empty()) return Stone::Empty;
    const Point last=lastMove_;
    const Stone color=at(last);
    if(color==Stone::Empty) return Stone::Empty;
    const int dirs[4][2]={{1,0},{0,1},{1,1},{1,-1}};
    for(const auto& dir:dirs) if(checkLine(last,dir[0],dir[1],color)) return color;
    return Stone::Empty;
}

bool Board::checkLine(Point p,int dx,int dy,Stone color) const {
    int count=1;
    for(int i=1;i<5;++i) {
        Point q(p.x+dx*i,p.y+dy*i);
        if(!q.isValid() || at(q)!=color) break;
        ++count;
    }
    for(int i=1;i<5;++i) {
        Point q(p.x-dx*i,p.y-dy*i);
        if(!q.isValid() || at(q)!=color) break;
        ++count;
    }
    return count>=5;
}
