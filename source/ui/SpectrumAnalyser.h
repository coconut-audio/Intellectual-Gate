#pragma once

#include <JuceHeader.h>
#include "../Processor.h"

using namespace std;

class SpectrumAnalyser : public Component
{
public:
    SpectrumAnalyser(Processor&);
    ~SpectrumAnalyser() override = default;

    void paint(Graphics&) override;
    void resized() override;
    void mouseDown(const MouseEvent&) override;
    void mouseDrag(const MouseEvent&) override;

    void updateSpectra(const float* dryData, const float* wetData, float dataSize);

    float frequencyToX(float frequency) const;
    float xToFrequency(float x) const;

    static constexpr float minDecibels = -60.0f;
    static constexpr float maxDecibels = 36.0f;
    static constexpr float minFrequency = 20.0f;
    static constexpr float maxFrequency = 20000.0f;

private:
    void applySavgolFilter(float* data, int size);

    Processor& processor;

    Rectangle<float> componentBounds;

    ColourGradient spectrumGradient;

    static constexpr int scopeSize = 512;
    float dryScopeData[scopeSize] = {};
    float wetScopeData[scopeSize] = {};
    float drySmoothed[scopeSize] = {};
    float wetSmoothed[scopeSize] = {};

    static constexpr int savgolWindow = 5;
    static constexpr int savgolHalfWindow = savgolWindow / 2;
    static constexpr float savgolNorm = 1.0f / 35.0f;
    static constexpr float savgolCoeffs[savgolWindow] = { -3.0f * savgolNorm, 12.0f * savgolNorm, 17.0f * savgolNorm, 12.0f * savgolNorm, -3.0f * savgolNorm };

    vector<int> gridFrequencies = { 50, 100, 200, 500, 1000, 2000, 5000, 10000 };
    static constexpr float lineWidth = 2.0f;

    double sampleRate = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumAnalyser)
};
