#pragma once

#include <JuceHeader.h>

class Compressor
{
public:
    Compressor() = default;

    void prepare(const dsp::ProcessSpec& spec);
    void process(dsp::AudioBlock<float>& block);
    void setThreshold(float thresholdDecibels);

    static constexpr float ratio = 8.0f;
    static constexpr float attackMilliseconds = 20.0f;
    static constexpr float releaseMilliseconds = 20.0f;

private:
    dsp::Compressor<float> compressor;
};
