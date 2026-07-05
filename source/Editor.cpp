#include "Editor.h"
#include "common/Colours.h"

Editor::Editor(Processor& processorRef)
    : AudioProcessorEditor(&processorRef)
    , processor(processorRef)
    , forwardFFT(processor.fftOrder)
    , window(1 << processor.fftOrder, dsp::WindowingFunction<float>::hann)
    , levelMeter(processor)
    , spectrumAnalyser(processor)
    , constrainer(std::make_unique<ComponentBoundsConstrainer>())
{
    setSize(800, 420);
    setResizable(true, constrainer.get());
    setResizeLimits(600, 320, 1600, 900);
    startTimerHz(30);

    addAndMakeVisible(levelMeter);
    addAndMakeVisible(spectrumAnalyser);
}

Editor::~Editor()
{
    stopTimer();
}

void Editor::paint(Graphics& g)
{
    g.setColour(Theme::background);
    g.fillAll();
}

void Editor::resized()
{
    auto bounds = getLocalBounds().reduced(20);

    auto meterHeight = bounds.getHeight() / 2 - 10;
    levelMeter.setBounds(bounds.removeFromTop(meterHeight));

    bounds.removeFromTop(20);

    spectrumAnalyser.setBounds(bounds);
}

void Editor::timerCallback()
{
    float dryRmsLevel = jlimit(LevelMeter::minDecibels, LevelMeter::maxDecibels, processor.getRmsValue(true));
    float wetRmsLevel = jlimit(LevelMeter::minDecibels, LevelMeter::maxDecibels, processor.getRmsValue(false));
    levelMeter.fillRmsValues(dryRmsLevel, wetRmsLevel);

    if (processor.isDryFftBlockReady())
    {
        window.multiplyWithWindowingTable(const_cast<float*>(processor.getDryFftData()), processor.fftSize);
        forwardFFT.performFrequencyOnlyForwardTransform(const_cast<float*>(processor.getDryFftData()));
    }

    if (processor.isWetFftBlockReady())
    {
        window.multiplyWithWindowingTable(const_cast<float*>(processor.getWetFftData()), processor.fftSize);
        forwardFFT.performFrequencyOnlyForwardTransform(const_cast<float*>(processor.getWetFftData()));
    }

    dryInterpolator.process(static_cast<float>(processor.fftSize) / static_cast<float>(interpolatedSize), processor.getDryFftData(), dryInterpolatedData, interpolatedSize);
    wetInterpolator.process(static_cast<float>(processor.fftSize) / static_cast<float>(interpolatedSize), processor.getWetFftData(), wetInterpolatedData, interpolatedSize);

    spectrumAnalyser.updateSpectra(dryInterpolatedData, wetInterpolatedData, static_cast<float>(interpolatedSize));
    processor.clearDryFftReady();
    processor.clearWetFftReady();

    repaint();
}
