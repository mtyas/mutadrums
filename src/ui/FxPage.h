#pragma once
#include "Widgets.h"
#include "../PluginProcessor.h"

namespace MutableDrums {

// Echo-train picture: bars above the line = left, below = right
class DelayViz : public juce::Component {
public:
    DelayViz(MutableDrumsAudioProcessor& p) : proc_(p) { setInterceptsMouseClicks(false, false); }
    void paint(juce::Graphics& g) override {
        auto r = getLocalBounds().toFloat();
        Theme::wellBox(g, r);
        auto a = r.reduced(12.0f, 10.0f);
        auto& s = proc_.getAPVTS();
        const float fb = s.getRawParameterValue(Params::ID::dlyFeedback)->load();
        const float w = s.getRawParameterValue(Params::ID::dlyPingPong)->load();
        const float tone = s.getRawParameterValue(Params::ID::dlyTone)->load();
        const float ms = proc_.delayTimeMs.load();
        const float mid = a.getCentreY();
        const float half = a.getHeight() * 0.5f - 4.0f;

        g.setColour(Theme::outline);
        g.drawHorizontalLine(juce::roundToInt(mid), a.getX(), a.getRight());
        g.setColour(Theme::textFaint);
        g.setFont(Theme::font(9.5f, true));
        g.drawText("L", a.withWidth(12.0f).withHeight(12.0f), juce::Justification::centredLeft);
        g.drawText("R", a.withWidth(12.0f).withHeight(12.0f).withBottomY(a.getBottom()), juce::Justification::centredLeft);

        const int maxEchoes = 14;
        const float spacing = (a.getWidth() - 20.0f) / static_cast<float>(maxEchoes);
        float L = 1.0f, R = 1.0f - w;
        // dry hit
        g.setColour(Theme::text);
        g.fillRoundedRectangle(a.getX() + 14.0f, mid - half, 3.0f, half * 2.0f, 1.5f);
        for (int n = 1; n <= maxEchoes; ++n) {
            const float x = a.getX() + 14.0f + n * spacing;
            const float darken = juce::jlimit(0.0f, 1.0f, (1.0f - tone) * 0.12f * n);
            const auto col = Theme::marblesT(0).interpolatedWith(Theme::textFaint, darken);
            g.setColour(col);
            if (L > 0.01f) g.fillRoundedRectangle(x, mid - half * L, 4.0f, half * L, 2.0f);
            g.setColour(col.withMultipliedAlpha(0.8f));
            if (R > 0.01f) g.fillRoundedRectangle(x, mid, 4.0f, half * R, 2.0f);
            const float nl = fb * ((1.0f - w) * L + w * R);
            const float nr = fb * ((1.0f - w) * R + w * L);
            L = nl; R = nr;
        }
        g.setColour(Theme::text);
        g.setFont(Theme::font(12.0f, true));
        g.drawText(Params::msText(ms, 0), a.removeFromTop(16.0f), juce::Justification::centredRight);
    }
private:
    MutableDrumsAudioProcessor& proc_;
};

// Decay-envelope picture for the reverb
class ReverbViz : public juce::Component {
public:
    ReverbViz(MutableDrumsAudioProcessor& p) : proc_(p) { setInterceptsMouseClicks(false, false); }
    void paint(juce::Graphics& g) override {
        auto r = getLocalBounds().toFloat();
        Theme::wellBox(g, r);
        auto a = r.reduced(12.0f, 10.0f);
        auto& s = proc_.getAPVTS();
        const float rt = FxReverb::decayToSeconds(s.getRawParameterValue(Params::ID::revDecay)->load());
        const float bright = s.getRawParameterValue(Params::ID::revBright)->load();
        const float pre = s.getRawParameterValue(Params::ID::revPreDelay)->load() * 0.001f;
        const float width = s.getRawParameterValue(Params::ID::revWidth)->load();
        const float span = juce::jmax(1.5f, (rt + pre) * 1.15f);

        // second gridlines
        g.setColour(Theme::outline);
        g.setFont(Theme::font(9.5f));
        const float stepS = span > 8.0f ? 2.0f : (span > 3.0f ? 1.0f : 0.5f);
        for (float t = stepS; t < span; t += stepS) {
            const float x = a.getX() + a.getWidth() * t / span;
            g.drawVerticalLine(juce::roundToInt(x), a.getY(), a.getBottom());
        }

        auto envelope = [&](float spread) {
            juce::Path p;
            p.startNewSubPath(a.getX(), a.getBottom());
            const int n = 160;
            for (int i = 0; i <= n; ++i) {
                const float t = span * i / n;
                float db = -60.0f;
                if (t >= pre) db = -60.0f * (t - pre) / rt;
                const float h = juce::jlimit(0.0f, 1.0f, (db + 60.0f) / 60.0f) * (1.0f - spread);
                p.lineTo(a.getX() + a.getWidth() * t / span, a.getBottom() - h * a.getHeight());
            }
            p.lineTo(a.getRight(), a.getBottom());
            p.closeSubPath();
            return p;
        };
        const auto col = Theme::marblesT(1).interpolatedWith(juce::Colour(0xffbfe3ff), bright * 0.6f);
        g.setColour(col.withAlpha(0.12f + 0.15f * width));
        g.fillPath(envelope(0.0f));
        g.setColour(col.withAlpha(0.45f));
        g.fillPath(envelope(0.18f * width));
        g.setColour(Theme::text);
        g.setFont(Theme::font(12.0f, true));
        g.drawText("RT60  " + juce::String(rt, 1) + " s", a.removeFromTop(16.0f), juce::Justification::centredRight);
    }
private:
    MutableDrumsAudioProcessor& proc_;
};

class FxPage : public juce::Component {
public:
    explicit FxPage(MutableDrumsAudioProcessor& p) : delayViz_(p), reverbViz_(p) {
        auto& s = p.getAPVTS();
        auto* um = &p.getUndoManager();
        using namespace Params;

        sync_.attach(s, ID::dlySync, um);
        sync_.setTooltip("Sync the delay time to the tempo");
        div_.attach(s, ID::dlyDiv, um);
        div_.combo.setTooltip("Note value of one echo (D = dotted, T = triplet)");
        auto knob = [&](Knob& k, const juce::String& id, juce::Colour c, const juce::String& tip) {
            k.attach(s, id, um);
            k.setAccent(c);
            k.setTooltipText(tip);
            addAndMakeVisible(k);
        };
        const auto dc = Theme::marblesT(0), rc = Theme::marblesT(1);
        knob(time_, ID::dlyTime, dc, "Delay time in milliseconds (when Sync is off)");
        knob(fb_, ID::dlyFeedback, dc, "How many repeats. High values give long dub trails - it never runs away.");
        knob(tone_, ID::dlyTone, dc, "Brightness of the repeats; each repeat gets darker when this is low.");
        knob(ping_, ID::dlyPingPong, dc, "0% = repeats in the centre, 100% = repeats bounce left / right");
        knob(tape_, ID::dlyTape, dc, "Tape colour: soft saturation and a gentle wobble on the repeats");
        knob(dRet_, ID::dlyReturn, dc, "Delay return level");
        knob(decay_, ID::revDecay, rc, "Reverb time (RT60)");
        knob(bright_, ID::revBright, rc, "Dark (absorbent room) to bright (metal plate)");
        knob(pre_, ID::revPreDelay, rc, "Gap before the reverb starts; keeps drum attacks clear");
        knob(width_, ID::revWidth, rc, "Stereo width of the reverb");
        knob(rRet_, ID::revReturn, rc, "Reverb return level");

        for (auto* c : std::initializer_list<juce::Component*>{ &sync_, &div_, &delayViz_, &reverbViz_ }) addAndMakeVisible(c);

        syncWatch_ = std::make_unique<juce::ParameterAttachment>(*s.getParameter(ID::dlySync), [this](float v) {
            const bool synced = v > 0.5f;
            div_.setVisible(synced);
            time_.setVisible(!synced);
        });
        syncWatch_->sendInitialUpdate();
    }

    void refresh() {
        if (++frame_ % 3 == 0) { delayViz_.repaint(); reverbViz_.repaint(); }
    }

    void paint(juce::Graphics& g) override {
        Theme::panelBox(g, dlyArea_.toFloat(), "Delay  -  send A", Theme::marblesT(0));
        Theme::panelBox(g, revArea_.toFloat(), "Reverb  -  send B", Theme::marblesT(1));
        g.setColour(Theme::textFaint);
        g.setFont(Theme::font(11.5f));
        g.drawFittedText("Send voices to the effects with the Delay / Reverb knobs on the Voice or Mixer page.",
                         hintArea_, juce::Justification::centred, 2);
    }

    void resized() override {
        auto r = getLocalBounds();
        hintArea_ = r.removeFromBottom(26);
        r.removeFromBottom(6);
        dlyArea_ = r.removeFromLeft((r.getWidth() - 10) / 2);
        r.removeFromLeft(10);
        revArea_ = r;

        {
            auto a = dlyArea_.reduced(12, 0);
            auto head = a.removeFromTop(34);
            sync_.setBounds(head.removeFromRight(64).reduced(0, 6));
            delayViz_.setBounds(a.removeFromTop(170));
            a.removeFromTop(14);
            auto row = a.removeFromTop(98);
            const int w = row.getWidth() / 3;
            auto timeCell = row.removeFromLeft(w);
            time_.setBounds(timeCell);
            div_.setBounds(timeCell.withSizeKeepingCentre(timeCell.getWidth() - 20, 46));
            fb_.setBounds(row.removeFromLeft(w));
            tone_.setBounds(row);
            a.removeFromTop(16);
            auto row2 = a.removeFromTop(98);
            ping_.setBounds(row2.removeFromLeft(w));
            tape_.setBounds(row2.removeFromLeft(w));
            dRet_.setBounds(row2);
        }
        {
            auto a = revArea_.reduced(12, 0);
            a.removeFromTop(34);
            reverbViz_.setBounds(a.removeFromTop(170));
            a.removeFromTop(14);
            auto row = a.removeFromTop(98);
            const int w = row.getWidth() / 3;
            decay_.setBounds(row.removeFromLeft(w));
            bright_.setBounds(row.removeFromLeft(w));
            pre_.setBounds(row);
            a.removeFromTop(16);
            auto row2 = a.removeFromTop(98);
            width_.setBounds(row2.removeFromLeft(w).translated(w / 2, 0));
            rRet_.setBounds(row2.removeFromLeft(w).translated(w / 2, 0));
        }
    }

private:
    juce::Rectangle<int> dlyArea_, revArea_, hintArea_;
    ToggleChip sync_{ "SYNC", Theme::marblesT(0) };
    ChoiceBox div_{ "Division" };
    Knob time_{ "Time" }, fb_{ "Feedback" }, tone_{ "Tone" }, ping_{ "Ping-Pong" }, tape_{ "Tape" }, dRet_{ "Return" };
    Knob decay_{ "Decay" }, bright_{ "Brightness" }, pre_{ "Pre-Delay" }, width_{ "Width" }, rRet_{ "Return" };
    DelayViz delayViz_;
    ReverbViz reverbViz_;
    std::unique_ptr<juce::ParameterAttachment> syncWatch_;
    int frame_{ 0 };
};

} // namespace MutableDrums
