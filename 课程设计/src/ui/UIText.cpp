#include "UIText.h"
#include <algorithm>
#include <cmath>
#include <map>
#include <string>
#include <vector>

namespace {
std::map<int,Font> fonts;
std::vector<int> glyphs;
std::string fontPath;
const char* labels=u8"五子棋悔重开切换模式思考中提示鼠标点击落当前黑方白步数获胜！游戏人人对战机玩家执上次搜索深度节点评分必堵后静态一步成多路威胁败势取消旧正在等待无合法着满盘和局异常请总止连冲四证明";

bool loadSize(int size) {
    if(fonts.count(size)) return true;
    Font candidate=LoadFontEx(fontPath.c_str(),size,glyphs.data(),static_cast<int>(glyphs.size()));
    if(candidate.texture.id==0 || candidate.texture.id==GetFontDefault().texture.id) return false;
    bool complete=candidate.baseSize==size;
    for(int codepoint:glyphs) {
        int index=GetGlyphIndex(candidate,codepoint);
        if(index<0 || index>=candidate.glyphCount || candidate.glyphs[index].value!=codepoint) { complete=false; break; }
    }
    if(!complete) { UnloadFont(candidate); return false; }
    SetTextureFilter(candidate.texture,TEXTURE_FILTER_POINT);
    fonts.emplace(size,candidate);
    TraceLog(LOG_INFO,"UI FONT: native atlas %i px (%i glyphs)",size,candidate.glyphCount);
    return true;
}
Font selectFont(int size) {
    size=std::max(1,size);
    if(!fontPath.empty() && loadSize(size)) return fonts.at(size);
    return GetFontDefault();
}
}
bool UIText::load() {
    if(!fonts.empty()) return true;
    glyphs.clear();
    for(int c=32;c<=126;++c) glyphs.push_back(c);
    int count=0; int* decoded=LoadCodepoints(labels,&count);
    glyphs.insert(glyphs.end(),decoded,decoded+count); UnloadCodepoints(decoded);
    std::sort(glyphs.begin(),glyphs.end()); glyphs.erase(std::unique(glyphs.begin(),glyphs.end()),glyphs.end());
    const std::vector<std::string> paths={std::string(GetApplicationDirectory())+"assets/fonts/Font.ttf","assets/fonts/Font.ttf"};
    for(const auto& path:paths) {
        if(!FileExists(path.c_str())) continue;
        fontPath=path;
        bool ok=true;
        // Rasterize each UI size directly from outlines; no 12px bitmap enlargement.
        for(int size:{12,14,16,18,20,24,36}) if(!loadSize(size)) { ok=false; break; }
        if(ok) { TraceLog(LOG_INFO,"UI FONT: bundled Zpix loaded from %s",path.c_str()); return true; }
        for(auto& item:fonts) UnloadFont(item.second);
        fonts.clear(); fontPath.clear();
    }
    TraceLog(LOG_ERROR,"UI FONT: missing or incomplete assets/fonts/Font.ttf; keep assets beside the executable");
    return false;
}
void UIText::unload() {
    for(auto& item:fonts) UnloadFont(item.second);
    fonts.clear(); glyphs.clear(); fontPath.clear();
}
void UIText::draw(const char* text,int x,int y,int size,Color color) {
    const Font font=selectFont(size);
    DrawTextEx(font,text,{static_cast<float>(x),static_cast<float>(y)},static_cast<float>(std::max(1,size)),0.0f,color);
}
int UIText::measure(const char* text,int size) {
    const Font font=selectFont(size);
    return static_cast<int>(std::ceil(MeasureTextEx(font,text,static_cast<float>(std::max(1,size)),0.0f).x));
}
