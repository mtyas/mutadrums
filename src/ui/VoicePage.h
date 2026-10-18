#pragma once
#include "Widgets.h"
#include "../PluginProcessor.h"

namespace MutableDrums {

// =====================================================================================
//  EQ response curve for the selected channel
// =====================================================================================
class EqCurve : public juce::Component {
public:
    explicit EqCurve(APVTS& s) : state_(s) { setInterceptsMouseClicks(false, false); }
    void setVoice(int v) { voice_ = v; repaint(); }
    void setColour(juce::Colour c) { colour_ = c; }

    void paint(juce::Graphics& g) override {
        auto r = getLocalBounds().toFloat();
        Theme::wellBox(g, r, 5.0f);
        auto a = r.reduced(4.0f, 4.0f);
        auto P = [this](const char* sfx) { return state_.getRawParameterValue(Params::vid(voice_, sfx))->load(); };
        const bool on = P(Params::V::eqOn) > 0.5f;
        const float sr = 48000.0f;
        Biquad lo, mid, hi;
        lo.setLowShelf(P(Params::V::eqLoF), P(Params::V::eqLoG), sr);
        mid.setPeaking(P(Params::V::eqMidF), P(Params::V::eqMidQ), P(Params::V::eqMidG), sr);
        hi.setHighShelf(P(Params::V::eqHiF), P(Params::V::eqHiG), sr);

        // grid: 100 Hz, 1 kHz, 10 kHz and 0 dB
        g.setColour(Theme::outline.withAlpha(0.8f));
        for (float f : { 100.0f, 1000.0f, 10000.0f }) {
            const float x = a.getX() + a.getWidth() * xFor(f);
            g.drawVerticalLine(juce::roundToInt(x), a.getY(), a.getBottom());
        }
        g.drawHorizontalLine(juce::roundToInt(a.getCentreY()), a.getX(), a.getRight());

        juce::Path path;
        const int n = juce::jmax(2, static_cast<int>(a.getWidth()));
        for (int i = 0; i < n; ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(n - 1);
            const float f = 20.0f * std::pow(1000.0f, t);
            float db = 0.0f;
            if (on) db = juce::Decibels::gainToDecibels(lo.magnitudeAt(f, sr) * mid.magnitudeAt(f, sr) * hi.magnitudeAt(f, sr));
            const float y = a.getCentreY() - juce::jlimit(-20.0f, 20.0f, db) / 20.0f * (a.getHeight() * 0.5f - 2.0f);
            const float x = a.getX() + t * a.getWidth();
            if (i == 0) path.startNewSubPath(x, y); else path.lineTo(x, y);
        }
        juce::Path fill(path);
        fill.lineTo(a.getRight(), a.getCentreY());
        fill.lineTo(a.getX(), a.getCentreY());
        fill.closeSubPath();
        g.setColour((on ? colour_ : Theme::textFaint).withAlpha(0.15f));
        g.fillPath(fill);
        g.setColour(on ? colour_ : Theme::textFaint);
        g.strokePath(path, juce::PathStrokeType(1.6f));
    }

private:
    static float xFor(float f) { return std::log(f / 20.0f) / std::log(1000.0f); }
    APVTS& state_;
    int voice_{ 0 };
    juce::Colour colour_{ Theme::accent };
};

// =====================================================================================
//  One modulation slot: [source] -> [destination]  [amount]
// =====================================================================================
class ModSlotRow : public juce::Component {
public:
    explicit ModSlotRow(int slot) : slot_(slot) {
        addAndMakeVisible(src_);
        addAndMakeVisible(dst_);
        addAndMakeVisible(amt_);
        amt_.setSliderStyle(juce::Slider::LinearHorizontal);
        amt_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        amt_.getProperties().set("bipolar", true);
        amt_.getProperties().set("accent", static_cast<juce::int64>(Theme::mod.getARGB()));
        amt_.onValueChange = [this] { repaint(); };
        src_.combo.setTooltip("Modulation source. Sampled once on every hit:\n"
                              "Marbles X1-X3 - stepped random voltages\nVelocity - softer hits move the destination\n"
                              "Random - new value per hit\nLFO - global tempo-synced LFO\nBar Ramp - rises across the bar");
        dst_.combo.setTooltip("What this slot modulates");
        amt_.setTooltip("Depth and direction. Double-click to reset.");
    }

    void attach(APVTS& s, int voice, juce::UndoManager* um) {
        src_.attach(s, Params::modSrcId(voice, slot_), um);
        dst_.attach(s, Params::modDstId(voice, slot_), um);
        att_.reset();
        att_ = std::make_unique<juce::SliderParameterAttachment>(*s.getParameter(Params::modAmtId(voice, slot_)), amt_, um);
        repaint();
    }

    void paint(juce::Graphics& g) override {
        g.setColour(Theme::textFaint);
        g.setFont(Theme::font(11.0f, true));
        g.drawText(juce::String(slot_ + 1), numArea_, juce::Justification::centredLeft);
        g.drawText(juce::CharPointer_UTF8("\xe2\x86\x92"), arrowArea_, juce::Justification::centred);
        const bool active = src_.combo.getSelectedId() > 1 && std::abs(amt_.getValue()) > 0.001;
        g.setColour(active ? Theme::mod : Theme::textDim);
        g.setFont(Theme::font(11.5f, true));
        g.drawText(amt_.getTextFromValue(amt_.getValue()), valueArea_, juce::Justification::centredRight);
    }

    void resized() override {
        auto r = getLocalBounds();
        numArea_ = r.removeFromLeft(14);
        src_.setBounds(r.removeFromLeft(118).reduced(0, 2));
        arrowArea_ = r.removeFromLeft(20);
        dst_.setBounds(r.removeFromLeft(100).reduced(0, 2));
        r.removeFromLeft(10);
        valueArea_ = r.removeFromRight(44);
        amt_.setBounds(r.reduced(0, 3));
    }

private:
    int slot_;
    ChoiceBox src_, dst_;
    juce::Slider amt_;
    std::unique_ptr<juce::SliderParameterAttachment> att_;
    juce::Rectangle<int> numArea_, arrowArea_, valueArea_;
};

// =====================================================================================
//  VoicePage — everything about the selected voice
// =====================================================================================
class VoicePage : public juce::Component {
public:
    explicit VoicePage(MutableDrumsAudioProcessor& p) : proc_(p), state_(p.getAPVTS()), eq_(state_) {
        for (auto* c : std::initializer_list<juce::Component*>{ &engine_, &trig_, &choke_, &note_,
                                                                &tune_, &decay_, &tone_, &char_, &level_, &pan_,
                                                                &send1_, &send2_, &lfoRate_, &lfoShape_,
                                                                &eqOn_, &eq_, &eqLoF_, &eqLoG_, &eqMidF_, &eqMidQ_, &eqMidG_, &eqHiF_, &eqHiG_,
                                                                &drvOn_, &drvMode_, &drive_, &drvTone_, &drvTrim_,
                                                                &cmpOn_, &cmpThr_, &cmpRat_, &cmpAtk_, &cmpRel_, &cmpMak_ })
            addAndMakeVisible(c);
        for (int i = 0; i < kNumModSlots; ++i) addAndMakeVisible(slots_.add(new ModSlotRow(i)));

        engine_.combo.setTooltip("Sound engine for this voice");
        trig_.combo.setTooltip("What plays this voice: a Grids lane, a Marbles gate, or only MIDI / the pad.\n"
                               "MIDI notes always play it as well.");
        choke_.combo.setTooltip("Voices in the same group cut each other off (e.g. closed hat chokes open hat).");
        note_.combo.setTooltip("MIDI note that plays this voice");

        tune_.setTooltipText("Pitch in semitones. Double-click to reset.");
        decay_.setTooltipText("Length of the sound");
        level_.setTooltipText("Channel fader");
        pan_.setTooltipText("Stereo position");
        send1_.setTooltipText("How much of this voice goes to the Delay");
        send2_.setTooltipText("How much of this voice goes to the Reverb");
        pan_.setBipolar(true);
        tune_.setBipolar(true);
        eqLoG_.setBipolar(true); eqMidG_.setBipolar(true); eqHiG_.setBipolar(true); drvTrim_.setBipolar(true);
        lfoRate_.combo.setTooltip("Global LFO speed (tempo-synced). Shared by all voices.");
        lfoShape_.combo.setTooltip("Global LFO shape");

        auto* um = &p.getUndoManager();
        lfoRate_.attach(state_, Params::ID::lfoRate, um);
        lfoShape_.attach(state_, Params::ID::lfoShape, um);

        setVoice(p.selectedVoice.load());
    }

    void setVoice(int v) {
        voice_ = juce::jlimit(0, Params::kNumVoices - 1, v);
        auto* um = &proc_.getUndoManager();
        using namespace Params;
        engine_.attach(state_, vid(voice_, V::engine), um);
        trig_.attach(state_, vid(voice_, V::trig), um);
        choke_.attach(state_, vid(voice_, V::choke), um);
        note_.attach(state_, vid(voice_, V::note), um);
        tune_.attach(state_, vid(voice_, V::tune), um);
        decay_.attach(state_, vid(voice_, V::decay), um);
        tone_.attach(state_, vid(voice_, V::tone), um);
        char_.attach(state_, vid(voice_, V::chr), um);
        level_.attach(state_, vid(voice_, V::level), um);
        pan_.attach(state_, vid(voice_, V::pan), um);
        send1_.attach(state_, vid(voice_, V::send1), um);
        send2_.attach(state_, vid(voice_, V::send2), um);
        eqOn_.attach(state_, vid(voice_, V::eqOn), um);
        eqLoF_.attach(state_, vid(voice_, V::eqLoF), um);
        eqLoG_.attach(state_, vid(voice_, V::eqLoG), um);
        eqMidF_.attach(state_, vid(voice_, V::eqMidF), um);
        eqMidQ_.attach(state_, vid(voice_, V::eqMidQ), um);
        eqMidG_.attach(state_, vid(voice_, V::eqMidG), um);
        eqHiF_.attach(state_, vid(voice_, V::eqHiF), um);
        eqHiG_.attach(state_, vid(voice_, V::eqHiG), um);
        drvOn_.attach(state_, vid(voice_, V::drvOn), um);
        drvMode_.attach(state_, vid(voice_, V::drvMode), um);
        drive_.attach(state_, vid(voice_, V::drive), um);
        drvTone_.attach(state_, vid(voice_, V::drvTone), um);
        drvTrim_.attach(state_, vid(voice_, V::drvTrim), um);
        cmpOn_.attach(state_, vid(voice_, V::cmpOn), um);
        cmpThr_.attach(state_, vid(voice_, V::cmpThr), um);
        cmpRat_.attach(state_, vid(voice_, V::cmpRat), um);
        cmpAtk_.attach(state_, vid(voice_, V::cmpAtk), um);
        cmpRel_.attach(state_, vid(voice_, V::cmpRel), um);
        cmpMak_.attach(state_, vid(voice_, V::cmpMak), um);
        for (auto* s : slots_) s->attach(state_, voice_, um);

        engineWatch_ = std::make_unique<juce::ParameterAttachment>(*state_.getParameter(vid(voice_, V::engine)),
                                                                   [this](float e) { setEngine(juce::roundToInt(e)); });
        engineWatch_->sendInitialUpdate();

        const auto col = Theme::voice(voice_);
        for (auto* k : { &tune_, &decay_, &tone_, &char_ }) k->setAccent(col);
        eq_.setVoice(voice_);
        eq_.setColour(col);
        eqOn_.setOnColour(col);
        drvOn_.setOnColour(col);
        cmpOn_.setOnColour(col);
        drvMode_.setOnColour(col);
        repaint();
    }

    void refresh() {
        // Live per-hit modulation rings
        auto mod = [this](int d) { return proc_.voiceMod[static_cast<size_t>(voice_)][static_cast<size_t>(d)].load(); };
        tune_.setModOffset(mod(MOD_DEST_PITCH) * 24.0f / 72.0f);
        decay_.setModOffset(mod(MOD_DEST_DECAY) * 0.5f);
        tone_.setModOffset(mod(MOD_DEST_TONE) * 0.5f);
        char_.setModOffset(mod(MOD_DEST_CHARACTER) * 0.5f);
        level_.setModOffset(mod(MOD_DEST_LEVEL) * 24.0f / 66.0f);
        pan_.setModOffset(mod(MOD_DEST_PAN) * 0.5f);

        const float gr = proc_.getChannelGR(voice_);
        if (std::abs(gr - gr_) > 0.05f || ++frame_ % 4 == 0) { gr_ = gr; repaint(grArea_); }
        if (frame_ % 3 == 0) eq_.repaint();
    }

    void paint(juce::Graphics& g) override {
        const auto col = Theme::voice(voice_);
        Theme::panelBox(g, soundArea_.toFloat());
        // Title
        auto head = soundArea_.reduced(12, 0).removeFromTop(30);
        g.setColour(col);
        g.fillRoundedRectangle(head.removeFromLeft(4).withSizeKeepingCentre(4, 14).toFloat(), 2.0f);
        head.removeFromLeft(8);
        g.setColour(Theme::text);
        g.setFont(Theme::font(14.0f, true));
        g.drawText("VOICE " + juce::String(voice_ + 1), head.removeFromLeft(80), juce::Justification::centredLeft);
        g.setColour(Theme::textDim);
        g.setFont(Theme::font(13.0f));
        g.drawText(getEngineInfo(engineIdx_).name, head, juce::Justification::centredLeft);

        g.setColour(Theme::textDim);
        g.setFont(Theme::font(11.5f));
        g.drawFittedText(getEngineInfo(engineIdx_).blurb, blurbArea_, juce::Justification::topLeft, 2, 0.9f);

        Theme::panelBox(g, modArea_.toFloat(), "Modulation  (per hit)", Theme::mod);
        Theme::panelBox(g, sendArea_.toFloat(), "Output");
        Theme::panelBox(g, eqArea_.toFloat(), "EQ");
        Theme::panelBox(g, drvArea_.toFloat(), "Drive");
        Theme::panelBox(g, cmpArea_.toFloat(), "Compressor");

        g.setColour(Theme::textFaint);
        g.setFont(Theme::font(9.5f, true));
        g.drawText("GLOBAL LFO", lfoLabel_, juce::Justification::centredLeft);

        // Gain-reduction bar
        Theme::wellBox(g, grArea_.toFloat(), 3.0f);
        const float grN = juce::jlimit(0.0f, 1.0f, gr_ / 24.0f);
        auto bar = grArea_.toFloat().reduced(1.5f);
        g.setColour(Theme::accent);
        g.fillRoundedRectangle(bar.withLeft(bar.getRight() - bar.getWidth() * grN), 2.0f);
        g.setColour(Theme::textDim);
        g.setFont(Theme::font(10.0f, true));
        g.drawText("GR " + juce::String(-gr_, 1) + " dB", grLabel_, juce::Justification::centredLeft);
    }

    void resized() override {
        auto r = getLocalBounds();

        // ---- Row A: sound
        soundArea_ = r.removeFromTop(180);
        {
            auto a = soundArea_.reduced(12, 0);
            a.removeFromTop(32);
            a.removeFromBottom(10);
            auto left = a.removeFromLeft(300);
            engine_.setBounds(left.removeFromTop(46));
            left.removeFromTop(4);
            blurbArea_ = left.removeFromTop(32);
            left.removeFromTop(6);
            auto row = left.removeFromTop(46);
            const int w3 = (row.getWidth() - 12) / 3;
            trig_.setBounds(row.removeFromLeft(w3 + 10));
            row.removeFromLeft(6);
            choke_.setBounds(row.removeFromLeft(w3 - 10));
            row.removeFromLeft(6);
            note_.setBounds(row);

            a.removeFromLeft(14);
            const int kw = a.getWidth() / 4;
            auto knobs = a.withSizeKeepingCentre(a.getWidth(), 112);
            tune_.setBounds(knobs.removeFromLeft(kw).reduced(4, 0));
            decay_.setBounds(knobs.removeFromLeft(kw).reduced(4, 0));
            tone_.setBounds(knobs.removeFromLeft(kw).reduced(4, 0));
            char_.setBounds(knobs.reduced(4, 0));
        }
        r.removeFromTop(10);

        // ---- Row B: modulation + output
        auto rowB = r.removeFromTop(146);
        sendArea_ = rowB.removeFromRight(300);
        rowB.removeFromRight(10);
        modArea_ = rowB;
        {
            auto a = modArea_.reduced(12, 0);
            a.removeFromTop(30);
            for (auto* s : slots_) s->setBounds(a.removeFromTop(28));
            a.removeFromTop(4);
            auto lfo = a.removeFromTop(26);
            lfoLabel_ = lfo.removeFromLeft(80);
            lfoRate_.setBounds(lfo.removeFromLeft(110).reduced(0, 1));
            lfo.removeFromLeft(8);
            lfoShape_.setBounds(lfo.removeFromLeft(110).reduced(0, 1));
        }
        {
            auto a = sendArea_.reduced(10, 0);
            a.removeFromTop(28);
            a.removeFromBottom(8);
            const int w = a.getWidth() / 4;
            level_.setBounds(a.removeFromLeft(w));
            pan_.setBounds(a.removeFromLeft(w));
            send1_.setBounds(a.removeFromLeft(w));
            send2_.setBounds(a);
        }
        r.removeFromTop(10);

        // ---- Row C: channel inserts
        auto rowC = r;
        eqArea_ = rowC.removeFromLeft(352);
        rowC.removeFromLeft(10);
        drvArea_ = rowC.removeFromLeft(210);
        rowC.removeFromLeft(10);
        cmpArea_ = rowC;
        {
            auto a = eqArea_.reduced(10, 0);
            auto head = a.removeFromTop(30);
            eqOn_.setBounds(head.removeFromRight(44).reduced(0, 6));
            eq_.setBounds(a.removeFromTop(64));
            a.removeFromTop(4);
            const int w = a.getWidth() / 7;
            for (auto* k : { &eqLoF_, &eqLoG_, &eqMidF_, &eqMidQ_, &eqMidG_, &eqHiF_ }) k->setBounds(a.removeFromLeft(w).withHeight(78));
            eqHiG_.setBounds(a.withHeight(78));
        }
        {
            auto a = drvArea_.reduced(10, 0);
            auto head = a.removeFromTop(30);
            drvOn_.setBounds(head.removeFromRight(44).reduced(0, 6));
            drvMode_.setBounds(a.removeFromTop(26));
            a.removeFromTop(12);
            const int w = a.getWidth() / 3;
            drive_.setBounds(a.removeFromLeft(w).withHeight(84));
            drvTone_.setBounds(a.removeFromLeft(w).withHeight(84));
            drvTrim_.setBounds(a.withHeight(84));
        }
        {
            auto a = cmpArea_.reduced(10, 0);
            auto head = a.removeFromTop(30);
            cmpOn_.setBounds(head.removeFromRight(44).reduced(0, 6));
            auto grRow = a.removeFromTop(16);
            grLabel_ = grRow.removeFromLeft(78);
            grArea_ = grRow.reduced(0, 3);
            a.removeFromTop(6);
            auto row1 = a.removeFromTop(78);
            const int w = row1.getWidth() / 3;
            cmpThr_.setBounds(row1.removeFromLeft(w));
            cmpRat_.setBounds(row1.removeFromLeft(w));
            cmpMak_.setBounds(row1);
            auto row2 = a.removeFromTop(78);
            cmpAtk_.setBounds(row2.removeFromLeft(w).translated(w / 2, 0));
            cmpRel_.setBounds(row2.removeFromLeft(w).translated(w / 2, 0));
        }
    }

private:
    void setEngine(int e) {
        engineIdx_ = e;
        const auto& info = getEngineInfo(e);
        tone_.setLabel(info.toneLabel);
        char_.setLabel(info.charLabel);
        tone_.setTooltipText(juce::String(info.toneLabel) + " - tone control for " + info.name);
        char_.setTooltipText(juce::String(info.charLabel) + " - character control for " + info.name);
        repaint();
    }

    MutableDrumsAudioProcessor& proc_;
    APVTS& state_;
    int voice_{ 0 }, engineIdx_{ 0 }, frame_{ 0 };
    float gr_{ 0.0f };

    juce::Rectangle<int> soundArea_, blurbArea_, modArea_, sendArea_, eqArea_, drvArea_, cmpArea_, lfoLabel_, grArea_, grLabel_;

    ChoiceBox engine_{ "Engine" }, trig_{ "Trigger" }, choke_{ "Choke" }, note_{ "MIDI Note" };
    Knob tune_{ "Tune" }, decay_{ "Decay" }, tone_{ "Tone" }, char_{ "Character" };
    Knob level_{ "Level", Theme::text }, pan_{ "Pan", Theme::text }, send1_{ "Delay", Theme::marblesT(0) }, send2_{ "Reverb", Theme::marblesT(1) };

    juce::OwnedArray<ModSlotRow> slots_;
    ChoiceBox lfoRate_, lfoShape_;

    ToggleChip eqOn_{ "ON" }, drvOn_{ "ON" }, cmpOn_{ "ON" };
    EqCurve eq_;
    Knob eqLoF_{ "Low" }, eqLoG_{ "Gain" }, eqMidF_{ "Mid" }, eqMidQ_{ "Q" }, eqMidG_{ "Gain" }, eqHiF_{ "High" }, eqHiG_{ "Gain" };
    Segmented drvMode_;
    Knob drive_{ "Drive" }, drvTone_{ "Tone" }, drvTrim_{ "Trim" };
    Knob cmpThr_{ "Thresh" }, cmpRat_{ "Ratio" }, cmpAtk_{ "Attack" }, cmpRel_{ "Release" }, cmpMak_{ "Makeup" };

    std::unique_ptr<juce::ParameterAttachment> engineWatch_;
};

} // namespace MutableDrums
