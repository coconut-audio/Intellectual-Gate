#pragma once

#include <JuceHeader.h>
#include "../Processor.h"

using namespace std;

class LevelMeter : public Component
{
public:
    LevelMeter(Processor&);
    ~LevelMeter() override = default;

    void paint(Graphics&) override;
    void resized() override;
    void mouseDown(const MouseEvent&) override;
    void mouseDrag(const MouseEvent&) override;

    void fillRmsValues(float dryValue, float wetValue);

    static constexpr float minDecibels = -60.0f;
    static constexpr float maxDecibels = 36.0f;
    static constexpr int bufferSize = 256;

private:
    float yToDecibels(float y) const;
    float decibelsToY(float db) const;

    Processor& processor;

    Rectangle<float> componentBounds;

    ColourGradient levelGradient;
    ColourGradient radialGradient;

    vector<float> dryRmsValues;
    vector<float> wetRmsValues;

    static constexpr float lineWidth = 2.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LevelMeter)
};
