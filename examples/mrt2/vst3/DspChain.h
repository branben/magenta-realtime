#pragma once

#include <array>
#include <atomic>
#include <cstddef>

/// Retro Soundfonts DSP Chain.
/// Processes interleaved stereo float buffers in-place.
/// Processing order: down-sample → bitcrush → LPF → envelope → mono-mix → SPC-delay.
/// All parameters are std::atomic<float> for lock-free access from any thread.

class DspChain {
public:
    DspChain();

    void setSampleRate(double sr) { sampleRate.store(sr, std::memory_order_relaxed); }
    void setParameters(
        float virtualSr, float bitDepth, float lpfCutoff,
        float attackMs, float decayMs, float sustainLevel,
        float monoMix, float spcDelayTime, float spcFeedback);

    void process(float* left, float* right, size_t count);

private:
    void downSample(float* left, float* right, size_t count);
    void bitCrush(float* left, float* right, size_t count);
    void lowPass(float* left, float* right, size_t count);
    void applyEnvelope(float* left, float* right, size_t count);
    void monoMix(float* left, float* right, size_t count);
    void spcDelay(float* left, float* right, size_t count);

    std::atomic<double> sampleRate{48000.0};

    // Parameters
    std::atomic<float> virtualSampleRate{22050.0f};
    std::atomic<float> bitDepth{8.0f};
    std::atomic<float> lpfCutoff{8000.0f};
    std::atomic<float> attackMs{20.0f};
    std::atomic<float> decayMs{150.0f};
    std::atomic<float> sustainLevel{0.1f};
    std::atomic<float> monoMixAmount{0.3f};
    std::atomic<float> spcDelayMs{120.0f};
    std::atomic<float> spcFeedbackAmount{0.25f};

    // Down-sampler state
    float dsPhase = 0.0f;
    float dsPrevL = 0.0f;
    float dsPrevR = 0.0f;

    // LPF state (1-pole IIR)
    float lpfStateL = 0.0f;
    float lpfStateR = 0.0f;

    // Envelope state
    float envLevel = 0.0f;
    int envStage = 0;

    // SPC delay line
    static constexpr size_t kMaxDelaySamples = 48000;
    std::array<float, kMaxDelaySamples> delayLineL{};
    std::array<float, kMaxDelaySamples> delayLineR{};
    size_t delayWritePos = 0;
    size_t delayLen = 0;
};
