#include "Pattern.h"
#include <algorithm>
#include <array>

namespace {
// Distinct empty squares completing five, restricted to windows containing required stones.
unsigned winningSquares(const int8_t* line, int n, unsigned required) {
    unsigned result=0;
    for (int s=0; s+5<=n; ++s) {
        unsigned stones=0, empty=0;
        int own=0;
        bool blocked=false;
        for (int i=s; i<s+5; ++i) {
            if (line[i]==1) { ++own; stones |= 1u<<i; }
            else if (line[i]==0) empty |= 1u<<i;
            else blocked=true;
        }
        if (!blocked && own==4 && (stones & required)==required) result |= empty;
    }
    return result;
}
int bits(unsigned v) { int n=0; for (;v;v&=v-1) ++n; return n; }
bool five(const int8_t* line, int n, unsigned required) {
    for (int s=0;s+5<=n;++s) {
        unsigned mask=0;
        for(int i=s;i<s+5;++i) if(line[i]==1) mask|=1u<<i;
        if(bits(mask)==5 && (mask&required)==required) return true;
    }
    return false;
}
ShapeType classify(std::array<int8_t,9> line) {
    if(line[4]!=1) return ShapeType::NONE;
    const unsigned center=1u<<4;
    if(five(line.data(),9,center)) return ShapeType::FIVE;
    int wins=bits(winningSquares(line.data(),9,center));
    if(wins>=2) return ShapeType::FOUR;
    if(wins==1) return ShapeType::BLOCKED_FOUR;
    bool sleepThree=false;
    for(int i=0;i<9;++i) if(line[i]==0) {
        line[i]=1;
        int next=bits(winningSquares(line.data(),9,center|(1u<<i)));
        line[i]=0;
        if(next>=2) return ShapeType::THREE;
        if(next==1) sleepThree=true;
    }
    if(sleepThree) return ShapeType::SLEEP_THREE;
    bool sleepTwo=false;
    for(int i=0;i<9;++i) if(line[i]==0) {
        line[i]=1;
        for(int j=0;j<9;++j) if(line[j]==0) {
            line[j]=1;
            int next=bits(winningSquares(line.data(),9,center|(1u<<i)|(1u<<j)));
            line[j]=0;
            if(next>=2) { line[i]=0; return ShapeType::TWO; }
            if(next==1) sleepTwo=true;
        }
        line[i]=0;
    }
    return sleepTwo ? ShapeType::SLEEP_TWO : ShapeType::NONE;
}
}

PatternRecognizer::PatternRecognizer() : shapeTable_(262144,255) {}

ShapeType PatternRecognizer::recognizeLine(const int8_t* line, int8_t color) const {
    std::array<int8_t,9> normalized{};
    unsigned code=0;
    for(int i=0;i<9;++i) {
        normalized[i]=line[i]==color ? 1 : line[i]==0 ? 0 : 2;
        code |= static_cast<unsigned>(normalized[i])<<(2*i);
    }
    uint8_t& cached=shapeTable_[code];
    if(cached==255) cached=static_cast<uint8_t>(classify(normalized));
    return static_cast<ShapeType>(cached);
}

int PatternRecognizer::getScore(ShapeType shape) {
    constexpr int scores[]={0,0,ShapeScore::SLEEP_TWO,ShapeScore::TWO,ShapeScore::SLEEP_THREE,
        ShapeScore::THREE,ShapeScore::BLOCKED_FOUR,ShapeScore::FOUR,ShapeScore::FIVE};
    return scores[static_cast<int>(shape)];
}

int PatternRecognizer::scoreLine(const int8_t* raw,int n,int8_t color) const {
    if(n<5) return 0;
    std::array<int8_t,15> line{};
    for(int i=0;i<n;++i) line[i]=raw[i]==color ? 1 : raw[i]==0 ? 0 : 2;
    struct Group { unsigned stones=0, ends=0; int count=0, score=0; };
    std::array<Group,15> groups{};
    int count=0;
    for(int s=0;s+5<=n;++s) {
        unsigned mask=0, empty=0;
        bool blocked=false;
        for(int i=s;i<s+5;++i) {
            if(line[i]==1) mask|=1u<<i;
            else if(line[i]==0) empty|=1u<<i;
            else blocked=true;
        }
        int stones=bits(mask);
        if(blocked || stones<2) continue;
        int g=0;
        while(g<count && groups[g].stones!=mask) ++g;
        if(g==count) { groups[g].stones=mask; groups[g].count=stones; ++count; }
        groups[g].ends |= empty;
    }
    for(int g=0;g<count;++g) {
        Group& v=groups[g];
        if(v.count==5) v.score=ShapeScore::FIVE;
        else if(v.count==4) v.score=bits(v.ends)>=2 ? ShapeScore::FOUR : ShapeScore::BLOCKED_FOUR;
        else if(v.count==3) {
            v.score=ShapeScore::SLEEP_THREE;
            for(int k=0;k<n;++k) if(line[k]==0) {
                line[k]=1;
                int wins=bits(winningSquares(line.data(),n,v.stones|(1u<<k)));
                line[k]=0;
                if(wins>=2) { v.score=ShapeScore::THREE; break; }
            }
        } else {
            int lo=0,hi=n-1;
            while(!(v.stones&(1u<<lo))) ++lo;
            while(!(v.stones&(1u<<hi))) --hi;
            bool open=lo>0 && hi+1<n && line[lo-1]==0 && line[hi+1]==0 && hi-lo<=2;
            v.score=open ? ShapeScore::TWO : ShapeScore::SLEEP_TWO;
        }
    }
    int total=0;
    for(int g=0;g<count;++g) {
        bool subset=false;
        for(int h=0;h<count;++h) if(groups[h].count>groups[g].count &&
            (groups[h].stones & groups[g].stones)==groups[g].stones) { subset=true; break; }
        if(!subset) total+=groups[g].score;
    }
    return total;
}
