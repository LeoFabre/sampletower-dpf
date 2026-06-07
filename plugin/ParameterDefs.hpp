#pragma once
#include <cstdint>

namespace st {

static constexpr int kNumSlots = 11;

enum ParamId : uint32_t {
    kParamSpeed = 0,        // 0.1 .. 2.0,  def 1.0
    kParamGain,            // -60 .. 0,    def 0.0
    kParamLowCutEnabled,   // bool,        def 1
    kParamLowCutFreq,      // 20 .. 1000,  def 30  (log)
    kParamLowCutSlope,     // 0 .. 3,      def 0
    kParamLoop1,           // bool x11,    def 0
    kParamCount = kParamLoop1 + kNumSlots   // = 16
};

// Range descriptor shared by plugin (initParameter) and UI (rangeFor).
struct ParamRange { float min, max, def; bool isBool; bool isLog; bool isInt; };

inline ParamRange rangeFor(uint32_t id) {
    switch (id) {
        case kParamSpeed:         return {0.1f, 2.0f, 1.0f, false, false, false};
        case kParamGain:          return {-60.f, 0.f, 0.f,  false, false, false};
        case kParamLowCutEnabled: return {0.f, 1.f, 1.f,    true,  false, false};
        case kParamLowCutFreq:    return {20.f, 1000.f, 30.f, false, true, false};
        case kParamLowCutSlope:   return {0.f, 3.f, 0.f,    false, false, true};
        default:                  return {0.f, 1.f, 0.f,    true,  false, false}; // loops
    }
}

} // namespace st
