#include "MagentaRetroVST3Editor.h"

static constexpr int kLabelWidth = 110;
static constexpr int kSliderHeight = 28;
static constexpr int kRowGap = 4;
static constexpr int kSectionGap = 16;
static constexpr int kToggleWidth = 80;

static void makeSlider (juce::Slider& s, juce::Label& l, const juce::String& name,
                        juce::AudioProcessorValueTreeState& params, const juce::String& paramID)
{
    s.setSliderStyle (juce::Slider::LinearHorizontal);
    s.setTextBoxStyle (juce::Slider::TextBoxRight, false, 55, 20);
    s.getValueObject().referTo (params.getParameterAsValue (paramID));
    l.setText (name, juce::dontSendNotification);
    l.attachToComponent (&s, true);
}

MagentaRetroVST3Editor::MagentaRetroVST3Editor (MagentaRetroVST3Processor& p)
    : juce::AudioProcessorEditor (&p), processor (p)
{
    auto& params = processor.parameters;

    makeSlider (volumeSlider, volumeLabel, "Volume", params, "volume");

    bypassButton.setButtonText ("Bypass");
    bypassButton.getToggleStateValue().referTo (params.getParameterAsValue ("bypass"));

    muteButton.setButtonText ("Mute");
    muteButton.getToggleStateValue().referTo (params.getParameterAsValue ("mute"));

    makeSlider (temperatureSlider, temperatureLabel, "Temperature", params, "temperature");
    makeSlider (topkSlider, topkLabel, "Top-K", params, "topk");
    makeSlider (toppSlider, toppLabel, "Top-P", params, "topp");

    makeSlider (virtualSrSlider, virtualSrLabel, "Virtual SR", params, "virtualSampleRate");
    makeSlider (bitDepthSlider, bitDepthLabel, "Bit Depth", params, "bitDepth");
    makeSlider (lpfCutoffSlider, lpfCutoffLabel, "LPF Cutoff", params, "lpfCutoff");
    makeSlider (attackSlider, attackLabel, "Attack", params, "attackMs");
    makeSlider (decaySlider, decayLabel, "Decay", params, "decayMs");
    makeSlider (sustainSlider, sustainLabel, "Sustain", params, "sustainLevel");
    makeSlider (monoMixSlider, monoMixLabel, "Mono Mix", params, "monoMix");
    makeSlider (spcDelaySlider, spcDelayLabel, "SPC Delay", params, "spcDelayMs");
    makeSlider (spcFeedbackSlider, spcFeedbackLabel, "SPC Feedback", params, "spcFeedback");

    setSize (420, 560);
}

void MagentaRetroVST3Editor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xFF2D2D35));
}

void MagentaRetroVST3Editor::resized()
{
    auto area = getLocalBounds().reduced (14);
    (void)area.removeFromTop (24);

    auto row = [&](juce::Slider& s)
    {
        auto r = area.removeFromTop (kSliderHeight);
        r.removeFromLeft (kLabelWidth);
        s.setBounds (r.reduced (2));
        area.removeFromTop (kRowGap);
    };

    {
        auto r = area.removeFromTop (28);
        bypassButton.setBounds (r.removeFromLeft (kToggleWidth));
        r.removeFromLeft (8);
        muteButton.setBounds (r.removeFromLeft (kToggleWidth));
        area.removeFromTop (6);
    }

    row (volumeSlider);
    area.removeFromTop (kSectionGap);

    row (temperatureSlider);
    row (topkSlider);
    row (toppSlider);
    area.removeFromTop (kSectionGap);

    row (virtualSrSlider);
    row (bitDepthSlider);
    row (lpfCutoffSlider);
    row (attackSlider);
    row (decaySlider);
    row (sustainSlider);
    row (monoMixSlider);
    row (spcDelaySlider);
    row (spcFeedbackSlider);
}
