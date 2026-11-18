#pragma once
#include "Search.h"

// 棋谱与对战日志使用的稳定 ASCII 名称；新增枚举值时同步这里。
inline const char* searchReasonName(SearchReason reason) {
    switch(reason) {
        case SearchReason::Normal: return "normal";
        case SearchReason::Opening: return "opening";
        case SearchReason::ImmediateWin: return "immediate_win";
        case SearchReason::ForcedDefense: return "forced_defense";
        case SearchReason::MultipleThreatLoss: return "multiple_threat_loss";
        case SearchReason::Fallback: return "fallback";
        case SearchReason::VCFProof: return "vcf_proof";
    }
    return "unknown";
}
inline const char* searchStatusName(SearchStatus status) {
    switch(status) {
        case SearchStatus::Unavailable: return "unavailable";
        case SearchStatus::Move: return "move";
        case SearchStatus::Terminal: return "terminal";
        case SearchStatus::Cancelled: return "cancelled";
        case SearchStatus::NoLegalMove: return "no_legal_move";
        case SearchStatus::Error: return "error";
    }
    return "unknown";
}
