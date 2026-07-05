#include "Processor.h"
#include "Editor.h"

using namespace std;

Processor::Processor()
    : AudioProcessor(BusesProperties()
      #if !JucePlugin_IsMidiEffect
        #if !JucePlugin_IsSynth
          .withInput("Input", AudioChannelSet::stereo(), true)
        #endif
          .withOutput("Output", AudioChannelSet::stereo(), true)
      #endif
      ),
      apvts(*this, nullptr, "PARAMETERS", createParameters())
{
    apvts.state = ValueTree("PARAMETERS");
    apvts.addParameterListener(ParamIDs::threshold, this);
    apvts.addParameterListener(ParamIDs::cutoff, this);
}

Processor::~Processor()
{
    apvts.removeParameterListener(ParamIDs::threshold, this);
    apvts.removeParameterListener(ParamIDs::cutoff, this);
}

const String Processor::getName() const
{
    return JucePlugin_Name;
}

bool Processor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool Processor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool Processor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double Processor::getTailLengthSeconds() const
{
    return 0.0;
}

int Processor::getNumPrograms()
{
    return 1;
}

int Processor::getCurrentProgram()
{
    return 0;
}

void Processor::setCurrentProgram(int index)
{
    ignoreUnused(index);
}

const String Processor::getProgramName(int index)
{
    ignoreUnused(index);
    return {};
}

void Processor::changeProgramName(int index, const String& newName)
{
    ignoreUnused(index, newName);
}

void Processor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<uint32>(samplesPerBlock);
    spec.numChannels = static_cast<uint32>(getTotalNumOutputChannels());

    compressor.prepare(spec);
    compressor.setThreshold(apvts.getRawParameterValue(ParamIDs::threshold)->load());

    for (int channel = 0; channel < numChannels; ++channel)
        highpassFilters[channel].prepare(spec);

    updateFilterCoefficients(apvts.getRawParameterValue(ParamIDs::cutoff)->load(), sampleRate);
}

void Processor::releaseResources()
{
}

bool Processor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    ignoreUnused(layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet() != AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
        return false;

   #if !JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}

void Processor::processBlock(AudioBuffer<float>& buffer, MidiBuffer&)
{
    ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    AudioSampleBuffer drySignalBuffer;
    drySignalBuffer.makeCopyOf(buffer);

    AudioSampleBuffer wetSignalBuffer;
    wetSignalBuffer.makeCopyOf(buffer);

    dsp::AudioBlock<float> compressorBlock(wetSignalBuffer);
    compressor.process(compressorBlock);

    dryRmsValue = Decibels::gainToDecibels(
        (buffer.getRMSLevel(0, 0, drySignalBuffer.getNumSamples())
       + drySignalBuffer.getRMSLevel(1, 0, drySignalBuffer.getNumSamples())) / 2.0f);

    wetRmsValue = Decibels::gainToDecibels(
        (wetSignalBuffer.getRMSLevel(0, 0, wetSignalBuffer.getNumSamples())
       + wetSignalBuffer.getRMSLevel(1, 0, wetSignalBuffer.getNumSamples())) / 2.0f);

    for (int sampleIndex = 0; sampleIndex < buffer.getNumSamples(); ++sampleIndex)
    {
        float drySample = 0.5f * drySignalBuffer.getSample(0, sampleIndex)
                        + 0.5f * drySignalBuffer.getSample(1, sampleIndex);
        pushNextDrySampleIntoFifo(drySample);
    }

    dsp::AudioBlock<float> wetBlock(wetSignalBuffer);
    for (int channel = 0; channel < numChannels; ++channel)
        highpassFilters[channel].process(wetBlock, channel);

    for (int sampleIndex = 0; sampleIndex < buffer.getNumSamples(); ++sampleIndex)
    {
        float wetSample = 0.5f * wetSignalBuffer.getSample(0, sampleIndex)
                        + 0.5f * wetSignalBuffer.getSample(1, sampleIndex);
        pushNextWetSampleIntoFifo(wetSample);
    }

    wetSignalBuffer.applyGain(-1.0f);
    wetSignalBuffer.addFrom(0, 0, drySignalBuffer, 0, 0, buffer.getNumSamples());
    wetSignalBuffer.addFrom(1, 0, drySignalBuffer, 1, 0, buffer.getNumSamples());

    buffer.makeCopyOf(wetSignalBuffer);
}

bool Processor::hasEditor() const
{
    return true;
}

AudioProcessorEditor* Processor::createEditor()
{
    return new Editor(*this);
}

void Processor::getStateInformation(MemoryBlock& destData)
{
    unique_ptr<XmlElement> params(apvts.state.createXml());
    copyXmlToBinary(*params, destData);
}

void Processor::setStateInformation(const void* data, int sizeInBytes)
{
    unique_ptr<XmlElement> params(getXmlFromBinary(data, sizeInBytes));

    if (params != nullptr)
    {
        if (params->hasTagName(apvts.state.getType()))
            apvts.replaceState(ValueTree::fromXml(*params));
    }
}

AudioProcessorValueTreeState::ParameterLayout Processor::createParameters()
{
    AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(make_unique<AudioParameterFloat>(
        ParamIDs::threshold, "Threshold",
        NormalisableRange<float>(-60.0f, 36.0f), 0.0f));

    layout.add(make_unique<AudioParameterFloat>(
        ParamIDs::cutoff, "Cutoff",
        NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.25f), 4000.0f));

    return layout;
}

float Processor::getRmsValue(bool isDry) const
{
    return isDry ? dryRmsValue : wetRmsValue;
}

void Processor::pushNextDrySampleIntoFifo(float sample)
{
    if (dryFifoIndex == fftSize)
    {
        if (!nextDryFftBlockReady)
        {
            zeromem(dryFftData, sizeof(dryFftData));
            memcpy(dryFftData, dryFifo, sizeof(dryFifo));
            nextDryFftBlockReady = true;
        }
        dryFifoIndex = 0;
    }

    dryFifo[dryFifoIndex++] = sample;
}

void Processor::pushNextWetSampleIntoFifo(float sample)
{
    if (wetFifoIndex == fftSize)
    {
        if (!nextWetFftBlockReady)
        {
            zeromem(wetFftData, sizeof(wetFftData));
            memcpy(wetFftData, wetFifo, sizeof(wetFifo));
            nextWetFftBlockReady = true;
        }
        wetFifoIndex = 0;
    }

    wetFifo[wetFifoIndex++] = sample;
}

void Processor::setFilterCutoff(float cutoff)
{
    if (currentSampleRate > 0.0)
        updateFilterCoefficients(cutoff, currentSampleRate);
}

void Processor::updateFilterCoefficients(float cutoff, double sampleRate)
{
    for (int channel = 0; channel < numChannels; ++channel)
        highpassFilters[channel].setCoefficients(cutoff, sampleRate);
}

void Processor::parameterChanged(const String& parameterID, float newValue)
{
    if (parameterID == ParamIDs::threshold)
        compressor.setThreshold(newValue);
    else if (parameterID == ParamIDs::cutoff)
        setFilterCutoff(newValue);
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Processor();
}
