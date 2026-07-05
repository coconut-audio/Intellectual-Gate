#include "HighpassFilter.h"

void HighpassFilter::prepare(const dsp::ProcessSpec& spec)
{
    for (int i = 0; i < numSections; ++i)
        sections[i].prepare(spec);
}

void HighpassFilter::process(dsp::AudioBlock<float>& block, int channel)
{
    auto channelBlock = block.getSingleChannelBlock(static_cast<size_t>(channel));
    dsp::ProcessContextReplacing<float> context(channelBlock);

    for (int i = 0; i < numSections; ++i)
        sections[i].process(context);
}

void HighpassFilter::setCoefficients(float cutoffFrequency, double sampleRate)
{
    auto coefficients = dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(
        cutoffFrequency, sampleRate, order);

    for (int i = 0; i < numSections; ++i)
        sections[i].coefficients = coefficients.getUnchecked(i);
}
