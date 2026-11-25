#pragma once
#include "core/Board.h"
#include <bitset>

namespace vcf_detail {
inline constexpr int Size=BoardConst::SIZE;
inline constexpr int Diagonals=2*Size-1;

struct Line {
    int x=0,y=0,dx=0,dy=0,length=0;
};

constexpr auto makeLines() {
    std::array<std::array<Line,Diagonals>,4> lines{};
    for(int i=0;i<Size;++i) {
        lines[0][i]={0,i,1,0,Size};
        lines[1][i]={i,0,0,1,Size};
    }
    for(int i=0;i<Diagonals;++i) {
        const int diff=i-(Size-1);
        lines[2][i]={diff<0?-diff:0,diff>0?diff:0,1,1,Size-(diff<0?-diff:diff)};
        const int x=i<Size?0:i-(Size-1);
        lines[3][i]={x,i-x,1,-1,x==0?i+1:Diagonals-i};
    }
    return lines;
}
inline constexpr auto Lines=makeLines();

class WinningPoints {
public:
    explicit WinningPoints(const Board& board) {
        for(Point p:board.getHistory()) update(p,board.at(p),true);
    }

    void update(Point p,Stone color,bool present) {
        const int ids[4]={p.y,p.x,p.y-p.x+Size-1,p.x+p.y};
        auto& side=masks_[index(color)];
        for(int d=0;d<4;++d) {
            const auto& line=Lines[d][ids[d]];
            const unsigned bit=1u<<(line.dx?p.x-line.x:p.y-line.y);
            auto& mask=side[d][ids[d]];
            mask=static_cast<uint16_t>(present?mask|bit:mask&~bit);
        }
    }

    std::bitset<Size*Size> find(Stone color) const {
        const int side=index(color);
        std::bitset<Size*Size> result;
        for(int d=0;d<4;++d) for(int i=0;i<(d<2?Size:Diagonals);++i) {
            const unsigned a=masks_[side][d][i];
            if(!a) continue;
            const unsigned b=a>>1,c=a>>2,e=a>>3,f=a>>4;
            const auto& line=Lines[d][i];
            if(line.length<5) continue;
            // 每项把四枚同色棋子对齐到窗口起点，再把缺口移回原位。
            unsigned holes=(b&c&e&f)|((a&c&e&f)<<1)|((a&b&e&f)<<2)|
                           ((a&b&c&f)<<3)|((a&b&c&e)<<4);
            holes&=((1u<<line.length)-1)&~(a|masks_[1-side][d][i]);
            for(int k=0;holes;++k,holes>>=1) {
                if(holes&1u) result.set((line.y+k*line.dy)*Size+line.x+k*line.dx);
            }
        }
        return result;
    }

private:
    static int index(Stone color) { return color==Stone::Black?0:1; }
    std::array<std::array<std::array<uint16_t,Diagonals>,4>,2> masks_{};
};
}
