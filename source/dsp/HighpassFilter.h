#pragma once

#include <JuceHeader.h>

class HighpassFilter
{
public:
    HighpassFilter() = default;

    void prepare(const dsp::ProcessSpec& spec);
    void process(dsp::AudioBlock<float>& block, int channel);
    void setCoefficients(float cutoffFrequency, double sampleRate);

    static constexpr int order = 4;
    static constexpr int numSections = order / 2;

private:
    dsp::IIR::Filter<float> sections[numSections];
};
