#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "MagentaRetroVST3Processor.h"

class MagentaRetroVST3Editor : public juce::AudioProcessorEditor
{
public:
    MagentaRetroVST3Editor (MagentaRetroVST3Processor& p);
    ~MagentaRetroVST3Editor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    MagentaRetroVST3Processor& processor;

    // Generation controls
    juce::Slider temperatureSlider;
    juce::Label temperatureLabel;
    juce::Slider topkSlider;
    juce::Label topkLabel;
    juce::Slider toppSlider;
    juce::Label toppLabel;

    // Retro FX controls
    juce::Slider virtualSrSlider;
    juce::Label virtualSrLabel;
    juce::Slider bitDepthSlider;
    juce::Label bitDepthLabel;
    juce::Slider lpfCutoffSlider;
    juce::Label lpfCutoffLabel;
    juce::Slider attackSlider;
    juce::Label attackLabel;
    juce::Slider decaySlider;
    juce::Label decayLabel;
    juce::Slider sustainSlider;
    juce::Label sustainLabel;
    juce::Slider monoMixSlider;
    juce::Label monoMixLabel;
    juce::Slider spcDelaySlider;
    juce::Label spcDelayLabel;
    juce::Slider spcFeedbackSlider;
    juce::Label spcFeedbackLabel;

    // Utility controls
    juce::Slider volumeSlider;
    juce::Label volumeLabel;
    juce::ToggleButton bypassButton;
    juce::ToggleButton muteButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MagentaRetroVST3Editor)
};
