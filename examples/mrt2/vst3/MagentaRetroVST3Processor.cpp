#include "MagentaRetroVST3Processor.h"
#include "MagentaRetroVST3Editor.h"

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MagentaRetroVST3Processor();
}

MagentaRetroVST3Processor::MagentaRetroVST3Processor()
    : juce::AudioProcessor (juce::AudioProcessor::BusesProperties()
        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    parameters.addParameterListener ("virtualSampleRate", this);
    parameters.addParameterListener ("bitDepth", this);
    parameters.addParameterListener ("lpfCutoff", this);
    parameters.addParameterListener ("attackMs", this);
    parameters.addParameterListener ("decayMs", this);
    parameters.addParameterListener ("sustainLevel", this);
    parameters.addParameterListener ("monoMix", this);
    parameters.addParameterListener ("spcDelayMs", this);
    parameters.addParameterListener ("spcFeedback", this);
}

MagentaRetroVST3Processor::~MagentaRetroVST3Processor() {}

void MagentaRetroVST3Processor::parameterChanged (const juce::String& id, float)
{
    if (id.startsWith ("virtualSr") || id.startsWith ("bitDepth") || id.startsWith ("lpfCut")
        || id.startsWith ("attack") || id.startsWith ("decay") || id.startsWith ("sustain")
        || id.startsWith ("monoMix") || id.startsWith ("spcDelay") || id.startsWith ("spcFeed"))
        updateDspParameters();
}

void MagentaRetroVST3Processor::updateDspParameters()
{
    dspChain.setParameters (
        *parameters.getRawParameterValue ("virtualSampleRate"),
        *parameters.getRawParameterValue ("bitDepth"),
        *parameters.getRawParameterValue ("lpfCutoff"),
        *parameters.getRawParameterValue ("attackMs"),
        *parameters.getRawParameterValue ("decayMs"),
        *parameters.getRawParameterValue ("sustainLevel"),
        *parameters.getRawParameterValue ("monoMix"),
        *parameters.getRawParameterValue ("spcDelayMs"),
        *parameters.getRawParameterValue ("spcFeedback"));
}

void MagentaRetroVST3Processor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (samplesPerBlock);
    currentSampleRate = sampleRate;

    dspChain.setSampleRate (sampleRate);
    updateDspParameters();

    inferenceEngine.initAssets (nullptr);
    inferenceEngine.loadMusicCoCaModel (nullptr);
    inferenceEngine.start ();
}

void MagentaRetroVST3Processor::releaseResources()
{
    inferenceEngine.stop ();
}

void MagentaRetroVST3Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    for (const auto metadata : midiMessages)
    {
        auto msg = metadata.getMessage();
        if (msg.isNoteOn())
            inferenceEngine.setNoteOn (msg.getNoteNumber());
        else if (msg.isNoteOff())
            inferenceEngine.setNoteOff (msg.getNoteNumber());
    }

    buffer.clear ();

    auto numSamples = buffer.getNumSamples();
    auto* outL = buffer.getWritePointer (0);
    auto* outR = buffer.getWritePointer (1);

    inferenceEngine.readAudio (outL, outR, static_cast<size_t>(numSamples));

    bool bypass = *parameters.getRawParameterValue ("bypass") > 0.5f;
    if (!bypass)
        dspChain.process (outL, outR, static_cast<size_t>(numSamples));
}

juce::AudioProcessorEditor* MagentaRetroVST3Processor::createEditor()
{
    return new MagentaRetroVST3Editor (*this);
}

void MagentaRetroVST3Processor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void MagentaRetroVST3Processor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName (parameters.state.getType()))
        parameters.replaceState (juce::ValueTree::fromXml (*xmlState));
}

juce::AudioProcessorValueTreeState::ParameterLayout MagentaRetroVST3Processor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // -- Generation parameters --
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "temperature", "Temperature",
        juce::NormalisableRange<float> (0.1f, 2.0f, 0.01f), 1.3f));

    params.push_back (std::make_unique<juce::AudioParameterInt> (
        "topk", "Top-K", 1, 1024, 40));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "topp", "Top-P",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.9f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "cfgmusiccoca", "Prompt Adherence",
        juce::NormalisableRange<float> (-1.0f, 7.0f, 0.1f), 3.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "cfgnotes", "Note Adherence",
        juce::NormalisableRange<float> (-1.0f, 7.0f, 0.1f), 1.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "cfgdrums", "Drums Adherence",
        juce::NormalisableRange<float> (-1.0f, 7.0f, 0.1f), 1.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "volume", "Volume",
        juce::NormalisableRange<float> (-60.0f, 12.0f, 0.1f), 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        "mute", "Mute", false));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        "bypass", "Bypass", false));

    params.push_back (std::make_unique<juce::AudioParameterInt> (
        "unmaskwidth", "Unmask Width", 0, 127, 0));

    params.push_back (std::make_unique<juce::AudioParameterInt> (
        "seedrotation", "Seed Rotation", 0, 1000, 0));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        "drumless", "Filter Drums", false));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        "midigate", "MIDI Gate", false));

    for (int i = 0; i < 6; ++i)
    {
        juce::String id = "weight_" + juce::String (i);
        juce::String name = "Weight " + juce::String (i);
        float defVal = (i < 2) ? 0.5f : 0.0f;
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            id, name,
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), defVal));
    }

    // -- Retro FX parameters --
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "virtualSampleRate", "Virtual Sample Rate",
        juce::NormalisableRange<float> (8000.0f, 24000.0f, 100.0f), 22050.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "bitDepth", "Bit Depth",
        juce::NormalisableRange<float> (4.0f, 16.0f, 0.5f), 8.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "lpfCutoff", "LPF Cutoff",
        juce::NormalisableRange<float> (4000.0f, 12000.0f, 100.0f), 8000.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "attackMs", "Attack",
        juce::NormalisableRange<float> (0.0f, 50.0f, 1.0f), 20.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "decayMs", "Decay",
        juce::NormalisableRange<float> (50.0f, 300.0f, 5.0f), 150.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "sustainLevel", "Sustain Level",
        juce::NormalisableRange<float> (0.0f, 0.3f, 0.01f), 0.1f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "monoMix", "Mono Mix",
        juce::NormalisableRange<float> (0.0f, 0.5f, 0.01f), 0.3f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "spcDelayMs", "SPC Delay",
        juce::NormalisableRange<float> (0.0f, 200.0f, 1.0f), 120.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "spcFeedback", "SPC Feedback",
        juce::NormalisableRange<float> (0.0f, 0.5f, 0.01f), 0.25f));

    return { params.begin(), params.end() };
}
