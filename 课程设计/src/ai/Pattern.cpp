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
const std::array<uint8_t,6561>& shapeTable() {
    // 中心格恒为己子，索引只编码其余八格。
    static const auto table=[] {
        std::array<uint8_t,6561> out{};
        std::array<int8_t,9> line{};
        line[4]=1;
        for(unsigned code=0;code<out.size();++code) {
            unsigned digits=code;
            for(int i=0;i<9;++i) if(i!=4) {
                line[i]=static_cast<int8_t>(digits%3);
                digits/=3;
            }
            out[code]=static_cast<uint8_t>(classify(line));
        }
        return out;
    }();
    return table;
}
}

PatternRecognizer::PatternRecognizer() { (void)shapeTable(); }

ShapeType PatternRecognizer::recognizeLine(const int8_t* line,int8_t color) const {
    if(line[4]!=color) return ShapeType::NONE;
    constexpr unsigned weight[]={1,3,9,27,0,81,243,729,2187};
    unsigned code=0;
    for(int i=0;i<9;++i) if(i!=4) {
        const unsigned digit=line[i]==color ? 1 : line[i]==0 ? 0 : 2;
        code+=digit*weight[i];
    }
    return static_cast<ShapeType>(shapeTable()[code]);
}

std::array<ShapeType,2> PatternRecognizer::recognizeLandingLine(const int8_t* line) const {
    static constexpr auto packed=[] {
        std::array<uint32_t,256> digits{};
        for(unsigned i=0;i<digits.size();++i) {
            const unsigned black=i==1 ? 1 : i==0 ? 0 : 2;
            const unsigned white=i==2 ? 1 : i==0 ? 0 : 2;
            digits[i]=black|(white<<16);
        }
        return digits;
    }();
    constexpr unsigned weight[]={1,3,9,27,0,81,243,729,2187};
    uint32_t key=0;
    // 黑白索引至多 6560，累加不会在两个 16 位字段间进位。
    for(int i=0;i<9;++i) if(i!=4) key+=packed[static_cast<uint8_t>(line[i])]*weight[i];
    const auto& table=shapeTable();
    return {static_cast<ShapeType>(table[key&0xFFFFu]),static_cast<ShapeType>(table[key>>16])};
}

int PatternRecognizer::getScore(ShapeType shape) {
    constexpr int scores[]={0,0,ShapeScore::SLEEP_TWO,ShapeScore::TWO,ShapeScore::SLEEP_THREE,
        ShapeScore::THREE,ShapeScore::BLOCKED_FOUR,ShapeScore::FOUR,ShapeScore::FIVE};
    return scores[static_cast<int>(shape)];
}

int PatternRecognizer::scoreLine(const int8_t* raw,int n,int8_t color) const {
    if(n<5) return 0;
    unsigned own=0,empty=0;
    for(int i=0;i<n;++i) {
        if(raw[i]==color) own|=1u<<i;
        else if(raw[i]==0) empty|=1u<<i;
    }
    if((own & (own-1))==0) return 0;
    const unsigned blocked=~(own|empty);
    struct Group { unsigned stones,ends; int count; bool openThree; };
    std::array<Group,11> groups;
    int count=0;
    for(int start=0;start+5<=n;++start) {
        const unsigned window=31u<<start;
        if(window&blocked) continue;
        const unsigned stones=window&own;
        if((stones&(stones-1))==0) continue;
        const unsigned holes=window&empty;
        int g=0;
        while(g<count && groups[g].stones!=stones) ++g;
        if(g==count) groups[count++]={stones,holes,bits(stones),false};
        else {
            // 同一组三子的两个窗口若共享空点，在该点落子后有两个成五点。
            groups[g].openThree|=(groups[g].ends&holes)!=0;
            groups[g].ends|=holes;
        }
    }
    int total=0;
    for(int g=0;g<count;++g) {
        const auto& v=groups[g];
        bool subset=false;
        for(int h=0;h<count;++h) if(groups[h].count>v.count &&
            (groups[h].stones&v.stones)==v.stones) { subset=true; break; }
        if(subset) continue;
        if(v.count==5) total+=ShapeScore::FIVE;
        else if(v.count==4) total+=(v.ends&(v.ends-1))!=0 ? ShapeScore::FOUR : ShapeScore::BLOCKED_FOUR;
        else if(v.count==3) total+=v.openThree ? ShapeScore::THREE : ShapeScore::SLEEP_THREE;
        else {
            int lo=0,hi=n-1;
            while(!(v.stones&(1u<<lo))) ++lo;
            while(!(v.stones&(1u<<hi))) --hi;
            const bool open=lo>0 && hi+1<n && hi-lo<=2 && (empty&(1u<<(lo-1))) && (empty&(1u<<(hi+1)));
            total+=open ? ShapeScore::TWO : ShapeScore::SLEEP_TWO;
        }
    }
    return total;
}
