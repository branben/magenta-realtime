#include "InferenceEngine.h"

#include <magentart/detail/autorelease_pool.h>

#include <cstring>
#include <mutex>
#include <sstream>

// Platform-specific path helpers
#if defined(__APPLE__)
#include <TargetConditionals.h>
#if TARGET_OS_MAC
#include <pwd.h>
#include <unistd.h>
#endif
#endif

static std::string getDefaultResourcesDir() {
#if defined(__APPLE__)
    const char* home = getenv("HOME");
    if (!home) {
        struct passwd* pw = getpwuid(getuid());
        if (pw) home = pw->pw_dir;
    }
    if (home) {
        return std::string(home) + "/Documents/Magenta/magenta-rt-v2/resources";
    }
#endif
    return "./resources";
}

static std::string getDefaultModelDir() {
#if defined(__APPLE__)
    const char* home = getenv("HOME");
    if (!home) {
        struct passwd* pw = getpwuid(getuid());
        if (pw) home = pw->pw_dir;
    }
    if (home) {
        return std::string(home) + "/Documents/Magenta/magenta-rt-v2/models/mrt2_base";
    }
#endif
    return "./models/mrt2_base";
}

InferenceEngine::InferenceEngine() {
    // Default blend weights: equal blend of first two prompts
    for (int i = 0; i < (int)magentart::core::kMaxPrompts; ++i) {
        float w = (i < 2) ? 0.5f : 0.0f;
        runner_.set_blend_weight(i, w);
    }
}

InferenceEngine::~InferenceEngine() {
    stop();
    unload();
}

bool InferenceEngine::initAssets(const char* resourcesDir) {
    std::string dir = resourcesDir ? resourcesDir : getDefaultResourcesDir();
    return runner_.init_assets(dir.c_str());
}

bool InferenceEngine::loadMusicCoCaModel(const char* resourcesDir, const char* subfolder) {
    std::string dir = resourcesDir ? resourcesDir : getDefaultResourcesDir();
    return runner_.load_musiccoca_model(dir.c_str(), subfolder);
}

bool InferenceEngine::loadModel(const char* mlxfnPath) {
    if (!mlxfnPath || mlxfnPath[0] == '\0') {
        // Try default model path
        std::string defaultDir = getDefaultModelDir();
        // Look for .mlxfn in the default directory
        std::string defaultPath = defaultDir + "/mrt2_base.mlxfn";
        return runner_.load_model(defaultPath.c_str());
    }
    return runner_.load_model(mlxfnPath);
}

void InferenceEngine::unload() {
    runner_.unload();
}

void InferenceEngine::start() {
    runner_.start();
}

void InferenceEngine::stop() {
    runner_.stop();
}

bool InferenceEngine::isLoaded() const {
    return runner_.is_loaded();
}

void InferenceEngine::setNoteOn(int note) {
    runner_.set_note_on(note);
}

void InferenceEngine::setNoteOff(int note) {
    runner_.set_note_off(note);
}

bool InferenceEngine::readAudio(float* destL, float* destR, size_t count, bool blocking) {
    return runner_.read_audio_stereo(destL, destR, count, blocking);
}

void InferenceEngine::setTemperature(float t) { runner_.set_temperature(t); }
float InferenceEngine::getTemperature() const { return runner_.get_temperature(); }
void InferenceEngine::setTopK(int k) { runner_.set_top_k(k); }
int InferenceEngine::getTopK() const { return runner_.get_top_k(); }
void InferenceEngine::setCfgMusicCoCa(float v) { runner_.set_cfg_musiccoca(v); }
float InferenceEngine::getCfgMusicCoCa() const { return runner_.get_cfg_musiccoca(); }
void InferenceEngine::setCfgNotes(float v) { runner_.set_cfg_notes(v); }
float InferenceEngine::getCfgNotes() const { return runner_.get_cfg_notes(); }
void InferenceEngine::setCfgDrums(float v) { runner_.set_cfg_drums(v); }
float InferenceEngine::getCfgDrums() const { return runner_.get_cfg_drums(); }
void InferenceEngine::setVolumeDb(float v) { runner_.set_volume_db(v); }
float InferenceEngine::getVolumeDb() const { return runner_.get_volume_db(); }
void InferenceEngine::setMute(bool m) { runner_.set_mute(m); }
bool InferenceEngine::getMute() const { return runner_.get_mute(); }
void InferenceEngine::setBypass(bool b) { runner_.set_bypass(b); }
bool InferenceEngine::getBypass() const { return runner_.get_bypass(); }
void InferenceEngine::setLatencyComp(bool c) { runner_.set_latency_comp(c); }
bool InferenceEngine::getLatencyComp() const { return runner_.get_latency_comp(); }
void InferenceEngine::setBufferSize(size_t cap) { runner_.set_buffer_size(cap); }
size_t InferenceEngine::getBufferSize() const { return runner_.get_buffer_size(); }
size_t InferenceEngine::getLatencySamples() const { return runner_.get_latency_samples(); }
void InferenceEngine::triggerReset() { runner_.trigger_reset(); }
void InferenceEngine::setDrumless(bool on) { runner_.set_drumless(on); }
bool InferenceEngine::getDrumless() const { return runner_.get_drumless(); }
void InferenceEngine::setMidiGateEnabled(bool e) { runner_.set_midi_gate_enabled(e); }
bool InferenceEngine::getMidiGateEnabled() const { return runner_.get_midi_gate_enabled(); }
void InferenceEngine::setOnsetMode(int mode) { runner_.set_onset_mode(mode); }
int InferenceEngine::getOnsetMode() const { return runner_.get_onset_mode(); }
void InferenceEngine::setSeedRotation(int r) { runner_.set_seed_rotation(r); }
int InferenceEngine::getSeedRotation() const { return runner_.get_seed_rotation(); }
void InferenceEngine::setBlendWeight(int i, float w) { runner_.set_blend_weight(i, w); }
float InferenceEngine::getBlendWeight(int i) const { return runner_.get_blend_weight(i); }
void InferenceEngine::setUnmaskWidth(int w) { runner_.set_unmask_width(w); }
int InferenceEngine::getUnmaskWidth() const { return runner_.get_unmask_width(); }

void InferenceEngine::setTextPrompt(const std::string& text) {
    runner_.set_text_prompt(text);
}

void InferenceEngine::setTextPrompts(const std::vector<std::string>& texts, const std::vector<float>& weights) {
    runner_.set_text_prompts(texts, weights);
}

bool InferenceEngine::saveState(const char* path) {
    return runner_.save_state(path);
}

bool InferenceEngine::loadState(const char* path) {
    return runner_.load_state(path);
}

magentart::core::EngineMetrics InferenceEngine::getMetrics() const {
    return runner_.get_metrics();
}
