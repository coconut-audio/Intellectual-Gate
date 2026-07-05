#include "SpectrumAnalyser.h"
#include "../common/Colours.h"

SpectrumAnalyser::SpectrumAnalyser(Processor& processorRef)
    : processor(processorRef)
{
}

float SpectrumAnalyser::frequencyToX(float frequency) const
{
    float proportion = log2(frequency / minFrequency) / log2(maxFrequency / minFrequency);
    return jmap<float>(proportion, componentBounds.getX(), componentBounds.getRight());
}

float SpectrumAnalyser::xToFrequency(float x) const
{
    float proportion = jmap<float>(x, componentBounds.getX(), componentBounds.getRight(), 0.0f, 1.0f);
    return minFrequency * pow(maxFrequency / minFrequency, proportion);
}

void SpectrumAnalyser::mouseDown(const MouseEvent& event)
{
    float frequency = xToFrequency(static_cast<float>(event.getPosition().getX()));
    frequency = jlimit(minFrequency, maxFrequency, frequency);
    auto& apvts = processor.getApvts();
    apvts.getParameter(ParamIDs::cutoff)->setValueNotifyingHost(
        apvts.getParameter(ParamIDs::cutoff)->convertTo0to1(frequency));
}

void SpectrumAnalyser::mouseDrag(const MouseEvent& event)
{
    float frequency = xToFrequency(static_cast<float>(event.getPosition().getX()));
    frequency = jlimit(minFrequency, maxFrequency, frequency);
    auto& apvts = processor.getApvts();
    apvts.getParameter(ParamIDs::cutoff)->setValueNotifyingHost(
        apvts.getParameter(ParamIDs::cutoff)->convertTo0to1(frequency));
}

void SpectrumAnalyser::paint(Graphics& g)
{
    g.setColour(Theme::surface);
    g.fillRect(componentBounds);

    for (int i = static_cast<int>(minDecibels) + 12; i < static_cast<int>(maxDecibels); i += 12)
    {
        float y = jmap<float>(static_cast<float>(i), minDecibels, maxDecibels, componentBounds.getBottom(), componentBounds.getY());
        g.setColour(i == 0 ? Theme::gridMajorLine() : Theme::gridMinorLine());
        g.drawLine(componentBounds.getX(), y, componentBounds.getRight(), y, 0.5f * lineWidth);
    }

    for (int frequency : gridFrequencies)
    {
        float x = frequencyToX(static_cast<float>(frequency));
        g.setColour(Theme::gridMinorLine());
        g.drawLine(x, componentBounds.getY(), x, componentBounds.getBottom(), 0.5f * lineWidth);
    }

    g.saveState();
    g.reduceClipRegion(componentBounds.toNearestInt());

    Path drySpectrumPath;
    for (int i = 0; i < scopeSize - 1; ++i)
    {
        float proportion = static_cast<float>(i) / static_cast<float>(scopeSize - 1);
        float freq = minFrequency * pow(maxFrequency / minFrequency, proportion);
        float x = frequencyToX(freq);
        float y = jmap<float>(drySmoothed[i], minDecibels, maxDecibels, componentBounds.getBottom(), componentBounds.getY());
        i == 0 ? drySpectrumPath.startNewSubPath(x, y) : drySpectrumPath.lineTo(x, y);
    }

    g.setColour(Theme::indicator);
    g.strokePath(drySpectrumPath, PathStrokeType(lineWidth));

    drySpectrumPath.lineTo(componentBounds.getRight(), componentBounds.getBottom());
    drySpectrumPath.lineTo(componentBounds.getX(), componentBounds.getBottom());
    drySpectrumPath.closeSubPath();

    g.setColour(Theme::indicatorFill());
    g.fillPath(drySpectrumPath);

    Path wetSpectrumPath;
    for (int i = 0; i < scopeSize - 1; ++i)
    {
        float proportion = static_cast<float>(i) / static_cast<float>(scopeSize - 1);
        float freq = minFrequency * pow(maxFrequency / minFrequency, proportion);
        float x = frequencyToX(freq);
        float y = jmap<float>(wetSmoothed[i], minDecibels, maxDecibels, componentBounds.getBottom(), componentBounds.getY());
        i == 0 ? wetSpectrumPath.startNewSubPath(x, y) : wetSpectrumPath.lineTo(x, y);
    }

    g.setColour(Theme::indicator);
    g.strokePath(wetSpectrumPath, PathStrokeType(lineWidth));

    wetSpectrumPath.lineTo(componentBounds.getRight(), componentBounds.getBottom());
    wetSpectrumPath.lineTo(componentBounds.getX(), componentBounds.getBottom());
    wetSpectrumPath.closeSubPath();

    g.setGradientFill(spectrumGradient);
    g.fillPath(wetSpectrumPath);

    for (int frequency : gridFrequencies)
    {
        float x = frequencyToX(static_cast<float>(frequency));
        g.setColour(Theme::textDim);
        g.setFont(10.0f);
        g.drawFittedText(String(frequency) + " Hz", Rectangle<int>(static_cast<int>(x) - 25, roundToInt(componentBounds.getBottom()) - 20, 50, 20), Justification::centred, 1);
    }

    g.restoreState();

    float cutoffX = frequencyToX(processor.getApvts().getRawParameterValue(ParamIDs::cutoff)->load());
    float cutoffY = jmap<float>(0.0f, minDecibels, maxDecibels, componentBounds.getBottom(), componentBounds.getY());

    ColourGradient cutoffGradient(Theme::indicator.withAlpha(0.0f), cutoffX - 20.0f, 0.0f, Theme::indicator.withAlpha(0.0f), componentBounds.getRight(), 0.0f, false);
    float totalDistance = componentBounds.getRight() - cutoffX + 20.0f;
    float proportion = 20.0f / totalDistance;
    cutoffGradient.addColour(proportion, Theme::indicator);

    Path cutoffPath;
    cutoffPath.startNewSubPath(cutoffX - 20.0f, componentBounds.getBottom());
    cutoffPath.quadraticTo(cutoffX - 20.0f, cutoffY + 40.0f, cutoffX, cutoffY);
    cutoffPath.lineTo(componentBounds.getRight(), cutoffY);

    Path cutoffRegionPath = cutoffPath;
    cutoffRegionPath.lineTo(componentBounds.getRight(), componentBounds.getBottom());
    cutoffRegionPath.closeSubPath();
    g.setColour(Theme::regionFill());
    g.fillPath(cutoffRegionPath);

    g.setGradientFill(cutoffGradient);
    g.strokePath(cutoffPath, PathStrokeType(lineWidth));
}

void SpectrumAnalyser::resized()
{
    componentBounds = getLocalBounds().toFloat();

    float y = jmap<float>(0.0f, minDecibels, maxDecibels, componentBounds.getBottom(), componentBounds.getY());
    spectrumGradient = ColourGradient(Theme::indicator.withAlpha(0.67f), componentBounds.getX(), componentBounds.getBottom(), Theme::indicator.withAlpha(0.27f), componentBounds.getX(), y, false);
}

void SpectrumAnalyser::applySavgolFilter(float* data, int size)
{
    for (int i = 0; i < size; ++i)
    {
        float sum = 0.0f;
        for (int j = -savgolHalfWindow; j <= savgolHalfWindow; ++j)
        {
        int index = jlimit(0, size - 1, i + j);
        sum += data[index] * savgolCoeffs[j + savgolHalfWindow];
        }
        data[i] = sum;
    }
}

void SpectrumAnalyser::updateSpectra(const float* dryFftData, const float* wetFftData, float dataSize)
{
    sampleRate = processor.getSampleRate();
    if (sampleRate <= 0)
        return;

    for (int i = 0; i < scopeSize; ++i)
    {
        float proportion = static_cast<float>(i) / static_cast<float>(scopeSize - 1);
        float frequency = minFrequency * pow(maxFrequency / minFrequency, proportion);
        float frequencyRatio = frequency / (static_cast<float>(sampleRate) / 2.0f);
        int fftDataIndex = jlimit<int>(0, static_cast<int>(dataSize / 2.0f - 1.0f), static_cast<int>(frequencyRatio * dataSize / 2.0f));

        float dryLevel = Decibels::gainToDecibels(dryFftData[fftDataIndex]) - Decibels::gainToDecibels(dataSize) + Decibels::gainToDecibels(512.0f) + static_cast<float>(i) * 0.05f;
        float wetLevel = Decibels::gainToDecibels(wetFftData[fftDataIndex]) - Decibels::gainToDecibels(dataSize) + Decibels::gainToDecibels(512.0f) + static_cast<float>(i) * 0.05f;

        dryScopeData[i] = 0.5f * jlimit(minDecibels, maxDecibels, dryLevel) + 0.5f * dryScopeData[i];
        wetScopeData[i] = 0.5f * jlimit(minDecibels, maxDecibels, wetLevel) + 0.5f * wetScopeData[i];
    }

    memcpy(drySmoothed, dryScopeData, sizeof(drySmoothed));
    memcpy(wetSmoothed, wetScopeData, sizeof(wetSmoothed));
    applySavgolFilter(drySmoothed, scopeSize);
    applySavgolFilter(wetSmoothed, scopeSize);

    repaint();
}
