#pragma once
#include "DistrhoUI.hpp"
#include "ParameterDefs.hpp"

START_NAMESPACE_DISTRHO

class SampleTowerUI : public UI
{
public:
    SampleTowerUI();

    void parameterChanged(uint32_t index, float value) override;
    void onNanoDisplay() override;
    bool onMouse(const MouseEvent& ev) override;
    bool onMotion(const MotionEvent& ev) override;

private:
    float params_[st::kParamCount] {};
    int   dragParam_ = -1;

    // Returns the parameter id whose control contains (x,y), or -1.
    int hitTest(double x, double y) const;
    // Draws a vertical slider (track + fill + handle + labels).
    void drawVerticalSlider(float x, float y, float w, float h, float norm,
                            const char* label, const char* valueStr);
    // Normalize/denormalize using st::rangeFor (UI cannot read Parameter metadata).
    float toNorm(uint32_t id, float v) const;
    float fromNorm(uint32_t id, float n) const;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SampleTowerUI)
};

END_NAMESPACE_DISTRHO
