#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <atomic>
#include "Params.h"
#include "dsp/GridsSequencer.h"
#include "dsp/MarblesSequencer.h"
#include "dsp/VoiceEngine.h"
#include "dsp/ChannelStrip.h"
#include "dsp/FxDelay.h"
#include "dsp/FxReverb.h"
#include "presets/PresetManager.h"

namespace MutableDrums {

class MutableDrumsAudioProcessor : public juce::AudioProcessor {
public:
    static constexpr int kVoices = Params::kNumVoices;

    MutableDrumsAudioProcessor();
    ~MutableDrumsAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "MutableDrums"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }

    int getNumPrograms() override { return juce::jmax(1, static_cast<int>(presetManager_.getAllPresets().size())); }
    int getCurrentProgram() override { return presetManager_.getCurrentPresetIndex(); }
    void setCurrentProgram(int index) override { loadPreset(index); }
    const juce::String getProgramName(int index) override;
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // ---- Parameters / presets (call from the message thread)
    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts_; }
    juce::UndoManager& getUndoManager() { return undoManager_; }
    PresetManager& getPresetManager() { return presetManager_; }

    void loadPreset(int index);
    void applyPreset(const PresetData& p);
    PresetData captureState() const;
    juce::String getCurrentPresetName() const { return currentPresetName_; }
    void setCurrentPresetName(const juce::String& n) { currentPresetName_ = n; }

    // ---- GUI -> audio
    void triggerVoiceManual(int voiceIndex);
    std::atomic<int> selectedVoice{ 0 };

    // ---- Audio -> GUI (all lock-free)
    std::atomic<int> currentStep{ 0 };                   // Grids 0..31
    std::atomic<int> marblesStep{ 0 };                   // Marbles déjà-vu loop index
    std::array<std::atomic<uint32_t>, 3> gridsHitCount{};   // BD / SD / HH
    std::array<std::atomic<uint32_t>, 3> marblesHitCount{}; // t1 / t2 / t3
    std::array<std::atomic<float>, 3> marblesX{};
    std::array<std::atomic<uint32_t>, kVoices> voiceHitCount{};
    std::array<std::array<std::atomic<float>, NUM_MOD_DESTINATIONS>, kVoices> voiceMod{};
    std::atomic<float> effectiveGridsX{ 0.5f }, effectiveGridsY{ 0.5f };
    std::atomic<float> lfoValue{ 0.0f };
    std::atomic<bool> hostIsPlaying{ false };
    std::atomic<bool> clockRunning{ false };
    std::atomic<float> currentBpm{ 120.0f };
    std::atomic<float> delayTimeMs{ 375.0f };

    float getChannelPeak(int ch) const { return channelPeaks_[static_cast<size_t>(ch % kVoices)].load(); }
    float getChannelGR(int ch) const { return channelGRs_[static_cast<size_t>(ch % kVoices)].load(); }
    float getMasterPeakL() const { return masterPeakL_.load(); }
    float getMasterPeakR() const { return masterPeakR_.load(); }
    float getReturnPeak(int which) const { return which == 0 ? dlyPeak_.load() : revPeak_.load(); }

private:
    struct VoiceParams {
        std::atomic<float>* engine{}; std::atomic<float>* trig{}; std::atomic<float>* choke{}; std::atomic<float>* note{};
        std::atomic<float>* tune{}; std::atomic<float>* decay{}; std::atomic<float>* tone{}; std::atomic<float>* chr{};
        std::atomic<float>* level{}; std::atomic<float>* pan{}; std::atomic<float>* mute{}; std::atomic<float>* solo{};
        std::atomic<float>* send1{}; std::atomic<float>* send2{};
        std::atomic<float>* eqOn{}; std::atomic<float>* eqLoF{}; std::atomic<float>* eqLoG{}; std::atomic<float>* eqMidF{};
        std::atomic<float>* eqMidQ{}; std::atomic<float>* eqMidG{}; std::atomic<float>* eqHiF{}; std::atomic<float>* eqHiG{};
        std::atomic<float>* drvOn{}; std::atomic<float>* drive{}; std::atomic<float>* drvTone{}; std::atomic<float>* drvTrim{}; std::atomic<float>* drvMode{};
        std::atomic<float>* cmpOn{}; std::atomic<float>* cmpThr{}; std::atomic<float>* cmpRat{}; std::atomic<float>* cmpAtk{};
        std::atomic<float>* cmpRel{}; std::atomic<float>* cmpMak{};
        std::array<std::atomic<float>*, kNumModSlots> modSrc{}, modDst{}, modAmt{};
    };

    void cacheParameterPointers();
    void pushParametersToDsp();
    void fireGridsStep(long long absStep);
    void fireMarblesStep(long long absStep);
    void triggerVoice(int v, float velocity);
    float computeLfo(double beatPos);
    void setParam(const juce::String& id, float plainValue);

    juce::UndoManager undoManager_;
    juce::AudioProcessorValueTreeState apvts_;
    PresetManager presetManager_;
    juce::String currentPresetName_{ "Init" };

    GridsSequencer grids_;
    MarblesSequencer marbles_;
    std::array<VoiceEngine, kVoices> voices_;
    std::array<ChannelStrip, kVoices> channels_;
    FxDelay delay_;
    FxReverb reverb_;

    // Cached parameter pointers
    std::array<VoiceParams, kVoices> vp_{};
    std::atomic<float>* pRun_{}; std::atomic<float>* pHostSync_{}; std::atomic<float>* pBpm_{}; std::atomic<float>* pMaster_{};
    std::atomic<float>* pGX_{}; std::atomic<float>* pGY_{}; std::atomic<float>* pGChaos_{};
    std::array<std::atomic<float>*, 3> pGDens_{}, pGLen_{};
    std::atomic<float>* pGMode_{}; std::atomic<float>* pGSwing_{};
    std::atomic<float>* pMRate_{}; std::atomic<float>* pMJitter_{}; std::atomic<float>* pMBias_{};
    std::atomic<float>* pMDejaVu_{}; std::atomic<float>* pMLength_{}; std::atomic<float>* pMModel_{};
    std::atomic<float>* pModGX_{}; std::atomic<float>* pModGY_{}; std::atomic<float>* pModGChaos_{}; std::atomic<float>* pModGDens_{};
    std::atomic<float>* pLfoRate_{}; std::atomic<float>* pLfoShape_{};
    std::atomic<float>* pDSync_{}; std::atomic<float>* pDDiv_{}; std::atomic<float>* pDTime_{}; std::atomic<float>* pDFb_{};
    std::atomic<float>* pDTone_{}; std::atomic<float>* pDPing_{}; std::atomic<float>* pDTape_{}; std::atomic<float>* pDRet_{};
    std::atomic<float>* pRDecay_{}; std::atomic<float>* pRBright_{}; std::atomic<float>* pRPre_{}; std::atomic<float>* pRWidth_{}; std::atomic<float>* pRRet_{};

    // Clock state (positions in 16th notes)
    double sampleRate_{ 44100.0 };
    double clockPos_{ 0.0 };
    double expectedHostPos_{ -1.0 };
    long long nextGridsStep_{ 0 };
    long long nextMarblesStep_{ 0 };
    double marblesJitterDelay_{ 0.0 };
    double freeBeatPos_{ 0.0 };
    bool wasRunning_{ false };
    float bpm_{ 120.0f };
    float swing_{ 0.0f };
    float marblesRate_{ 1.0f };
    float marblesJitter_{ 0.0f };
    float lfo_{ 0.0f };
    float lfoSH_{ 0.0f };
    double lastLfoCycle_{ -1.0 };
    uint32_t rng_{ 0xC0FFEEu };

    std::array<int, kVoices> trigSource_{};
    std::array<int, kVoices> chokeGroup_{};
    std::array<int, kVoices> midiNote_{};

    float masterGain_{ 0.7f };
    float masterTarget_{ 0.7f };

    std::atomic<uint32_t> pendingManual_{ 0 };

    std::array<std::atomic<float>, kVoices> channelPeaks_{};
    std::array<std::atomic<float>, kVoices> channelGRs_{};
    std::atomic<float> masterPeakL_{ 0.0f }, masterPeakR_{ 0.0f }, dlyPeak_{ 0.0f }, revPeak_{ 0.0f };
    float dlyPeakHold_{ 0.0f }, revPeakHold_{ 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MutableDrumsAudioProcessor)
};

} // namespace MutableDrums
