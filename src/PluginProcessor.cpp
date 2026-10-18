#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace MutableDrums {

namespace {
    // In the old preset format each engine read either "timbre" or "morph" as its 4th control.
    bool engineUsesTimbre(int e) {
        return e == ENGINE_PLAITS_ANALOG_SD || e == ENGINE_PLAITS_SYNTH_SD ||
               e == ENGINE_PLAITS_PARTICLE || e == ENGINE_BRAIDS_SNARE;
    }
    inline float dbToGain(float db) { return db <= -59.0f ? 0.0f : std::pow(10.0f, db / 20.0f); }
}

MutableDrumsAudioProcessor::MutableDrumsAudioProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts_(*this, &undoManager_, "MutableDrums", Params::createLayout()) {
    for (auto& a : channelPeaks_) a.store(0.0f);
    for (auto& a : channelGRs_) a.store(0.0f);
    for (auto& a : gridsHitCount) a.store(0);
    for (auto& a : marblesHitCount) a.store(0);
    for (auto& a : marblesX) a.store(0.5f);
    for (auto& a : voiceHitCount) a.store(0);
    for (auto& row : voiceMod) for (auto& a : row) a.store(0.0f);

    for (int i = 0; i < kVoices; ++i) voices_[static_cast<size_t>(i)].setSeed(static_cast<uint32_t>(i + 1));

    cacheParameterPointers();

    if (!presetManager_.getAllPresets().empty()) {
        applyPreset(presetManager_.getCurrentPreset());
        currentPresetName_ = presetManager_.getCurrentPreset().name;
    }
    undoManager_.clearUndoHistory();
}

// =====================================================================================
//  Parameter plumbing
// =====================================================================================
void MutableDrumsAudioProcessor::cacheParameterPointers() {
    using namespace Params;
    auto P = [this](const juce::String& id) {
        auto* p = apvts_.getRawParameterValue(id);
        jassert(p != nullptr);
        return p;
    };
    pRun_ = P(ID::run); pHostSync_ = P(ID::hostSync); pBpm_ = P(ID::bpm); pMaster_ = P(ID::master);
    pGX_ = P(ID::gridsX); pGY_ = P(ID::gridsY); pGChaos_ = P(ID::gridsChaos);
    pGDens_ = { P(ID::gridsDensBD), P(ID::gridsDensSD), P(ID::gridsDensHH) };
    pGLen_ = { P(ID::gridsLenBD), P(ID::gridsLenSD), P(ID::gridsLenHH) };
    pGMode_ = P(ID::gridsMode); pGSwing_ = P(ID::gridsSwing);
    pMRate_ = P(ID::marblesRate); pMJitter_ = P(ID::marblesJitter); pMBias_ = P(ID::marblesBias);
    pMDejaVu_ = P(ID::marblesDejaVu); pMLength_ = P(ID::marblesLength); pMModel_ = P(ID::marblesModel);
    pModGX_ = P(ID::modGridsX); pModGY_ = P(ID::modGridsY); pModGChaos_ = P(ID::modGridsChaos); pModGDens_ = P(ID::modGridsDens);
    pLfoRate_ = P(ID::lfoRate); pLfoShape_ = P(ID::lfoShape);
    pDSync_ = P(ID::dlySync); pDDiv_ = P(ID::dlyDiv); pDTime_ = P(ID::dlyTime); pDFb_ = P(ID::dlyFeedback);
    pDTone_ = P(ID::dlyTone); pDPing_ = P(ID::dlyPingPong); pDTape_ = P(ID::dlyTape); pDRet_ = P(ID::dlyReturn);
    pRDecay_ = P(ID::revDecay); pRBright_ = P(ID::revBright); pRPre_ = P(ID::revPreDelay); pRWidth_ = P(ID::revWidth); pRRet_ = P(ID::revReturn);

    for (int v = 0; v < kVoices; ++v) {
        auto& p = vp_[static_cast<size_t>(v)];
        auto Q = [&](const char* s) { return P(vid(v, s)); };
        p.engine = Q(V::engine); p.trig = Q(V::trig); p.choke = Q(V::choke); p.note = Q(V::note);
        p.tune = Q(V::tune); p.decay = Q(V::decay); p.tone = Q(V::tone); p.chr = Q(V::chr);
        p.level = Q(V::level); p.pan = Q(V::pan); p.mute = Q(V::mute); p.solo = Q(V::solo);
        p.send1 = Q(V::send1); p.send2 = Q(V::send2);
        p.eqOn = Q(V::eqOn); p.eqLoF = Q(V::eqLoF); p.eqLoG = Q(V::eqLoG); p.eqMidF = Q(V::eqMidF);
        p.eqMidQ = Q(V::eqMidQ); p.eqMidG = Q(V::eqMidG); p.eqHiF = Q(V::eqHiF); p.eqHiG = Q(V::eqHiG);
        p.drvOn = Q(V::drvOn); p.drive = Q(V::drive); p.drvTone = Q(V::drvTone); p.drvTrim = Q(V::drvTrim); p.drvMode = Q(V::drvMode);
        p.cmpOn = Q(V::cmpOn); p.cmpThr = Q(V::cmpThr); p.cmpRat = Q(V::cmpRat); p.cmpAtk = Q(V::cmpAtk);
        p.cmpRel = Q(V::cmpRel); p.cmpMak = Q(V::cmpMak);
        for (int s = 0; s < kNumModSlots; ++s) {
            p.modSrc[static_cast<size_t>(s)] = P(modSrcId(v, s));
            p.modDst[static_cast<size_t>(s)] = P(modDstId(v, s));
            p.modAmt[static_cast<size_t>(s)] = P(modAmtId(v, s));
        }
    }
}

void MutableDrumsAudioProcessor::pushParametersToDsp() {
    auto f = [](std::atomic<float>* a) { return a->load(std::memory_order_relaxed); };
    auto i = [](std::atomic<float>* a) { return static_cast<int>(std::lround(a->load(std::memory_order_relaxed))); };
    auto b = [](std::atomic<float>* a) { return a->load(std::memory_order_relaxed) > 0.5f; };

    grids_.setCoordinates(f(pGX_), f(pGY_));
    grids_.setChaos(f(pGChaos_));
    for (int k = 0; k < 3; ++k) {
        grids_.setDensity(k, f(pGDens_[static_cast<size_t>(k)]));
        grids_.setEuclideanLength(k, i(pGLen_[static_cast<size_t>(k)]));
    }
    grids_.setOutputMode(i(pGMode_) == 0 ? GridsSequencer::MODE_DRUMS : GridsSequencer::MODE_EUCLIDEAN);
    swing_ = f(pGSwing_);

    marblesRate_ = Params::marblesRateValue(i(pMRate_));
    marblesJitter_ = f(pMJitter_);
    marbles_.setRate(marblesRate_);
    marbles_.setJitter(marblesJitter_);
    marbles_.setBias(f(pMBias_));
    marbles_.setDejaVu(f(pMDejaVu_));
    marbles_.setLoopLength(i(pMLength_));
    marbles_.setModel(static_cast<MarblesSequencer::TModel>(juce::jlimit(0, 2, i(pMModel_))));

    delay_.setBpm(bpm_);
    delay_.setParameters(f(pDTime_), f(pDFb_), f(pDTone_), f(pDPing_), f(pDRet_), b(pDSync_), i(pDDiv_), f(pDTape_));
    reverb_.setParameters(f(pRDecay_), f(pRBright_), f(pRPre_), f(pRWidth_), f(pRRet_));
    masterTarget_ = dbToGain(f(pMaster_));

    bool anySolo = false;
    for (const auto& p : vp_) anySolo |= b(p.solo);

    for (int v = 0; v < kVoices; ++v) {
        const auto& p = vp_[static_cast<size_t>(v)];
        auto& voice = voices_[static_cast<size_t>(v)];
        auto& ch = channels_[static_cast<size_t>(v)];

        voice.setEngineType(i(p.engine));
        voice.setTune(f(p.tune));
        voice.setDecay(f(p.decay));
        voice.setTone(f(p.tone));
        voice.setCharacter(f(p.chr));
        for (int s = 0; s < kNumModSlots; ++s)
            voice.setModSlot(s, i(p.modSrc[static_cast<size_t>(s)]), i(p.modDst[static_cast<size_t>(s)]), f(p.modAmt[static_cast<size_t>(s)]));

        trigSource_[static_cast<size_t>(v)] = i(p.trig);
        chokeGroup_[static_cast<size_t>(v)] = i(p.choke);
        midiNote_[static_cast<size_t>(v)] = i(p.note);

        const bool audible = anySolo ? b(p.solo) : !b(p.mute);
        ch.setEQEnabled(b(p.eqOn));
        ch.setEQ(f(p.eqLoF), f(p.eqLoG), f(p.eqMidF), f(p.eqMidQ), f(p.eqMidG), f(p.eqHiF), f(p.eqHiG));
        ch.setDrive(b(p.drvOn), f(p.drive), f(p.drvTone), f(p.drvTrim), i(p.drvMode));
        ch.setCompressor(b(p.cmpOn), f(p.cmpThr), f(p.cmpRat), f(p.cmpAtk), f(p.cmpRel), f(p.cmpMak));
        ch.setMix(f(p.level), juce::jlimit(-1.0f, 1.0f, f(p.pan) + voice.getPanOffset()), f(p.send1), f(p.send2), audible);
    }
}

void MutableDrumsAudioProcessor::setParam(const juce::String& id, float plainValue) {
    if (auto* param = dynamic_cast<juce::RangedAudioParameter*>(apvts_.getParameter(id)))
        param->setValueNotifyingHost(param->convertTo0to1(plainValue));
}

// =====================================================================================
//  Presets
// =====================================================================================
const juce::String MutableDrumsAudioProcessor::getProgramName(int index) {
    const auto& presets = presetManager_.getAllPresets();
    if (index >= 0 && index < static_cast<int>(presets.size())) return presets[static_cast<size_t>(index)].name;
    return "Init";
}

void MutableDrumsAudioProcessor::loadPreset(int index) {
    const auto& presets = presetManager_.getAllPresets();
    if (index < 0 || index >= static_cast<int>(presets.size())) return;
    presetManager_.selectPreset(index);
    undoManager_.beginNewTransaction("Load preset");
    applyPreset(presetManager_.getCurrentPreset());
    currentPresetName_ = presetManager_.getCurrentPreset().name;
}

void MutableDrumsAudioProcessor::applyPreset(const PresetData& p) {
    using namespace Params;
    setParam(ID::hostSync, p.syncToHost ? 1.0f : 0.0f);
    if (p.bpm >= 30.0f && p.bpm <= 300.0f) setParam(ID::bpm, p.bpm);
    setParam(ID::master, p.masterVolumeDB);

    setParam(ID::gridsX, p.gridsX);
    setParam(ID::gridsY, p.gridsY);
    setParam(ID::gridsChaos, p.gridsChaos);
    setParam(ID::gridsDensBD, p.gridsDensity[0]);
    setParam(ID::gridsDensSD, p.gridsDensity[1]);
    setParam(ID::gridsDensHH, p.gridsDensity[2]);
    setParam(ID::gridsMode, static_cast<float>(p.gridsOutputMode == 0 ? 0 : 1));
    setParam(ID::gridsSwing, p.gridsSwing);
    setParam(ID::gridsLenBD, static_cast<float>(p.gridsEuclideanLength[0]));
    setParam(ID::gridsLenSD, static_cast<float>(p.gridsEuclideanLength[1]));
    setParam(ID::gridsLenHH, static_cast<float>(p.gridsEuclideanLength[2]));
    setParam(ID::modGridsX, p.gridsModMapX);
    setParam(ID::modGridsY, p.gridsModMapY);
    setParam(ID::modGridsChaos, p.gridsModChaos);
    setParam(ID::modGridsDens, p.gridsModDensity);

    setParam(ID::marblesRate, static_cast<float>(marblesRateIndexFor(p.marblesRate)));
    setParam(ID::marblesJitter, p.marblesJitter);
    setParam(ID::marblesBias, p.marblesBias);
    setParam(ID::marblesDejaVu, p.marblesDejaVu);
    setParam(ID::marblesLength, static_cast<float>(p.marblesLoopLength));
    setParam(ID::marblesModel, static_cast<float>(p.marblesModel));
    setParam(ID::lfoRate, static_cast<float>(p.lfoRate));
    setParam(ID::lfoShape, static_cast<float>(p.lfoShape));

    setParam(ID::dlySync, p.delaySync ? 1.0f : 0.0f);
    setParam(ID::dlyDiv, static_cast<float>(p.delayDivision));
    setParam(ID::dlyTime, p.delayTimeMs);
    setParam(ID::dlyFeedback, p.delayFeedback);
    setParam(ID::dlyTone, p.delayTone);
    setParam(ID::dlyPingPong, p.delayPingPong);
    setParam(ID::dlyTape, p.delayWarmth);
    setParam(ID::dlyReturn, p.delayReturnDB);

    setParam(ID::revDecay, p.reverbDecay);
    setParam(ID::revBright, p.reverbDamp);
    setParam(ID::revPreDelay, p.reverbPreDelayMs);
    setParam(ID::revWidth, p.reverbWidth);
    // v1 presets were balanced against the old reverb, which had ~+38 dB of internal gain
    // (that is what caused the runaway). The new reverb is calibrated; lift old returns a bit.
    setParam(ID::revReturn, p.formatVersion < 2 ? juce::jmin(6.0f, p.reverbReturnDB + 8.0f) : p.reverbReturnDB);

    for (int v = 0; v < kVoices; ++v) {
        const auto& c = p.channels[static_cast<size_t>(v)];
        auto S = [&](const char* suffix, float value) { setParam(vid(v, suffix), value); };
        S(V::engine, static_cast<float>(c.engine));
        S(V::trig, static_cast<float>(c.triggerSource == 7 ? 0 : c.triggerSource));
        S(V::choke, static_cast<float>(juce::jlimit(0, 4, c.chokeGroup)));
        S(V::note, static_cast<float>(c.midiNote >= 0 ? c.midiNote : kDefaultNotes[v]));
        S(V::tune, c.tune);
        S(V::decay, c.decay);
        S(V::tone, c.tone);
        S(V::chr, engineUsesTimbre(c.engine) ? c.timbre : c.morph);
        S(V::level, c.volumeDB);
        S(V::pan, c.pan);
        S(V::mute, 0.0f);
        S(V::solo, 0.0f);
        S(V::send1, c.aux1SendDB);
        S(V::send2, c.aux2SendDB);
        S(V::eqOn, c.eqBypass ? 0.0f : 1.0f);
        S(V::eqLoF, c.eqLowFreq);   S(V::eqLoG, c.eqLowGain);
        S(V::eqMidF, c.eqMidFreq);  S(V::eqMidQ, c.eqMidQ);  S(V::eqMidG, c.eqMidGain);
        S(V::eqHiF, c.eqHighFreq);  S(V::eqHiG, c.eqHighGain);
        S(V::drvOn, c.driveBypass ? 0.0f : 1.0f);
        S(V::drive, c.drive); S(V::drvTone, c.driveTone); S(V::drvTrim, c.driveTrimDB);
        S(V::drvMode, static_cast<float>(juce::jlimit(0, 2, c.driveMode)));
        // Old presets had the compressor always on with a gentle setting; keep that behaviour.
        S(V::cmpOn, c.compBypass ? 0.0f : 1.0f);
        S(V::cmpThr, c.compThresh); S(V::cmpRat, c.compRatio); S(V::cmpAtk, c.compAttackMs);
        S(V::cmpRel, c.compReleaseMs); S(V::cmpMak, c.compMakeupDB);

        const int srcs[3] = { c.modSource1, c.modSource2, c.modSource3 };
        const int dsts[3] = { c.modDest1, c.modDest2, c.modDest3 };
        const float amts[3] = { c.modAmount1, c.modAmount2, c.modAmount3 };
        for (int s = 0; s < kNumModSlots; ++s) {
            setParam(modSrcId(v, s), static_cast<float>(juce::jlimit(0, NUM_MOD_SOURCES - 1, srcs[s])));
            setParam(modDstId(v, s), static_cast<float>(juce::jlimit(0, NUM_MOD_DESTINATIONS - 1, dsts[s])));
            setParam(modAmtId(v, s), amts[s]);
        }
    }
}

PresetData MutableDrumsAudioProcessor::captureState() const {
    using namespace Params;
    auto get = [this](const juce::String& id) -> float {
        if (auto* param = dynamic_cast<juce::RangedAudioParameter*>(apvts_.getParameter(id)))
            return param->convertFrom0to1(param->getValue());
        return 0.0f;
    };
    auto geti = [&](const juce::String& id) { return static_cast<int>(std::lround(get(id))); };

    PresetData p;
    p.name = currentPresetName_.toStdString();
    p.category = "User";
    p.author = "User";
    p.isFactory = false;
    p.formatVersion = 2;

    p.syncToHost = get(ID::hostSync) > 0.5f;
    p.bpm = get(ID::bpm);
    p.masterVolumeDB = get(ID::master);
    p.gridsX = get(ID::gridsX); p.gridsY = get(ID::gridsY); p.gridsChaos = get(ID::gridsChaos);
    p.gridsDensity[0] = get(ID::gridsDensBD); p.gridsDensity[1] = get(ID::gridsDensSD); p.gridsDensity[2] = get(ID::gridsDensHH);
    p.gridsOutputMode = geti(ID::gridsMode);
    p.gridsSwing = get(ID::gridsSwing);
    p.gridsEuclideanLength[0] = geti(ID::gridsLenBD); p.gridsEuclideanLength[1] = geti(ID::gridsLenSD); p.gridsEuclideanLength[2] = geti(ID::gridsLenHH);
    p.gridsModMapX = get(ID::modGridsX); p.gridsModMapY = get(ID::modGridsY);
    p.gridsModChaos = get(ID::modGridsChaos); p.gridsModDensity = get(ID::modGridsDens);
    p.marblesRate = marblesRateValue(geti(ID::marblesRate));
    p.marblesJitter = get(ID::marblesJitter); p.marblesBias = get(ID::marblesBias);
    p.marblesDejaVu = get(ID::marblesDejaVu); p.marblesLoopLength = geti(ID::marblesLength);
    p.marblesModel = geti(ID::marblesModel);
    p.lfoRate = geti(ID::lfoRate); p.lfoShape = geti(ID::lfoShape);

    p.delaySync = get(ID::dlySync) > 0.5f; p.delayDivision = geti(ID::dlyDiv);
    p.delayTimeMs = get(ID::dlyTime); p.delayFeedback = get(ID::dlyFeedback); p.delayTone = get(ID::dlyTone);
    p.delayPingPong = get(ID::dlyPingPong); p.delayWarmth = get(ID::dlyTape); p.delayReturnDB = get(ID::dlyReturn);
    p.reverbDecay = get(ID::revDecay); p.reverbDamp = get(ID::revBright); p.reverbPreDelayMs = get(ID::revPreDelay);
    p.reverbWidth = get(ID::revWidth); p.reverbReturnDB = get(ID::revReturn);

    for (int v = 0; v < kVoices; ++v) {
        auto& c = p.channels[static_cast<size_t>(v)];
        auto G = [&](const char* s) { return get(vid(v, s)); };
        c.engine = static_cast<int>(std::lround(G(V::engine)));
        c.triggerSource = static_cast<int>(std::lround(G(V::trig)));
        c.chokeGroup = static_cast<int>(std::lround(G(V::choke)));
        c.midiNote = static_cast<int>(std::lround(G(V::note)));
        c.tune = G(V::tune); c.decay = G(V::decay); c.tone = G(V::tone);
        c.timbre = c.morph = G(V::chr);
        c.volumeDB = G(V::level); c.pan = G(V::pan);
        c.aux1SendDB = G(V::send1); c.aux2SendDB = G(V::send2);
        c.eqBypass = G(V::eqOn) < 0.5f;
        c.eqLowFreq = G(V::eqLoF); c.eqLowGain = G(V::eqLoG);
        c.eqMidFreq = G(V::eqMidF); c.eqMidQ = G(V::eqMidQ); c.eqMidGain = G(V::eqMidG);
        c.eqHighFreq = G(V::eqHiF); c.eqHighGain = G(V::eqHiG);
        c.driveBypass = G(V::drvOn) < 0.5f;
        c.drive = G(V::drive); c.driveTone = G(V::drvTone); c.driveTrimDB = G(V::drvTrim);
        c.driveMode = static_cast<int>(std::lround(G(V::drvMode)));
        c.compBypass = G(V::cmpOn) < 0.5f;
        c.compThresh = G(V::cmpThr); c.compRatio = G(V::cmpRat); c.compAttackMs = G(V::cmpAtk);
        c.compReleaseMs = G(V::cmpRel); c.compMakeupDB = G(V::cmpMak);
        int* srcs[3] = { &c.modSource1, &c.modSource2, &c.modSource3 };
        int* dsts[3] = { &c.modDest1, &c.modDest2, &c.modDest3 };
        float* amts[3] = { &c.modAmount1, &c.modAmount2, &c.modAmount3 };
        for (int s = 0; s < kNumModSlots; ++s) {
            *srcs[s] = geti(modSrcId(v, s));
            *dsts[s] = geti(modDstId(v, s));
            *amts[s] = get(modAmtId(v, s));
        }
    }
    return p;
}

// =====================================================================================
//  Audio
// =====================================================================================
void MutableDrumsAudioProcessor::prepareToPlay(double sampleRate, int) {
    sampleRate_ = sampleRate;
    for (int i = 0; i < kVoices; ++i) {
        voices_[static_cast<size_t>(i)].init(static_cast<float>(sampleRate));
        channels_[static_cast<size_t>(i)].init(static_cast<float>(sampleRate));
    }
    delay_.init(static_cast<float>(sampleRate));
    reverb_.init(static_cast<float>(sampleRate));
    bpm_ = pBpm_->load();
    pushParametersToDsp();
    for (auto& c : channels_) c.reset();
    delay_.reset();
    reverb_.reset();
    masterGain_ = masterTarget_;
    wasRunning_ = false;
    expectedHostPos_ = -1.0;
}

void MutableDrumsAudioProcessor::releaseResources() {
    for (auto& v : voices_) v.reset();
    for (auto& c : channels_) c.reset();
    delay_.reset();
    reverb_.reset();
}

bool MutableDrumsAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void MutableDrumsAudioProcessor::triggerVoiceManual(int voiceIndex) {
    if (voiceIndex >= 0 && voiceIndex < kVoices)
        pendingManual_.fetch_or(1u << voiceIndex);
}

void MutableDrumsAudioProcessor::triggerVoice(int v, float velocity) {
    const int cg = chokeGroup_[static_cast<size_t>(v)];
    if (cg > 0) {
        for (int j = 0; j < kVoices; ++j)
            if (j != v && chokeGroup_[static_cast<size_t>(j)] == cg) voices_[static_cast<size_t>(j)].choke();
    }
    ModContext ctx;
    ctx.x[0] = marbles_.getVoltage(0);
    ctx.x[1] = marbles_.getVoltage(1);
    ctx.x[2] = marbles_.getVoltage(2);
    ctx.lfo = lfo_;
    ctx.barPos = static_cast<float>(grids_.getCurrentStep() % 16) / 16.0f;

    auto& voice = voices_[static_cast<size_t>(v)];
    voice.trigger(velocity, ctx);
    voiceHitCount[static_cast<size_t>(v)].fetch_add(1);
    for (int d = 0; d < NUM_MOD_DESTINATIONS; ++d)
        voiceMod[static_cast<size_t>(v)][static_cast<size_t>(d)].store(voice.getModOffset(d));
}

void MutableDrumsAudioProcessor::fireGridsStep(long long absStep) {
    const float x1 = marbles_.getVoltage(0), x2 = marbles_.getVoltage(1), x3 = marbles_.getVoltage(2);
    const float modX = pModGX_->load() * (x1 - 0.5f) * 2.0f;
    const float modY = pModGY_->load() * (x2 - 0.5f) * 2.0f;
    const float modChaos = pModGChaos_->load() * (x3 - 0.5f) * 2.0f;
    const float modDens = pModGDens_->load() * (marbles_.getGate(0) ? 0.35f : -0.15f);

    grids_.evaluateAt(absStep, modX, modY, modChaos, modDens);
    currentStep.store(grids_.getCurrentStep());
    effectiveGridsX.store(grids_.getEffectiveX());
    effectiveGridsY.store(grids_.getEffectiveY());

    for (int k = 0; k < 3; ++k)
        if (grids_.isTriggered(k)) gridsHitCount[static_cast<size_t>(k)].fetch_add(1);

    for (int v = 0; v < kVoices; ++v) {
        const int src = trigSource_[static_cast<size_t>(v)];
        if (src >= TRIG_GRIDS_BD && src <= TRIG_GRIDS_HH) {
            const int inst = src - TRIG_GRIDS_BD;
            if (grids_.isTriggered(inst)) triggerVoice(v, grids_.isAccented(inst) ? 1.0f : 0.72f);
        }
    }
}

void MutableDrumsAudioProcessor::fireMarblesStep(long long absStep) {
    marbles_.stepClock(static_cast<int>(absStep % 1000000));
    marblesStep.store(marbles_.getHistoryIndex());
    for (int k = 0; k < 3; ++k) {
        marblesX[static_cast<size_t>(k)].store(marbles_.getVoltage(k));
        if (marbles_.getGate(k)) marblesHitCount[static_cast<size_t>(k)].fetch_add(1);
    }
    for (int v = 0; v < kVoices; ++v) {
        const int src = trigSource_[static_cast<size_t>(v)];
        if (src >= TRIG_MARBLES_T1 && src <= TRIG_MARBLES_T3) {
            const int t = src - TRIG_MARBLES_T1;
            if (marbles_.getGate(t)) triggerVoice(v, 0.6f + 0.4f * marbles_.getVoltage(t));
        }
    }
}

float MutableDrumsAudioProcessor::computeLfo(double beatPos) {
    const double len = Params::lfoRateBeats(static_cast<int>(std::lround(pLfoRate_->load())));
    const double cycles = beatPos / len;
    const double cyc = std::floor(cycles);
    const float ph = static_cast<float>(cycles - cyc);
    if (cyc != lastLfoCycle_) {
        lastLfoCycle_ = cyc;
        rng_ = rng_ * 1664525u + 1013904223u;
        lfoSH_ = static_cast<float>(rng_ >> 8) / 8388608.0f - 1.0f;
    }
    switch (static_cast<int>(std::lround(pLfoShape_->load()))) {
        case 0: return std::sin(ph * juce::MathConstants<float>::twoPi);
        case 1: return 1.0f - 4.0f * std::abs(ph - 0.5f);
        case 2: return ph * 2.0f - 1.0f;
        case 3: return 1.0f - ph * 2.0f;
        case 4: return ph < 0.5f ? 1.0f : -1.0f;
        default: return lfoSH_;
    }
}

void MutableDrumsAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    for (int c = 0; c < buffer.getNumChannels(); ++c) buffer.clear(c, 0, numSamples);
    if (buffer.getNumChannels() < 2) return;

    // ---- Transport
    const bool hostSync = pHostSync_->load() > 0.5f;
    bool hostPlaying = false;
    double ppq = 0.0;
    bool hasPpq = false;
    float bpm = pBpm_->load();
    if (auto* ph = getPlayHead()) {
        if (auto pos = ph->getPosition()) {
            hostPlaying = pos->getIsPlaying();
            if (hostSync && pos->getBpm().hasValue()) bpm = static_cast<float>(*pos->getBpm());
            if (pos->getPpqPosition().hasValue()) { ppq = *pos->getPpqPosition(); hasPpq = true; }
        }
    }
    bpm_ = juce::jlimit(30.0f, 300.0f, bpm);
    hostIsPlaying.store(hostPlaying);
    currentBpm.store(bpm_);

    pushParametersToDsp();
    delayTimeMs.store(delay_.getEffectiveTimeMs());

    const bool running = pRun_->load() > 0.5f && (hostSync ? hostPlaying : true);
    const double samplesPer16th = sampleRate_ * 60.0 / (static_cast<double>(bpm_) * 4.0);
    const double inc = 1.0 / samplesPer16th;

    if (running) {
        if (hostSync && hasPpq) {
            const double hostPos = ppq * 4.0;
            if (!wasRunning_ || std::abs(hostPos - expectedHostPos_) > 0.25) {
                clockPos_ = hostPos;
                nextGridsStep_ = static_cast<long long>(std::ceil(clockPos_ - 1.0e-6));
                nextMarblesStep_ = static_cast<long long>(std::ceil(clockPos_ * marblesRate_ - 1.0e-6));
                marblesJitterDelay_ = 0.0;
            } else {
                clockPos_ = hostPos; // follow the host exactly, no drift
            }
        } else if (!wasRunning_) {
            clockPos_ = 0.0;
            nextGridsStep_ = 0;
            nextMarblesStep_ = 0;
            marblesJitterDelay_ = 0.0;
        }
    }
    wasRunning_ = running;
    clockRunning.store(running);

    // LFO (per block is plenty: it is sampled per hit)
    const double beatPos = (running ? clockPos_ * 0.25 : freeBeatPos_);
    lfo_ = computeLfo(beatPos);
    lfoValue.store(lfo_);
    freeBeatPos_ = running ? beatPos : freeBeatPos_ + numSamples * (bpm_ / 60.0) / sampleRate_;

    // Manual pad hits from the GUI
    if (const uint32_t manual = pendingManual_.exchange(0)) {
        for (int v = 0; v < kVoices; ++v)
            if (manual & (1u << v)) triggerVoice(v, 1.0f);
    }

    float* outL = buffer.getWritePointer(0);
    float* outR = buffer.getWritePointer(1);
    float peakL = 0.0f, peakR = 0.0f, pkD = 0.0f, pkR = 0.0f;

    auto midiIt = midiMessages.cbegin();
    const auto midiEnd = midiMessages.cend();

    for (int n = 0; n < numSamples; ++n) {
        // ---- Sample-accurate MIDI
        while (midiIt != midiEnd && (*midiIt).samplePosition <= n) {
            const auto msg = (*midiIt).getMessage();
            if (msg.isNoteOn()) {
                const int note = msg.getNoteNumber();
                for (int v = 0; v < kVoices; ++v)
                    if (midiNote_[static_cast<size_t>(v)] == note) triggerVoice(v, msg.getFloatVelocity());
            }
            ++midiIt;
        }

        // ---- Sequencer clock
        if (running) {
            // Grids (with swing on odd 16ths)
            for (int guard = 0; guard < 4; ++guard) {
                const double swingDelay = (nextGridsStep_ & 1) ? static_cast<double>(swing_) * 0.5 : 0.0;
                if (clockPos_ < static_cast<double>(nextGridsStep_) + swingDelay) break;
                fireGridsStep(nextGridsStep_);
                ++nextGridsStep_;
            }
            // Marbles (own rate, with humanising jitter)
            const double mPos = clockPos_ * marblesRate_;
            for (int guard = 0; guard < 4; ++guard) {
                if (mPos < static_cast<double>(nextMarblesStep_) + marblesJitterDelay_) break;
                fireMarblesStep(nextMarblesStep_);
                ++nextMarblesStep_;
                rng_ = rng_ * 1664525u + 1013904223u;
                const double r = static_cast<double>(rng_ >> 8) / 16777216.0; // 0..1
                marblesJitterDelay_ = static_cast<double>(marblesJitter_) * 0.45 * r * r;
            }
            clockPos_ += inc;
        }

        // ---- Voices -> channel strips -> busses
        float sumL = 0.0f, sumR = 0.0f, aux1 = 0.0f, aux2 = 0.0f;
        for (int v = 0; v < kVoices; ++v) {
            const float dry = voices_[static_cast<size_t>(v)].process();
            float l, r, a1, a2;
            channels_[static_cast<size_t>(v)].process(dry, l, r, a1, a2);
            sumL += l; sumR += r; aux1 += a1; aux2 += a2;
        }

        float dL, dR, rL, rR;
        delay_.process(aux1, aux1, dL, dR);
        reverb_.process(aux2, aux2, rL, rR);

        masterGain_ += (masterTarget_ - masterGain_) * 0.001f;
        float busL = (sumL * 0.65f + dL * 0.7f + rL * 0.7f) * masterGain_;
        float busR = (sumR * 0.65f + dR * 0.7f + rR * 0.7f) * masterGain_;

        // Transparent safety limiter: linear to -1.4 dBFS, soft knee above
        auto softLimit = [](float x) {
            const float a = std::abs(x);
            if (a < 0.85f) return x;
            return std::copysign(0.85f + 0.13f * std::tanh((a - 0.85f) / 0.13f), x);
        };
        busL = softLimit(busL);
        busR = softLimit(busR);
        outL[n] = busL;
        outR[n] = busR;

        peakL = std::max(peakL, std::abs(busL));
        peakR = std::max(peakR, std::abs(busR));
        pkD = std::max(pkD, std::max(std::abs(dL), std::abs(dR)));
        pkR = std::max(pkR, std::max(std::abs(rL), std::abs(rR)));
    }

    if (running && hostSync && hasPpq) expectedHostPos_ = clockPos_;

    for (int v = 0; v < kVoices; ++v) {
        channelPeaks_[static_cast<size_t>(v)].store(channels_[static_cast<size_t>(v)].getPeakLevel());
        channelGRs_[static_cast<size_t>(v)].store(channels_[static_cast<size_t>(v)].getGainReductionDB());
    }
    masterPeakL_.store(peakL);
    masterPeakR_.store(peakR);
    const float blockDecay = std::exp(-static_cast<float>(numSamples) / (0.3f * static_cast<float>(sampleRate_)));
    dlyPeakHold_ = std::max(pkD, dlyPeakHold_ * blockDecay);
    revPeakHold_ = std::max(pkR, revPeakHold_ * blockDecay);
    dlyPeak_.store(dlyPeakHold_);
    revPeak_.store(revPeakHold_);
}

juce::AudioProcessorEditor* MutableDrumsAudioProcessor::createEditor() {
    return new MutableDrumsAudioProcessorEditor(*this);
}

// =====================================================================================
//  State
// =====================================================================================
void MutableDrumsAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {
    auto state = apvts_.copyState();
    state.setProperty("presetName", currentPresetName_, nullptr);
    state.setProperty("presetIndex", presetManager_.getCurrentPresetIndex(), nullptr);
    state.setProperty("selectedVoice", selectedVoice.load(), nullptr);
    state.setProperty("stateVersion", 2, nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary(*xml, destData);
}

void MutableDrumsAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
    if (auto xml = getXmlFromBinary(data, sizeInBytes)) {
        if (xml->hasTagName(apvts_.state.getType())) {
            auto tree = juce::ValueTree::fromXml(*xml);
            currentPresetName_ = tree.getProperty("presetName", "Init").toString();
            presetManager_.selectPreset(static_cast<int>(tree.getProperty("presetIndex", 0)));
            selectedVoice.store(juce::jlimit(0, kVoices - 1, static_cast<int>(tree.getProperty("selectedVoice", 0))));
            apvts_.replaceState(tree);
            return;
        }
    }

    // Legacy (v1) sessions stored the state as a JSON preset
    juce::String json(juce::CharPointer_UTF8(static_cast<const char*>(data)), static_cast<size_t>(sizeInBytes));
    auto parsed = juce::JSON::parse(json);
    if (parsed.isObject()) {
        auto preset = presetManager_.deserializePreset(parsed);
        applyPreset(preset);
        currentPresetName_ = preset.name;
        undoManager_.clearUndoHistory();
    }
}

} // namespace MutableDrums

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new MutableDrums::MutableDrumsAudioProcessor();
}
