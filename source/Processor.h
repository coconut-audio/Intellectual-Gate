#pragma once

#include <JuceHeader.h>
#include "common/Parameters.h"
#include "dsp/Compressor.h"
#include "dsp/HighpassFilter.h"

class Processor final : public AudioProcessor, public AudioProcessorValueTreeState::Listener
{
public:
    static constexpr int fftOrder = 10;
    static constexpr int fftSize = 1 << fftOrder;

    Processor();
    ~Processor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(AudioBuffer<float>&, MidiBuffer&) override;
    using AudioProcessor::processBlock;

    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const String getProgramName(int index) override;
    void changeProgramName(int index, const String& newName) override;

    void getStateInformation(MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    float getRmsValue(bool isDry) const;
    AudioProcessorValueTreeState& getApvts() { return apvts; }

    bool isDryFftBlockReady() const { return nextDryFftBlockReady; }
    bool isWetFftBlockReady() const { return nextWetFftBlockReady; }
    const float* getDryFftData() const { return dryFftData; }
    const float* getWetFftData() const { return wetFftData; }
    void clearDryFftReady() { nextDryFftBlockReady = false; }
    void clearWetFftReady() { nextWetFftBlockReady = false; }

    void parameterChanged(const String& parameterID, float newValue) override;

private:
    AudioProcessorValueTreeState::ParameterLayout createParameters();

    void pushNextDrySampleIntoFifo(float sample);
    void pushNextWetSampleIntoFifo(float sample);
    void updateFilterCoefficients(float cutoff, double sampleRate);
    void setFilterCutoff(float cutoff);

    AudioProcessorValueTreeState apvts;

    float dryRmsValue = 0.0f;
    float wetRmsValue = 0.0f;

    Compressor compressor;

    static constexpr int numChannels = 2;
    HighpassFilter highpassFilters[numChannels];
    double currentSampleRate = 0.0;

    float dryFifo[fftSize] = {};
    float wetFifo[fftSize] = {};
    float dryFftData[2 * fftSize] = {};
    float wetFftData[2 * fftSize] = {};
    int dryFifoIndex = 0;
    int wetFifoIndex = 0;
    bool nextDryFftBlockReady = false;
    bool nextWetFftBlockReady = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Processor)
};
