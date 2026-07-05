#pragma once

#include <JuceHeader.h>
#include "Processor.h"
#include "ui/LevelMeter.h"
#include "ui/SpectrumAnalyser.h"

class Editor final : public AudioProcessorEditor, public Timer
{
public:
    explicit Editor(Processor&);
    ~Editor() override;

    void paint(Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    Processor& processorRef;

    static constexpr int interpolatedSize = 16000;
    dsp::FFT forwardFFT;
    dsp::WindowingFunction<float> window;
    LagrangeInterpolator dryInterpolator;
    LagrangeInterpolator wetInterpolator;
    float dryInterpolatedData[interpolatedSize] = {};
    float wetInterpolatedData[interpolatedSize] = {};

    LevelMeter levelMeter;
    SpectrumAnalyser spectrumAnalyser;
    std::unique_ptr<ComponentBoundsConstrainer> constrainer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Editor)
};
