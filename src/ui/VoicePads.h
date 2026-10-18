#pragma once
#include "Widgets.h"
#include "../PluginProcessor.h"

namespace MutableDrums {

// =====================================================================================
//  One pad per voice: click to select + audition. Shows engine, trigger, activity, M/S.
// =====================================================================================
class VoicePad : public juce::Component, public juce::SettableTooltipClient {
public:
    VoicePad(MutableDrumsAudioProcessor& p, int index) : proc_(p), index_(index) {
        auto& s = p.getAPVTS();
        auto* um = &p.getUndoManager();
        mute_.attach(s, Params::vid(index, Params::V::mute), um);
        solo_.attach(s, Params::vid(index, Params::V::solo), um);
        mute_.setTooltip("Mute");
        solo_.setTooltip("Solo");
        addAndMakeVisible(mute_);
        addAndMakeVisible(solo_);
        engineAtt_ = std::make_unique<juce::ParameterAttachment>(*s.getParameter(Params::vid(index, Params::V::engine)),
                                                                 [this](float v) { engine_ = juce::roundToInt(v); repaint(); });
        trigAtt_ = std::make_unique<juce::ParameterAttachment>(*s.getParameter(Params::vid(index, Params::V::trig)),
                                                               [this](float v) { trig_ = juce::roundToInt(v); repaint(); });
        engineAtt_->sendInitialUpdate();
        trigAtt_->sendInitialUpdate();
        setTooltip("Click to select and play. Use the M / S buttons to mute or solo.");
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
    }

    std::function<void(int)> onSelect;

    void setSelected(bool s) { if (s != selected_) { selected_ = s; repaint(); } }

    void refresh() {
        const auto hits = proc_.voiceHitCount[static_cast<size_t>(index_)].load();
        if (hits != lastHits_) { lastHits_ = hits; flash_ = 1.0f; }
        else flash_ = juce::jmax(0.0f, flash_ - 0.09f);
        meter_ = juce::jmax(proc_.getChannelPeak(index_), meter_ * 0.85f);
        repaint();
    }

    void paint(juce::Graphics& g) override {
        auto r = getLocalBounds().toFloat().reduced(1.0f);
        const auto col = Theme::voice(index_);
        g.setColour(selected_ ? Theme::panelHi.brighter(0.05f) : Theme::panel);
        g.fillRoundedRectangle(r, 8.0f);
        if (flash_ > 0.0f) {
            g.setColour(col.withAlpha(0.22f * flash_));
            g.fillRoundedRectangle(r, 8.0f);
        }
        g.setColour(selected_ ? col : Theme::outline);
        g.drawRoundedRectangle(r.reduced(0.5f), 8.0f, selected_ ? 2.0f : 1.0f);

        // Colour tab
        auto top = r.reduced(8.0f, 7.0f);
        g.setColour(col);
        g.fillRoundedRectangle(top.removeFromTop(4.0f), 2.0f);
        top.removeFromTop(6.0f);

        // Number + meter
        auto line1 = top.removeFromTop(16.0f);
        g.setColour(Theme::textDim);
        g.setFont(Theme::font(11.0f, true));
        g.drawText(juce::String(index_ + 1), line1, juce::Justification::centredLeft);
        auto meter = line1.removeFromRight(28.0f).withSizeKeepingCentre(28.0f, 4.0f);
        g.setColour(Theme::well);
        g.fillRoundedRectangle(meter, 2.0f);
        const float db = meter_ > 0.00001f ? juce::Decibels::gainToDecibels(meter_) : -100.0f;
        g.setColour(col);
        g.fillRoundedRectangle(meter.withWidth(meter.getWidth() * juce::jlimit(0.0f, 1.0f, (db + 48.0f) / 48.0f)), 2.0f);

        // Engine name
        const auto& info = getEngineInfo(engine_);
        g.setColour(Theme::text);
        g.setFont(Theme::font(13.5f, true));
        g.drawFittedText(info.shortName, top.removeFromTop(18.0f).toNearestInt(), juce::Justification::centredLeft, 1, 0.8f);

        // Trigger source
        g.setColour(trig_ == 0 ? Theme::textFaint : Theme::textDim);
        g.setFont(Theme::font(10.5f));
        g.drawFittedText(trig_ == 0 ? "MIDI / pad" : getTriggerSourceName(trig_), top.removeFromTop(14.0f).toNearestInt(),
                         juce::Justification::centredLeft, 1, 0.8f);
    }

    void resized() override {
        auto r = getLocalBounds().reduced(7, 7);
        auto bottom = r.removeFromBottom(18);
        mute_.setBounds(bottom.removeFromLeft(bottom.getWidth() / 2).withTrimmedRight(2));
        solo_.setBounds(bottom.withTrimmedLeft(2));
    }

    void mouseDown(const juce::MouseEvent& e) override {
        if (onSelect) onSelect(index_);
        if (!e.mods.isPopupMenu()) proc_.triggerVoiceManual(index_);
    }

private:
    MutableDrumsAudioProcessor& proc_;
    int index_;
    int engine_{ 0 }, trig_{ 0 };
    bool selected_{ false };
    float flash_{ 0.0f }, meter_{ 0.0f };
    uint32_t lastHits_{ 0 };
    ToggleChip mute_{ "M", Theme::danger }, solo_{ "S", Theme::accent };
    std::unique_ptr<juce::ParameterAttachment> engineAtt_, trigAtt_;
};

class VoicePads : public juce::Component {
public:
    explicit VoicePads(MutableDrumsAudioProcessor& p) {
        for (int i = 0; i < Params::kNumVoices; ++i) {
            auto* pad = pads_.add(new VoicePad(p, i));
            pad->onSelect = [this](int idx) { if (onSelect) onSelect(idx); };
            addAndMakeVisible(pad);
        }
    }
    std::function<void(int)> onSelect;

    void setSelected(int idx) { for (int i = 0; i < pads_.size(); ++i) pads_[i]->setSelected(i == idx); }
    void refresh() { for (auto* p : pads_) p->refresh(); }

    void resized() override {
        auto r = getLocalBounds();
        const int gap = 6;
        const int w = (r.getWidth() - gap * (pads_.size() - 1)) / pads_.size();
        for (int i = 0; i < pads_.size(); ++i) {
            pads_[i]->setBounds(i == pads_.size() - 1 ? r : r.removeFromLeft(w));
            r.removeFromLeft(gap);
        }
    }

private:
    juce::OwnedArray<VoicePad> pads_;
};

} // namespace MutableDrums
