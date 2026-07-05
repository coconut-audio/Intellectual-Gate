#include <gtest/gtest.h>
#include "Processor.h"

class ProcessorTest : public ::testing::Test
{
protected:
    Processor processor;

    void prepareToPlay(double sampleRate = 44100.0, int blockSize = 512)
    {
        processor.prepareToPlay(sampleRate, blockSize);
    }

    AudioBuffer<float> processAudio(int numSamples = 512, float frequency = 1000.0f, float amplitude = 0.5f)
    {
        AudioBuffer<float> buffer(2, numSamples);
        for (int channel = 0; channel < 2; ++channel)
            for (int sample = 0; sample < numSamples; ++sample)
                buffer.setSample(channel, sample, amplitude * std::sin(2.0f * MathConstants<float>::pi * frequency * static_cast<float>(sample) / 44100.0f));

        MidiBuffer midi;
        processor.processBlock(buffer, midi);
        return buffer;
    }

    float bufferRms(const AudioBuffer<float>& buffer)
    {
        return (buffer.getRMSLevel(0, 0, buffer.getNumSamples())
              + buffer.getRMSLevel(1, 0, buffer.getNumSamples())) / 2.0f;
    }
};

TEST_F(ProcessorTest, ProducesNonSilentOutput)
{
    prepareToPlay();
    AudioBuffer<float> result = processAudio();
    EXPECT_GT(bufferRms(result), 0.0f);
}

TEST_F(ProcessorTest, HandlesDifferentSampleRates)
{
    prepareToPlay(44100.0, 512);
    AudioBuffer<float> at44k = processAudio();

    prepareToPlay(48000.0, 512);
    AudioBuffer<float> at48k = processAudio();

    EXPECT_GT(bufferRms(at44k), 0.0f);
    EXPECT_GT(bufferRms(at48k), 0.0f);
}

TEST_F(ProcessorTest, HandlesDifferentBlockSizes)
{
    prepareToPlay(44100.0, 128);
    AudioBuffer<float> smallBlock = processAudio(128);

    prepareToPlay(44100.0, 1024);
    AudioBuffer<float> largeBlock = processAudio(1024);

    EXPECT_GT(bufferRms(smallBlock), 0.0f);
    EXPECT_GT(bufferRms(largeBlock), 0.0f);
}

TEST_F(ProcessorTest, ThresholdAffectsCompressorResponse)
{
    prepareToPlay();

    processor.getApvts().getParameter(ParamIDs::threshold)->setValueNotifyingHost(0.0f);
    AudioBuffer<float> lowThreshold = processAudio();

    processor.getApvts().getParameter(ParamIDs::threshold)->setValueNotifyingHost(1.0f);
    AudioBuffer<float> highThreshold = processAudio();

    EXPECT_NE(bufferRms(lowThreshold), bufferRms(highThreshold));
}

TEST_F(ProcessorTest, CutoffAffectsFilterResponse)
{
    prepareToPlay();

    processor.getApvts().getParameter(ParamIDs::cutoff)->setValueNotifyingHost(0.0f);
    AudioBuffer<float> lowCutoff = processAudio();

    processor.getApvts().getParameter(ParamIDs::cutoff)->setValueNotifyingHost(1.0f);
    AudioBuffer<float> highCutoff = processAudio();

    EXPECT_NE(bufferRms(lowCutoff), bufferRms(highCutoff));
}

TEST_F(ProcessorTest, FftDataReadyAfterEnoughBlocks)
{
    prepareToPlay();

    for (int block = 0; block < 3; ++block)
        processAudio();

    EXPECT_TRUE(processor.isDryFftBlockReady());
    EXPECT_TRUE(processor.isWetFftBlockReady());
}

TEST_F(ProcessorTest, StateSaveAndLoad)
{
    prepareToPlay();

    processor.getApvts().getParameter(ParamIDs::threshold)->setValueNotifyingHost(0.75f);

    MemoryBlock destData;
    processor.getStateInformation(destData);
    EXPECT_GT(destData.getSize(), 0u);

    Processor newProcessor;
    newProcessor.prepareToPlay(44100.0, 512);
    newProcessor.setStateInformation(destData.getData(), static_cast<int>(destData.getSize()));

    auto savedXml = processor.getXmlFromBinary(destData.getData(), static_cast<int>(destData.getSize()));
    ASSERT_NE(savedXml, nullptr);
    EXPECT_TRUE(savedXml->hasTagName("PARAMETERS"));
}

TEST_F(ProcessorTest, ConsecutiveBlocksAreStable)
{
    prepareToPlay();

    for (int block = 0; block < 20; ++block)
    {
        AudioBuffer<float> result = processAudio();
        float rms = bufferRms(result);
        EXPECT_FALSE(std::isnan(rms));
        EXPECT_FALSE(std::isinf(rms));
    }
}

TEST_F(ProcessorTest, RmsValuesUpdateAfterProcessing)
{
    prepareToPlay();
    processAudio();

    EXPECT_NE(processor.getRmsValue(true), 0.0f);
    EXPECT_NE(processor.getRmsValue(false), 0.0f);
}

TEST_F(ProcessorTest, OutputIsMonoSumDifference)
{
    prepareToPlay();
    AudioBuffer<float> result = processAudio();

    for (int sample = 0; sample < result.getNumSamples(); ++sample)
        EXPECT_NEAR(result.getSample(0, sample), result.getSample(1, sample), 1e-6f);
}
