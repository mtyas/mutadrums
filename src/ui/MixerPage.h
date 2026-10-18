#pragma once
#include "Widgets.h"
#include "../PluginProcessor.h"

namespace MutableDrums {

// Vertical fader bound to a parameter, with its value printed underneath
class Fader : public juce::Component {
public:
    explicit Fader(juce::Colour c = Theme::accent) {
        slider.setSliderStyle(juce::Slider::LinearVertical);
        slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        slider.getProperties().set("accent", static_cast<juce::int64>(c.getARGB()));
        slider.onValueChange = [this] { repaint(); };
        addAndMakeVisible(slider);
        addAndMakeVisible(meter);
    }
    void attach(APVTS& s, const juce::String& id, juce::UndoManager* um) {
        att_ = std::make_unique<juce::SliderParameterAttachment>(*s.getParameter(id), slider, um);
        // Make the fader travel feel like a console: more resolution near 0 dB
        slider.setSkewFactorFromMidPoint(-12.0);
    }
    void setColour(juce::Colour c) { slider.getProperties().set("accent", static_cast<juce::int64>(c.getARGB())); slider.repaint(); }

    void resized() override {
        auto r = getLocalBounds();
        r.removeFromBottom(16);
        meter.setBounds(r.removeFromRight(8).reduced(0, 6));
        r.removeFromRight(4);
        slider.setBounds(r);
    }
    void paint(juce::Graphics& g) override {
        g.setColour(slider.isMouseOverOrDragging() ? Theme::text : Theme::textDim);
        g.setFont(Theme::font(10.5f));
        g.drawFittedText(slider.getTextFromValue(slider.getValue()), getLocalBounds().removeFromBottom(14),
                         juce::Justification::centred, 1, 0.7f);
    }

    juce::Slider slider;
    Meter meter;

private:
    std::unique_ptr<juce::SliderParameterAttachment> att_;
};

class MixerStrip : public juce::Component {
public:
    MixerStrip(MutableDrumsAudioProcessor& p, int index) : proc_(p), index_(index) {
        auto& s = p.getAPVTS();
        auto* um = &p.getUndoManager();
        using namespace Params;
        const auto col = Theme::voice(index);

        eq_.attach(s, vid(index, V::eqOn), um);
        drv_.attach(s, vid(index, V::drvOn), um);
        cmp_.attach(s, vid(index, V::cmpOn), um);
        for (auto* t : { &eq_, &drv_, &cmp_ }) { t->setOnColour(col.withAlpha(0.85f)); addAndMakeVisible(t); }
        eq_.setTooltip("EQ on/off");
        drv_.setTooltip("Drive on/off");
        cmp_.setTooltip("Compressor on/off");

        send1_.attach(s, vid(index, V::send1), um);
        send2_.attach(s, vid(index, V::send2), um);
        pan_.attach(s, vid(index, V::pan), um);
        pan_.setBipolar(true);
        for (auto* k : { &send1_, &send2_, &pan_ }) addAndMakeVisible(k);

        fader_.attach(s, vid(index, V::level), um);
        fader_.setColour(col);
        addAndMakeVisible(fader_);

        mute_.attach(s, vid(index, V::mute), um);
        solo_.attach(s, vid(index, V::solo), um);
        addAndMakeVisible(mute_);
        addAndMakeVisible(solo_);

        engineAtt_ = std::make_unique<juce::ParameterAttachment>(*s.getParameter(vid(index, V::engine)),
                                                                 [this](float v) { engine_ = juce::roundToInt(v); repaint(); });
        engineAtt_->sendInitialUpdate();
    }

    std::function<void(int)> onSelect;

    void refresh() { fader_.meter.setLevel(proc_.getChannelPeak(index_)); }

    void paint(juce::Graphics& g) override {
        auto r = getLocalBounds().toFloat();
        g.setColour(Theme::panel);
        g.fillRoundedRectangle(r, 7.0f);
        g.setColour(Theme::outline);
        g.drawRoundedRectangle(r.reduced(0.5f), 7.0f, 1.0f);
        auto head = getLocalBounds().reduced(6, 6).removeFromTop(34);
        g.setColour(Theme::voice(index_));
        g.fillRoundedRectangle(head.removeFromTop(4).toFloat(), 2.0f);
        g.setColour(Theme::text);
        g.setFont(Theme::font(12.0f, true));
        g.drawText(juce::String(index_ + 1), head.removeFromTop(15), juce::Justification::centred);
        g.setColour(Theme::textDim);
        g.setFont(Theme::font(10.5f));
        g.drawFittedText(getEngineInfo(engine_).shortName, head, juce::Justification::centred, 1, 0.7f);
    }

    void resized() override {
        auto r = getLocalBounds().reduced(6, 6);
        r.removeFromTop(38);
        auto chips = r.removeFromTop(18);
        const int cw = chips.getWidth() / 3;
        eq_.setBounds(chips.removeFromLeft(cw).withTrimmedRight(1));
        drv_.setBounds(chips.removeFromLeft(cw).withTrimmedRight(1));
        cmp_.setBounds(chips);
        r.removeFromTop(6);
        send1_.setBounds(r.removeFromTop(70));
        send2_.setBounds(r.removeFromTop(70));
        pan_.setBounds(r.removeFromTop(70));
        r.removeFromTop(4);
        auto ms = r.removeFromBottom(20);
        mute_.setBounds(ms.removeFromLeft(ms.getWidth() / 2).withTrimmedRight(1));
        solo_.setBounds(ms.withTrimmedLeft(1));
        r.removeFromBottom(4);
        fader_.setBounds(r);
    }

    void mouseDown(const juce::MouseEvent&) override { if (onSelect) onSelect(index_); }

private:
    MutableDrumsAudioProcessor& proc_;
    int index_, engine_{ 0 };
    ToggleChip eq_{ "EQ" }, drv_{ "DRV" }, cmp_{ "CMP" };
    Knob send1_{ "Delay", Theme::marblesT(0) }, send2_{ "Reverb", Theme::marblesT(1) }, pan_{ "Pan", Theme::text };
    Fader fader_;
    ToggleChip mute_{ "M", Theme::danger }, solo_{ "S", Theme::accent };
    std::unique_ptr<juce::ParameterAttachment> engineAtt_;
};

// Return / master strip
class BusStrip : public juce::Component {
public:
    BusStrip(MutableDrumsAudioProcessor& p, const juce::String& name, const juce::String& paramId, juce::Colour c, int which)
        : proc_(p), name_(name), colour_(c), which_(which), fader_(c) {
        fader_.attach(p.getAPVTS(), paramId, &p.getUndoManager());
        addAndMakeVisible(fader_);
        if (which_ == 2) addAndMakeVisible(meterR_);
    }
    void refresh() {
        if (which_ == 2) {
            fader_.meter.setLevel(proc_.getMasterPeakL());
            meterR_.setLevel(proc_.getMasterPeakR());
        } else {
            fader_.meter.setLevel(proc_.getReturnPeak(which_));
        }
    }
    void paint(juce::Graphics& g) override {
        auto r = getLocalBounds().toFloat();
        g.setColour(which_ == 2 ? Theme::panelHi : Theme::panel);
        g.fillRoundedRectangle(r, 7.0f);
        g.setColour(Theme::outline);
        g.drawRoundedRectangle(r.reduced(0.5f), 7.0f, 1.0f);
        auto head = getLocalBounds().reduced(6, 6).removeFromTop(34);
        g.setColour(colour_);
        g.fillRoundedRectangle(head.removeFromTop(4).toFloat(), 2.0f);
        g.setColour(Theme::text);
        g.setFont(Theme::font(11.5f, true));
        g.drawFittedText(name_, head, juce::Justification::centred, 2, 0.8f);
    }
    void resized() override {
        auto r = getLocalBounds().reduced(6, 6);
        r.removeFromTop(44);
        r.removeFromBottom(24);
        if (which_ == 2) {
            auto m = r.removeFromRight(8).withTrimmedBottom(16).reduced(0, 6);
            meterR_.setBounds(m);
            r.removeFromRight(2);
        }
        fader_.setBounds(r);
    }

private:
    MutableDrumsAudioProcessor& proc_;
    juce::String name_;
    juce::Colour colour_;
    int which_;
    Fader fader_;
    Meter meterR_;
};

class MixerPage : public juce::Component {
public:
    explicit MixerPage(MutableDrumsAudioProcessor& p)
        : dly_(p, "DELAY RETURN", Params::ID::dlyReturn, Theme::marblesT(0), 0),
          rev_(p, "REVERB RETURN", Params::ID::revReturn, Theme::marblesT(1), 1),
          master_(p, "MASTER", Params::ID::master, Theme::accent, 2) {
        for (int i = 0; i < Params::kNumVoices; ++i) {
            auto* s = strips_.add(new MixerStrip(p, i));
            s->onSelect = [this](int idx) { if (onSelect) onSelect(idx); };
            addAndMakeVisible(s);
        }
        addAndMakeVisible(dly_);
        addAndMakeVisible(rev_);
        addAndMakeVisible(master_);
    }
    std::function<void(int)> onSelect;

    void refresh() {
        for (auto* s : strips_) s->refresh();
        dly_.refresh();
        rev_.refresh();
        master_.refresh();
    }

    void resized() override {
        auto r = getLocalBounds();
        const int gap = 5;
        const int n = strips_.size() + 3;
        const int w = (r.getWidth() - gap * (n - 1) - 8) / n;
        for (auto* s : strips_) { s->setBounds(r.removeFromLeft(w)); r.removeFromLeft(gap); }
        r.removeFromLeft(8);
        dly_.setBounds(r.removeFromLeft(w)); r.removeFromLeft(gap);
        rev_.setBounds(r.removeFromLeft(w)); r.removeFromLeft(gap);
        master_.setBounds(r);
    }

private:
    juce::OwnedArray<MixerStrip> strips_;
    BusStrip dly_, rev_, master_;
};

} // namespace MutableDrums
