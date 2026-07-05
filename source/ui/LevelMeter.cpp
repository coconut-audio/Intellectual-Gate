#include "LevelMeter.h"
#include "../common/Colours.h"

LevelMeter::LevelMeter(Processor& processorRef)
    : processor(processorRef)
{
    dryRmsValues.resize(bufferSize, minDecibels);
    wetRmsValues.resize(bufferSize, minDecibels);
}

float LevelMeter::yToDecibels(float y) const
{
    return jmap<float>(y, componentBounds.getBottom(), componentBounds.getY(), minDecibels, maxDecibels);
}

float LevelMeter::decibelsToY(float db) const
{
    return jmap<float>(db, minDecibels, maxDecibels, componentBounds.getBottom(), componentBounds.getY());
}

void LevelMeter::mouseDown(const MouseEvent& event)
{
    float decibels = yToDecibels(static_cast<float>(event.getPosition().getY()));
    decibels = jlimit(minDecibels, maxDecibels, decibels);
    auto& apvts = processor.getApvts();
    apvts.getParameter(ParamIDs::threshold)->setValueNotifyingHost(
        apvts.getParameter(ParamIDs::threshold)->convertTo0to1(decibels));
}

void LevelMeter::mouseDrag(const MouseEvent& event)
{
    float decibels = yToDecibels(static_cast<float>(event.getPosition().getY()));
    decibels = jlimit(minDecibels, maxDecibels, decibels);
    auto& apvts = processor.getApvts();
    apvts.getParameter(ParamIDs::threshold)->setValueNotifyingHost(
        apvts.getParameter(ParamIDs::threshold)->convertTo0to1(decibels));
}

void LevelMeter::paint(Graphics& g)
{
    g.setColour(Theme::surface);
    g.fillRect(componentBounds);

    for (int i = static_cast<int>(minDecibels) + 12; i < static_cast<int>(maxDecibels); i += 12)
    {
        float y = jmap<float>(static_cast<float>(i), minDecibels, maxDecibels, componentBounds.getBottom(), componentBounds.getY());
        g.setColour(i == 0 ? Theme::gridMajorLine() : Theme::gridMinorLine());
        g.drawLine(componentBounds.getX(), y, componentBounds.getRight(), y, 0.5f * lineWidth);
    }

    g.saveState();
    g.reduceClipRegion(componentBounds.toNearestInt());

    Path dryRmsPath;
    for (size_t i = 0; i < dryRmsValues.size() - 1; ++i)
    {
        float x = jmap<float>(static_cast<float>(i), 0.0f, static_cast<float>(dryRmsValues.size() - 1), componentBounds.getX(), componentBounds.getRight());
        float y = jmap<float>(dryRmsValues[i], minDecibels, maxDecibels, componentBounds.getBottom(), componentBounds.getY());
        i == 0 ? dryRmsPath.startNewSubPath(x, y) : dryRmsPath.lineTo(x, y);
    }

    g.setColour(Theme::indicator);
    g.strokePath(dryRmsPath, PathStrokeType(lineWidth / 2.0f));

    dryRmsPath.lineTo(componentBounds.getRight(), componentBounds.getBottom());
    dryRmsPath.lineTo(componentBounds.getX(), componentBounds.getBottom());
    dryRmsPath.closeSubPath();

    g.setColour(Theme::indicatorFill());
    g.fillPath(dryRmsPath);

    Path wetRmsPath;
    for (size_t i = 0; i < wetRmsValues.size() - 1; ++i)
    {
        float x = jmap<float>(static_cast<float>(i), 0.0f, static_cast<float>(wetRmsValues.size() - 1), componentBounds.getX(), componentBounds.getRight());
        float y = jmap<float>(wetRmsValues[i], minDecibels, maxDecibels, componentBounds.getBottom(), componentBounds.getY());
        i == 0 ? wetRmsPath.startNewSubPath(x, y) : wetRmsPath.lineTo(x, y);
    }

    g.setColour(Theme::indicator);
    g.strokePath(wetRmsPath, PathStrokeType(lineWidth / 2.0f));

    wetRmsPath.lineTo(componentBounds.getRight(), componentBounds.getBottom());
    wetRmsPath.lineTo(componentBounds.getX(), componentBounds.getBottom());
    wetRmsPath.closeSubPath();

    g.setGradientFill(levelGradient);
    g.fillPath(wetRmsPath);

    for (int i = static_cast<int>(minDecibels) + 12; i < static_cast<int>(maxDecibels); i += 12)
    {
        float y = jmap<float>(static_cast<float>(i), minDecibels, maxDecibels, componentBounds.getBottom(), componentBounds.getY());
        g.setColour(Theme::textDim);
        g.setFont(10.0f);
        g.drawFittedText(String(i) + " dB", Rectangle<int>(componentBounds.getX(), static_cast<int>(y) - 10, 50, 20), Justification::right, 1);
    }

    g.restoreState();

    float thresholdY = decibelsToY(processor.getApvts().getRawParameterValue(ParamIDs::threshold)->load());

    g.setColour(Theme::regionFill());
    g.fillRect(componentBounds.getX(), thresholdY, componentBounds.getWidth(), componentBounds.getBottom() - thresholdY);

    g.setGradientFill(radialGradient);
    g.drawLine(componentBounds.getX(), thresholdY, componentBounds.getRight(), thresholdY, lineWidth);
}

void LevelMeter::resized()
{
    componentBounds = getLocalBounds().toFloat();

    float y = jmap<float>(0.0f, minDecibels, maxDecibels, componentBounds.getBottom(), componentBounds.getY());
    levelGradient = ColourGradient(Theme::indicator.withAlpha(0.67f), componentBounds.getX(), componentBounds.getBottom(), Theme::indicator.withAlpha(0.27f), componentBounds.getX(), y, false);

    radialGradient = ColourGradient(Theme::indicator.withAlpha(0.0f), componentBounds.getX(), 0.0f, Theme::indicator.withAlpha(0.0f), componentBounds.getRight(), 0.0f, false);
    radialGradient.addColour(0.5f, Theme::indicator);
}

void LevelMeter::fillRmsValues(float dryValue, float wetValue)
{
    rotate(dryRmsValues.begin(), dryRmsValues.begin() + 1, dryRmsValues.end());
    dryRmsValues.back() = dryValue;

    rotate(wetRmsValues.begin(), wetRmsValues.begin() + 1, wetRmsValues.end());
    wetRmsValues.back() = wetValue;

    repaint();
}
