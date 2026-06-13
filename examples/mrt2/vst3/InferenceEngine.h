#pragma once

#include <magentart/realtime_runner.h>
#include <string>
#include <vector>

/// InferenceEngine — thin wrapper around magentart::core::RealtimeRunner
/// for the VST3 plugin. No JUCE dependency; all MIDI/audio handled by
/// the VST3 processor which calls into this class.
///
/// Thread safety:
///   - Lifecycle (initAssets, loadModel, start, stop): controller thread only
///   - MIDI (setNoteOn, setNoteOff): audio thread safe (forwards to RealtimeRunner atomics)
///   - Parameters (setTemperature, etc.): any thread (atomic)
///   - readAudio: audio thread only (forwards to RealtimeRunner::read_audio_stereo)

class InferenceEngine {
public:
    InferenceEngine();
    ~InferenceEngine();

    // ── Lifecycle (controller thread) ──────────────────────────────────────
    bool initAssets(const char* resourcesDir);
    bool loadMusicCoCaModel(const char* resourcesDir, const char* subfolder = "musiccoca");
    bool loadModel(const char* mlxfnPath);
    void unload();

    void start();
    void stop();

    bool isLoaded() const;

    // ── Audio thread ───────────────────────────────────────────────────────
    void setNoteOn(int note);
    void setNoteOff(int note);
    bool readAudio(float* destL, float* destR, size_t count, bool blocking = false);

    // ── Parameters (any thread, atomic) ────────────────────────────────────
    void setTemperature(float t);
    float getTemperature() const;
    void setTopK(int k);
    int getTopK() const;
    void setCfgMusicCoCa(float v);
    float getCfgMusicCoCa() const;
    void setCfgNotes(float v);
    float getCfgNotes() const;
    void setCfgDrums(float v);
    float getCfgDrums() const;
    void setVolumeDb(float v);
    float getVolumeDb() const;
    void setMute(bool m);
    bool getMute() const;
    void setBypass(bool b);
    bool getBypass() const;
    void setLatencyComp(bool c);
    bool getLatencyComp() const;
    void setBufferSize(size_t cap);
    size_t getBufferSize() const;
    size_t getLatencySamples() const;
    void triggerReset();
    void setDrumless(bool on);
    bool getDrumless() const;
    void setMidiGateEnabled(bool e);
    bool getMidiGateEnabled() const;
    void setOnsetMode(int mode);
    int getOnsetMode() const;
    void setSeedRotation(int r);
    int getSeedRotation() const;
    void setBlendWeight(int i, float w);
    float getBlendWeight(int i) const;
    void setUnmaskWidth(int w);
    int getUnmaskWidth() const;

    // ── Prompts ────────────────────────────────────────────────────────────
    void setTextPrompt(const std::string& text);
    void setTextPrompts(const std::vector<std::string>& texts, const std::vector<float>& weights);

    // ── State persistence ──────────────────────────────────────────────────
    bool saveState(const char* path);
    bool loadState(const char* path);

    // ── Metrics ────────────────────────────────────────────────────────────
    magentart::core::EngineMetrics getMetrics() const;

private:
    magentart::core::RealtimeRunner runner_;
};
