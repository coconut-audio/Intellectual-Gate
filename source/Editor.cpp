#include "Editor.h"
#include "common/Colours.h"

Editor::Editor(Processor& proc)
    : AudioProcessorEditor(&proc)
    , processorRef(proc)
    , forwardFFT(processorRef.fftOrder)
    , window(1 << processorRef.fftOrder, dsp::WindowingFunction<float>::hann)
    , levelMeter(processorRef)
    , spectrumAnalyser(processorRef)
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
    float dryRmsLevel = jlimit(LevelMeter::minDecibels, LevelMeter::maxDecibels, processorRef.getRmsValue(true));
    float wetRmsLevel = jlimit(LevelMeter::minDecibels, LevelMeter::maxDecibels, processorRef.getRmsValue(false));
    levelMeter.fillRmsValues(dryRmsLevel, wetRmsLevel);

    if (processorRef.isDryFftBlockReady())
    {
        window.multiplyWithWindowingTable(const_cast<float*>(processorRef.getDryFftData()), processorRef.fftSize);
        forwardFFT.performFrequencyOnlyForwardTransform(const_cast<float*>(processorRef.getDryFftData()));
    }

    if (processorRef.isWetFftBlockReady())
    {
        window.multiplyWithWindowingTable(const_cast<float*>(processorRef.getWetFftData()), processorRef.fftSize);
        forwardFFT.performFrequencyOnlyForwardTransform(const_cast<float*>(processorRef.getWetFftData()));
    }

    dryInterpolator.process(static_cast<float>(processorRef.fftSize) / static_cast<float>(interpolatedSize), processorRef.getDryFftData(), dryInterpolatedData, interpolatedSize);
    wetInterpolator.process(static_cast<float>(processorRef.fftSize) / static_cast<float>(interpolatedSize), processorRef.getWetFftData(), wetInterpolatedData, interpolatedSize);

    spectrumAnalyser.updateSpectra(dryInterpolatedData, wetInterpolatedData, static_cast<float>(interpolatedSize));
    processorRef.clearDryFftReady();
    processorRef.clearWetFftReady();

    repaint();
}
