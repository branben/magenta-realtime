#pragma once

#include "DspChain.h"
#include "InferenceEngine.h"
#include <juce_audio_processors/juce_audio_processors.h>

class MagentaRetroVST3Processor : public juce::AudioProcessor,
                                 public juce::AudioProcessorValueTreeState::Listener
{
public:
    MagentaRetroVST3Processor();
    ~MagentaRetroVST3Processor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "MagentaRetroVST3"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState parameters;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    InferenceEngine& getInferenceEngine() { return inferenceEngine; }
    DspChain& getDspChain() { return dspChain; }

private:
    void parameterChanged (const juce::String& parameterID, float newValue) override;
    void updateDspParameters();

    InferenceEngine inferenceEngine;
    DspChain dspChain;
    double currentSampleRate = 48000.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MagentaRetroVST3Processor)
};
