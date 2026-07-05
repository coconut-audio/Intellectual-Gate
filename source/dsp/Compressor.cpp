#include "Compressor.h"

void Compressor::prepare(const dsp::ProcessSpec& spec)
{
    compressor.prepare(spec);
    compressor.setRatio(ratio);
    compressor.setAttack(attackMilliseconds);
    compressor.setRelease(releaseMilliseconds);
}

void Compressor::process(dsp::AudioBlock<float>& block)
{
    dsp::ProcessContextReplacing<float> context(block);
    compressor.process(context);
}

void Compressor::setThreshold(float thresholdDecibels)
{
    compressor.setThreshold(thresholdDecibels);
}
