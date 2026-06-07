#include "SampleTowerUI.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>

START_NAMESPACE_DISTRHO

using DGL_NAMESPACE::Color;

static constexpr uint kWidth = 600, kHeight = 320;

// ---- Fixed layout geometry ------------------------------------------------
// Vertical-slider tracks (Speed, Gain, LowCut freq, LowCut slope).
static constexpr float kSliderW   = 46.f;
static constexpr float kTrackTop  = 60.f;
static constexpr float kTrackH    = 180.f;
static constexpr float kHandleH   = 12.f;

static constexpr float kSpeedX    = 24.f;
static constexpr float kGainX     = 90.f;

// LowCut group.
static constexpr float kLowCutEnX = 170.f;  // enabled toggle (square)
static constexpr float kLowCutEnY = 60.f;
static constexpr float kToggleSz  = 28.f;
static constexpr float kFreqX     = 220.f;  // freq vertical slider
static constexpr float kSlopeX    = 286.f;  // slope vertical slider

// 11 Loop toggles: 3 columns x 4 rows grid on the right.
static constexpr float kLoopX0    = 360.f;
static constexpr float kLoopY0    = 60.f;
static constexpr float kLoopW     = 72.f;
static constexpr float kLoopH     = 38.f;
static constexpr float kLoopGapX  = 8.f;
static constexpr float kLoopGapY  = 8.f;
static constexpr int   kLoopCols  = 3;

// Returns the track rectangle for a vertical-slider parameter.
static void sliderTrack(uint32_t id, float& x, float& y, float& w, float& h)
{
    w = kSliderW; y = kTrackTop; h = kTrackH;
    switch (id) {
        case st::kParamSpeed:      x = kSpeedX; break;
        case st::kParamGain:       x = kGainX;  break;
        case st::kParamLowCutFreq: x = kFreqX;  break;
        case st::kParamLowCutSlope:x = kSlopeX; break;
        default:                   x = -1000.f; break; // not a slider
    }
}

// Returns the rectangle of the i-th loop toggle (0..kNumSlots-1).
static void loopRect(int i, float& x, float& y)
{
    const int col = i % kLoopCols;
    const int row = i / kLoopCols;
    x = kLoopX0 + col * (kLoopW + kLoopGapX);
    y = kLoopY0 + row * (kLoopH + kLoopGapY);
}

SampleTowerUI::SampleTowerUI() : UI(kWidth, kHeight) {
    for (uint32_t i = 0; i < st::kParamCount; ++i)
        params_[i] = st::rangeFor(i).def;
    setGeometryConstraints(kWidth, kHeight, true, true);
}

float SampleTowerUI::toNorm(uint32_t id, float v) const {
    const st::ParamRange r = st::rangeFor(id);
    if (r.max <= r.min) return 0.0f;
    if (r.isLog) {
        const float lo = std::log(r.min), hi = std::log(r.max);
        return std::clamp((std::log(std::max(v, r.min)) - lo) / (hi - lo), 0.f, 1.f);
    }
    return std::clamp((v - r.min) / (r.max - r.min), 0.f, 1.f);
}
float SampleTowerUI::fromNorm(uint32_t id, float n) const {
    const st::ParamRange r = st::rangeFor(id);
    n = std::clamp(n, 0.f, 1.f);
    float v;
    if (r.isLog) {
        const float lo = std::log(r.min), hi = std::log(r.max);
        v = std::exp(lo + n * (hi - lo));
    } else {
        v = r.min + n * (r.max - r.min);
    }
    if (r.isBool) v = (n >= 0.5f) ? 1.0f : 0.0f;
    if (r.isInt)  v = std::round(v);
    return v;
}

void SampleTowerUI::parameterChanged(uint32_t index, float value) {
    if (index < st::kParamCount) { params_[index] = value; repaint(); }
}

// ---- Drawing helpers ------------------------------------------------------
void SampleTowerUI::drawVerticalSlider(float x, float y, float w, float h,
                                       float norm, const char* label,
                                       const char* valueStr)
{
    // Track.
    beginPath();
    rect(x, y, w, h);
    fillColor(Color(13, 13, 18));
    fill();
    strokeColor(Color(128, 128, 128));
    stroke();

    // Fill from bottom up.
    const float fillH = std::clamp(norm, 0.f, 1.f) * (h - 2.f);
    beginPath();
    rect(x + 1.f, y + h - 1.f - fillH, w - 2.f, fillH);
    fillColor(Color(70, 120, 190));
    fill();

    // Handle.
    const float hy = y + (h - kHandleH) - std::clamp(norm, 0.f, 1.f) * (h - kHandleH);
    beginPath();
    rect(x - 2.f, hy, w + 4.f, kHandleH);
    fillColor(Color(230, 230, 235));
    fill();
    strokeColor(Color(90, 90, 96));
    stroke();

    // Labels.
    fontSize(11.f);
    textAlign(ALIGN_CENTER | ALIGN_BASELINE);
    fillColor(Color(217, 217, 217));
    text(x + w * 0.5f, y - 6.f, label, nullptr);
    text(x + w * 0.5f, y + h + 16.f, valueStr, nullptr);
}

void SampleTowerUI::onNanoDisplay() {
    // Background.
    beginPath(); rect(0, 0, getWidth(), getHeight());
    fillColor(Color(26, 26, 31)); fill();

    // Title.
    fontSize(18.f);
    textAlign(ALIGN_LEFT | ALIGN_BASELINE);
    fillColor(Color(217, 217, 217));
    text(20.f, 36.f, "SampleTower", nullptr);

    char buf[32];

    // --- Speed & Gain vertical sliders ---
    {
        float x, y, w, h;
        sliderTrack(st::kParamSpeed, x, y, w, h);
        std::snprintf(buf, sizeof(buf), "%.2fx", params_[st::kParamSpeed]);
        drawVerticalSlider(x, y, w, h, toNorm(st::kParamSpeed, params_[st::kParamSpeed]),
                           "Speed", buf);

        sliderTrack(st::kParamGain, x, y, w, h);
        std::snprintf(buf, sizeof(buf), "%.0fdB", params_[st::kParamGain]);
        drawVerticalSlider(x, y, w, h, toNorm(st::kParamGain, params_[st::kParamGain]),
                           "Gain", buf);
    }

    // --- LowCut group ---
    {
        fontSize(11.f);
        textAlign(ALIGN_LEFT | ALIGN_BASELINE);
        fillColor(Color(170, 170, 200));
        text(kLowCutEnX, 52.f, "LowCut", nullptr);

        // Enabled toggle (square).
        const bool en = params_[st::kParamLowCutEnabled] > 0.5f;
        beginPath();
        rect(kLowCutEnX, kLowCutEnY, kToggleSz, kToggleSz);
        fillColor(en ? Color(102, 204, 102) : Color(64, 64, 71)); fill();
        strokeColor(Color(153, 153, 153)); stroke();
        fontSize(10.f);
        textAlign(ALIGN_CENTER | ALIGN_BASELINE);
        fillColor(Color(217, 217, 217));
        text(kLowCutEnX + kToggleSz * 0.5f, kLowCutEnY + kToggleSz + 14.f,
             en ? "On" : "Off", nullptr);

        float x, y, w, h;
        sliderTrack(st::kParamLowCutFreq, x, y, w, h);
        std::snprintf(buf, sizeof(buf), "%.0fHz", params_[st::kParamLowCutFreq]);
        drawVerticalSlider(x, y, w, h,
                           toNorm(st::kParamLowCutFreq, params_[st::kParamLowCutFreq]),
                           "Freq", buf);

        sliderTrack(st::kParamLowCutSlope, x, y, w, h);
        std::snprintf(buf, sizeof(buf), "%d", (int) std::lround(params_[st::kParamLowCutSlope]));
        drawVerticalSlider(x, y, w, h,
                           toNorm(st::kParamLowCutSlope, params_[st::kParamLowCutSlope]),
                           "Slope", buf);
    }

    // --- 11 Loop toggles ---
    {
        fontSize(11.f);
        textAlign(ALIGN_LEFT | ALIGN_BASELINE);
        fillColor(Color(170, 170, 200));
        text(kLoopX0, 52.f, "Loop", nullptr);

        for (int i = 0; i < st::kNumSlots; ++i) {
            float lx, ly;
            loopRect(i, lx, ly);
            const uint32_t pid = st::kParamLoop1 + i;
            const bool on = params_[pid] > 0.5f;
            beginPath();
            rect(lx, ly, kLoopW, kLoopH);
            fillColor(on ? Color(102, 178, 102) : Color(46, 46, 56)); fill();
            strokeColor(Color(128, 128, 128)); stroke();

            std::snprintf(buf, sizeof(buf), "Slot %d", i + 1);
            fontSize(11.f);
            textAlign(ALIGN_CENTER | ALIGN_MIDDLE);
            fillColor(Color(225, 225, 225));
            text(lx + kLoopW * 0.5f, ly + kLoopH * 0.5f, buf, nullptr);
        }
    }
}

// ---- Hit-testing ----------------------------------------------------------
int SampleTowerUI::hitTest(double dx, double dy) const {
    const float mx = (float) dx, my = (float) dy;

    // Vertical sliders.
    const uint32_t sliders[] = {
        st::kParamSpeed, st::kParamGain, st::kParamLowCutFreq, st::kParamLowCutSlope
    };
    for (uint32_t id : sliders) {
        float x, y, w, h;
        sliderTrack(id, x, y, w, h);
        if (mx >= x - 2.f && mx <= x + w + 2.f && my >= y && my <= y + h)
            return (int) id;
    }

    // LowCut enabled toggle.
    if (mx >= kLowCutEnX && mx <= kLowCutEnX + kToggleSz &&
        my >= kLowCutEnY && my <= kLowCutEnY + kToggleSz)
        return (int) st::kParamLowCutEnabled;

    // 11 Loop toggles.
    for (int i = 0; i < st::kNumSlots; ++i) {
        float lx, ly;
        loopRect(i, lx, ly);
        if (mx >= lx && mx <= lx + kLoopW && my >= ly && my <= ly + kLoopH)
            return (int) (st::kParamLoop1 + i);
    }
    return -1;
}

bool SampleTowerUI::onMouse(const MouseEvent& ev) {
    if (ev.button != 1) return false;
    if (!ev.press) { dragParam_ = -1; return false; }
    const int id = hitTest(ev.pos.getX(), ev.pos.getY());
    if (id < 0) return false;
    const st::ParamRange r = st::rangeFor((uint32_t) id);
    if (r.isBool) {                                   // toggle on click
        params_[id] = params_[id] > 0.5f ? 0.0f : 1.0f;
        setParameterValue((uint32_t) id, params_[id]);
        repaint();
    } else {
        dragParam_ = id;                              // begin drag
        // Immediately seek to the click position on the track.
        float x, y, w, h;
        sliderTrack((uint32_t) id, x, y, w, h);
        const float norm = std::clamp((y + h - (float) ev.pos.getY()) / h, 0.f, 1.f);
        const float v = fromNorm((uint32_t) id, norm);
        params_[id] = v;
        setParameterValue((uint32_t) id, v);
        repaint();
    }
    return true;
}

bool SampleTowerUI::onMotion(const MotionEvent& ev) {
    if (dragParam_ < 0) return false;
    // Map vertical position within the control's track to a normalized value
    // (top of track = 1.0, bottom = 0.0).
    float x, y, w, h;
    sliderTrack((uint32_t) dragParam_, x, y, w, h);
    const float norm = std::clamp((y + h - (float) ev.pos.getY()) / h, 0.f, 1.f);
    const float v = fromNorm((uint32_t) dragParam_, norm);
    params_[dragParam_] = v;
    setParameterValue((uint32_t) dragParam_, v);
    repaint();
    return true;
}

UI* createUI() { return new SampleTowerUI(); }

END_NAMESPACE_DISTRHO
