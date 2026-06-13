#include "DspChain.h"

#include <algorithm>
#include <cmath>

DspChain::DspChain() {
    delayLineL.fill(0.0f);
    delayLineR.fill(0.0f);
}

void DspChain::setParameters(
    float virtualSr, float bitDepth, float lpfCutoff,
    float attackMs, float decayMs, float sustainLevel,
    float monoMix, float spcDelayTime, float spcFeedback)
{
    virtualSampleRate.store(virtualSr, std::memory_order_relaxed);
    this->bitDepth.store(bitDepth, std::memory_order_relaxed);
    this->lpfCutoff.store(lpfCutoff, std::memory_order_relaxed);
    this->attackMs.store(attackMs, std::memory_order_relaxed);
    this->decayMs.store(decayMs, std::memory_order_relaxed);
    this->sustainLevel.store(sustainLevel, std::memory_order_relaxed);
    monoMixAmount.store(monoMix, std::memory_order_relaxed);
    spcDelayMs.store(spcDelayTime, std::memory_order_relaxed);
    spcFeedbackAmount.store(spcFeedback, std::memory_order_relaxed);

    double sr = sampleRate.load(std::memory_order_relaxed);
    float delaySamples = spcDelayTime * 0.001f * static_cast<float>(sr);
    delayLen = static_cast<size_t>(std::min(delaySamples, static_cast<float>(kMaxDelaySamples - 1)));
    if (delayLen < 1) delayLen = 1;
}

void DspChain::process(float* left, float* right, size_t count) {
    downSample(left, right, count);
    bitCrush(left, right, count);
    lowPass(left, right, count);
    applyEnvelope(left, right, count);
    monoMix(left, right, count);
    spcDelay(left, right, count);
}

void DspChain::downSample(float* left, float* right, size_t count) {
    float vsr = virtualSampleRate.load(std::memory_order_relaxed);
    double sr = sampleRate.load(std::memory_order_relaxed);
    float ratio = static_cast<float>(vsr / sr);
    if (ratio >= 1.0f) return;

    for (size_t i = 0; i < count; ++i) {
        dsPhase += ratio;
        if (dsPhase >= 1.0f) {
            dsPhase -= 1.0f;
            dsPrevL = left[i];
            dsPrevR = right[i];
        }
        left[i] = dsPrevL;
        right[i] = dsPrevR;
    }
}

void DspChain::bitCrush(float* left, float* right, size_t count) {
    float bits = bitDepth.load(std::memory_order_relaxed);
    if (bits >= 16.0f) return;

    float levels = std::pow(2.0f, bits - 1.0f);
    for (size_t i = 0; i < count; ++i) {
        left[i] = std::round(left[i] * levels) / levels;
        right[i] = std::round(right[i] * levels) / levels;
    }
}

void DspChain::lowPass(float* left, float* right, size_t count) {
    float cutoff = lpfCutoff.load(std::memory_order_relaxed);
    double sr = sampleRate.load(std::memory_order_relaxed);
    float rc = 1.0f / (2.0f * 3.14159265f * cutoff);
    float dt = 1.0f / static_cast<float>(sr);
    float alpha = dt / (rc + dt);

    for (size_t i = 0; i < count; ++i) {
        lpfStateL += alpha * (left[i] - lpfStateL);
        lpfStateR += alpha * (right[i] - lpfStateR);
        left[i] = lpfStateL;
        right[i] = lpfStateR;
    }
}

void DspChain::applyEnvelope(float* left, float* right, size_t count) {
    float atk = attackMs.load(std::memory_order_relaxed);
    float dec = decayMs.load(std::memory_order_relaxed);
    float sus = sustainLevel.load(std::memory_order_relaxed);
    double sr = sampleRate.load(std::memory_order_relaxed);

    float atkRate = (atk > 0.0f) ? (1.0f / (atk * 0.001f * static_cast<float>(sr))) : 1.0f;
    float decRate = (dec > 0.0f) ? (1.0f / (dec * 0.001f * static_cast<float>(sr))) : 1.0f;

    for (size_t i = 0; i < count; ++i) {
        if (envStage == 0) {
            envLevel += atkRate;
            if (envLevel >= 1.0f) {
                envLevel = 1.0f;
                envStage = 1;
            }
        } else if (envStage == 1) {
            envLevel -= decRate * (1.0f - sus);
            if (envLevel <= sus) {
                envLevel = sus;
                envStage = 2;
            }
        }
        left[i] *= envLevel;
        right[i] *= envLevel;
    }
}

void DspChain::monoMix(float* left, float* right, size_t count) {
    float mix = monoMixAmount.load(std::memory_order_relaxed);
    if (mix <= 0.0f) return;

    for (size_t i = 0; i < count; ++i) {
        float mono = (left[i] + right[i]) * 0.5f;
        left[i] = left[i] * (1.0f - mix) + mono * mix;
        right[i] = right[i] * (1.0f - mix) + mono * mix;
    }
}

void DspChain::spcDelay(float* left, float* right, size_t count) {
    float fb = spcFeedbackAmount.load(std::memory_order_relaxed);
    if (delayLen < 2 || fb <= 0.0f) return;

    for (size_t i = 0; i < count; ++i) {
        size_t readPos = (delayWritePos + kMaxDelaySamples - delayLen) % kMaxDelaySamples;
        float delayedL = delayLineL[readPos];
        float delayedR = delayLineR[readPos];

        delayLineL[delayWritePos] = left[i] + delayedL * fb;
        delayLineR[delayWritePos] = right[i] + delayedR * fb;

        left[i] += delayedL;
        right[i] += delayedR;

        delayWritePos = (delayWritePos + 1) % kMaxDelaySamples;
    }
}
