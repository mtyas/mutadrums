// Mutadrums - generative drum machine
// Copyright (C) 2026 mtyas
// SPDX-License-Identifier: GPL-3.0-or-later
//
// This program is free software: you can redistribute it and/or modify it under the terms of the
// GNU General Public License as published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version. See the LICENSE file for details.

#pragma once
#include "Widgets.h"
#include "../PluginProcessor.h"

namespace Mutadrums {

// =====================================================================================
//  XY pad for the Grids topographic map
// =====================================================================================
class GridsXYPad : public juce::Component, public juce::SettableTooltipClient {
public:
    GridsXYPad(APVTS& s, juce::UndoManager* um) {
        auto* px = dynamic_cast<juce::RangedAudioParameter*>(s.getParameter(Params::ID::gridsX));
        auto* py = dynamic_cast<juce::RangedAudioParameter*>(s.getParameter(Params::ID::gridsY));
        ax_ = std::make_unique<juce::ParameterAttachment>(*px, [this](float v) { x_ = v; repaint(); }, um);
        ay_ = std::make_unique<juce::ParameterAttachment>(*py, [this](float v) { y_ = v; repaint(); }, um);
        ax_->sendInitialUpdate();
        ay_->sendInitialUpdate();
        setTooltip("Drag to morph between the 25 drum patterns of the Topo map. "
                   "The teal ring shows where Chance modulation is currently pushing it.");
        setMouseCursor(juce::MouseCursor::CrosshairCursor);
    }

    void setEffective(float ex, float ey) {
        if (std::abs(ex - ex_) > 0.002f || std::abs(ey - ey_) > 0.002f) { ex_ = ex; ey_ = ey; repaint(); }
    }

    void paint(juce::Graphics& g) override {
        auto r = getLocalBounds().toFloat();
        Theme::wellBox(g, r);
        auto a = r.reduced(10.0f);

        // 5x5 node lattice of the Grids map
        for (int i = 0; i < 5; ++i) {
            const float fx = a.getX() + a.getWidth() * i / 4.0f;
            const float fy = a.getY() + a.getHeight() * i / 4.0f;
            g.setColour(Theme::outline.withAlpha(0.7f));
            g.drawVerticalLine(juce::roundToInt(fx), a.getY(), a.getBottom());
            g.drawHorizontalLine(juce::roundToInt(fy), a.getX(), a.getRight());
        }
        g.setColour(Theme::outlineHi);
        for (int i = 0; i < 5; ++i)
            for (int j = 0; j < 5; ++j)
                g.fillEllipse(juce::Rectangle<float>(3.0f, 3.0f).withCentre({ a.getX() + a.getWidth() * i / 4.0f, a.getY() + a.getHeight() * j / 4.0f }));

        const auto pos = toScreen(a, x_, y_);
        const bool enabled = isEnabled();
        const juce::Colour acc = enabled ? Theme::accent : Theme::textFaint;

        g.setColour(acc.withAlpha(0.25f));
        g.drawVerticalLine(juce::roundToInt(pos.x), a.getY(), a.getBottom());
        g.drawHorizontalLine(juce::roundToInt(pos.y), a.getX(), a.getRight());

        if (enabled && (std::abs(ex_ - x_) > 0.01f || std::abs(ey_ - y_) > 0.01f)) {
            const auto e = toScreen(a, ex_, ey_);
            g.setColour(Theme::mod.withAlpha(0.5f));
            g.drawLine(pos.x, pos.y, e.x, e.y, 1.0f);
            g.setColour(Theme::mod);
            g.drawEllipse(juce::Rectangle<float>(10.0f, 10.0f).withCentre(e), 1.5f);
        }

        g.setColour(acc.withAlpha(0.25f));
        g.fillEllipse(juce::Rectangle<float>(22.0f, 22.0f).withCentre(pos));
        g.setColour(acc);
        g.fillEllipse(juce::Rectangle<float>(11.0f, 11.0f).withCentre(pos));

        g.setColour(Theme::textFaint);
        g.setFont(Theme::font(10.0f, true));
        g.drawText("MAP X " + juce::String(juce::roundToInt(x_ * 100)), r.reduced(8, 4).removeFromBottom(12), juce::Justification::centredRight);
        g.drawText("MAP Y " + juce::String(juce::roundToInt(y_ * 100)), r.reduced(8, 4).removeFromTop(12), juce::Justification::centredLeft);
        if (!enabled) {
            g.setColour(Theme::textDim);
            g.setFont(Theme::font(11.0f, true));
            g.drawText("Map not used in Euclidean mode", r, juce::Justification::centred);
        }
    }

    void mouseDown(const juce::MouseEvent& e) override {
        ax_->beginGesture();
        ay_->beginGesture();
        dragging_ = true;
        setFrom(e.position);
    }
    void mouseDrag(const juce::MouseEvent& e) override { if (dragging_) setFrom(e.position); }
    void mouseUp(const juce::MouseEvent&) override {
        if (!dragging_) return;
        dragging_ = false;
        ax_->endGesture();
        ay_->endGesture();
    }
    void mouseDoubleClick(const juce::MouseEvent&) override {
        ax_->setValueAsCompleteGesture(0.5f);
        ay_->setValueAsCompleteGesture(0.5f);
    }

private:
    static juce::Point<float> toScreen(juce::Rectangle<float> a, float x, float y) {
        return { a.getX() + x * a.getWidth(), a.getBottom() - y * a.getHeight() };
    }
    void setFrom(juce::Point<float> p) {
        auto a = getLocalBounds().toFloat().reduced(10.0f);
        const float x = juce::jlimit(0.0f, 1.0f, (p.x - a.getX()) / a.getWidth());
        const float y = juce::jlimit(0.0f, 1.0f, (a.getBottom() - p.y) / a.getHeight());
        ax_->setValueAsPartOfGesture(x);
        ay_->setValueAsPartOfGesture(y);
    }

    std::unique_ptr<juce::ParameterAttachment> ax_, ay_;
    float x_{ 0.5f }, y_{ 0.5f }, ex_{ 0.5f }, ey_{ 0.5f };
    bool dragging_{ false };
};

// =====================================================================================
//  32-step preview of what Grids will play (BD / SD / HH lanes) with a playhead
// =====================================================================================
class GridsStepView : public juce::Component, public juce::SettableTooltipClient {
public:
    explicit GridsStepView(APVTS& s) : state_(s) {
        setTooltip("Pattern preview for the current map position and densities. "
                   "Bright cells are accents. Chaos and Chance modulation add variations on top.");
    }

    void update(int step, bool running, const std::array<bool, 3>& hits) {
        bool changed = (step != step_) || (running != running_);
        step_ = step;
        running_ = running;
        for (int i = 0; i < 3; ++i) {
            if (hits[static_cast<size_t>(i)]) { flash_[static_cast<size_t>(i)] = 1.0f; changed = true; }
            else if (flash_[static_cast<size_t>(i)] > 0.0f) { flash_[static_cast<size_t>(i)] = juce::jmax(0.0f, flash_[static_cast<size_t>(i)] - 0.12f); changed = true; }
        }
        if (changed || ++frame_ % 6 == 0) repaint(); // also refresh preview when params move
    }

    void paint(juce::Graphics& g) override {
        auto r = getLocalBounds().toFloat();
        Theme::wellBox(g, r);
        auto a = r.reduced(6.0f, 6.0f);
        const float labelW = 26.0f;
        auto labels = a.removeFromLeft(labelW);
        const float rowH = a.getHeight() / 3.0f;
        const float cellW = a.getWidth() / 32.0f;

        auto P = [this](const juce::String& id) { return state_.getRawParameterValue(id)->load(); };
        const float x = P(Params::ID::gridsX), y = P(Params::ID::gridsY);
        const bool euclid = P(Params::ID::gridsMode) > 0.5f;
        const float dens[3] = { P(Params::ID::gridsDensBD), P(Params::ID::gridsDensSD), P(Params::ID::gridsDensHH) };
        const int lens[3] = { juce::roundToInt(P(Params::ID::gridsLenBD)), juce::roundToInt(P(Params::ID::gridsLenSD)), juce::roundToInt(P(Params::ID::gridsLenHH)) };
        const auto x8 = static_cast<uint8_t>(juce::jlimit(0.0f, 255.0f, x * 255.0f));
        const auto y8 = static_cast<uint8_t>(juce::jlimit(0.0f, 255.0f, y * 255.0f));

        // Beat markers
        for (int s = 0; s < 32; s += 4) {
            g.setColour(s % 16 == 0 ? Theme::outlineHi : Theme::outline.withAlpha(0.6f));
            g.drawVerticalLine(juce::roundToInt(a.getX() + s * cellW), a.getY(), a.getBottom());
        }
        // Playhead
        if (running_) {
            g.setColour(Theme::text.withAlpha(0.10f));
            g.fillRect(a.getX() + step_ * cellW, a.getY(), cellW, a.getHeight());
        }

        static const char* names[3] = { "BD", "SD", "HH" };
        for (int i = 0; i < 3; ++i) {
            const auto col = Theme::gridsInst(i);
            auto lr = labels.withY(a.getY() + i * rowH).withHeight(rowH);
            g.setColour(col.interpolatedWith(juce::Colours::white, flash_[static_cast<size_t>(i)] * 0.6f));
            g.setFont(Theme::font(10.0f, true));
            g.drawText(names[i], lr, juce::Justification::centredLeft);

            const int dens8 = juce::jlimit(0, 255, static_cast<int>(dens[i] * 255.0f));
            for (int s = 0; s < 32; ++s) {
                bool hit = false, accent = false;
                if (euclid) {
                    hit = GridsSequencer::euclideanHit(lens[i], dens[i], s);
                    accent = hit && (s % juce::jmax(1, lens[i]) == 0);
                } else {
                    const uint8_t level = GridsSequencer::readDrumMap(static_cast<uint8_t>(s), static_cast<uint8_t>(i), x8, y8);
                    hit = dens8 > 0 && level > 255 - dens8;
                    accent = hit && level > 192;
                }
                auto cell = juce::Rectangle<float>(a.getX() + s * cellW, a.getY() + i * rowH, cellW, rowH).reduced(1.2f, 2.5f);
                if (hit) {
                    float alpha = accent ? 1.0f : 0.55f;
                    if (running_ && s == step_) alpha = 1.0f;
                    g.setColour(col.withAlpha(alpha));
                    g.fillRoundedRectangle(cell, 2.0f);
                    if (running_ && s == step_) {
                        g.setColour(juce::Colours::white.withAlpha(0.6f * flash_[static_cast<size_t>(i)] + 0.2f));
                        g.drawRoundedRectangle(cell, 2.0f, 1.2f);
                    }
                } else {
                    g.setColour(Theme::panelHi);
                    g.fillRoundedRectangle(cell.reduced(cell.getWidth() * 0.3f, cell.getHeight() * 0.3f), 1.5f);
                }
            }
        }
    }

private:
    APVTS& state_;
    int step_{ 0 };
    bool running_{ false };
    int frame_{ 0 };
    std::array<float, 3> flash_{};
};

// =====================================================================================
//  Marbles display: t gates, X voltages and the Déjà-Vu loop
// =====================================================================================
class MarblesDisplay : public juce::Component, public juce::SettableTooltipClient {
public:
    explicit MarblesDisplay(APVTS& s) : state_(s) {
        setTooltip("t1-t3: random gate outputs (use them as voice triggers).  X1-X3: random voltages, "
                   "used as modulation sources.  The ring shows the Deja-Vu loop: higher Deja-Vu repeats it more.");
    }

    void update(const std::array<bool, 3>& hits, const std::array<float, 3>& xs, int loopIndex) {
        for (int i = 0; i < 3; ++i) {
            auto& f = flash_[static_cast<size_t>(i)];
            f = hits[static_cast<size_t>(i)] ? 1.0f : juce::jmax(0.0f, f - 0.1f);
            auto& xv = x_[static_cast<size_t>(i)];
            xv += (xs[static_cast<size_t>(i)] - xv) * 0.5f;
        }
        loopIndex_ = loopIndex;
        repaint();
    }

    void paint(juce::Graphics& g) override {
        auto r = getLocalBounds().toFloat();
        Theme::wellBox(g, r);
        auto a = r.reduced(10.0f, 8.0f);

        // --- t lamps
        auto tArea = a.removeFromLeft(a.getWidth() * 0.34f);
        const float lampW = tArea.getWidth() / 3.0f;
        for (int i = 0; i < 3; ++i) {
            auto cell = tArea.withX(tArea.getX() + i * lampW).withWidth(lampW);
            const auto col = Theme::marblesT(i);
            auto lamp = juce::Rectangle<float>(26.0f, 26.0f).withCentre({ cell.getCentreX(), cell.getCentreY() - 6.0f });
            const float f = flash_[static_cast<size_t>(i)];
            g.setColour(col.withAlpha(0.12f + 0.25f * f));
            g.fillEllipse(lamp.expanded(5.0f * f));
            g.setColour(col.withAlpha(0.25f + 0.75f * f));
            g.fillEllipse(lamp);
            g.setColour(Theme::textDim);
            g.setFont(Theme::font(10.0f, true));
            g.drawText("t" + juce::String(i + 1), cell.withTop(lamp.getBottom() + 3.0f).withHeight(12.0f), juce::Justification::centred);
        }

        // --- X bars
        a.removeFromLeft(8.0f);
        auto xArea = a.removeFromLeft(a.getWidth() * 0.45f);
        const float barW = xArea.getWidth() / 3.0f;
        for (int i = 0; i < 3; ++i) {
            auto col = xArea.withX(xArea.getX() + i * barW).withWidth(barW).reduced(barW * 0.28f, 0.0f);
            auto track = col.withTrimmedBottom(16.0f);
            g.setColour(Theme::panelHi);
            g.fillRoundedRectangle(track, 3.0f);
            const float v = juce::jlimit(0.0f, 1.0f, x_[static_cast<size_t>(i)]);
            g.setColour(Theme::mod.withAlpha(0.85f));
            g.fillRoundedRectangle(track.withTop(track.getBottom() - track.getHeight() * v), 3.0f);
            g.setColour(Theme::textDim);
            g.setFont(Theme::font(10.0f, true));
            g.drawText("X" + juce::String(i + 1), col.withTop(track.getBottom() + 3.0f), juce::Justification::centred);
        }

        // --- Déjà-Vu ring
        a.removeFromLeft(6.0f);
        const float d = juce::jmin(a.getWidth(), a.getHeight());
        auto ring = a.withSizeKeepingCentre(d, d).reduced(4.0f);
        const int len = juce::roundToInt(state_.getRawParameterValue(Params::ID::marblesLength)->load());
        const float dv = state_.getRawParameterValue(Params::ID::marblesDejaVu)->load();
        const auto c = ring.getCentre();
        const float rad = ring.getWidth() * 0.5f - 3.0f;
        for (int i = 0; i < 16; ++i) {
            const float ang = juce::MathConstants<float>::twoPi * i / 16.0f - juce::MathConstants<float>::halfPi;
            const juce::Point<float> p(c.x + std::cos(ang) * rad, c.y + std::sin(ang) * rad);
            const bool inLoop = i < len;
            const bool cur = i == loopIndex_;
            g.setColour(cur ? Theme::accent : (inLoop ? Theme::accent.withAlpha(0.2f + 0.6f * dv) : Theme::outline));
            g.fillEllipse(juce::Rectangle<float>(cur ? 7.0f : 4.5f, cur ? 7.0f : 4.5f).withCentre(p));
        }
        g.setColour(Theme::textDim);
        g.setFont(Theme::font(9.5f, true));
        g.drawText("DEJA", ring.withSizeKeepingCentre(ring.getWidth(), 12.0f).translated(0, -6.0f), juce::Justification::centred);
        g.setColour(Theme::text);
        g.drawText(juce::String(juce::roundToInt(dv * 100)) + "%", ring.withSizeKeepingCentre(ring.getWidth(), 12.0f).translated(0, 6.0f), juce::Justification::centred);
    }

private:
    APVTS& state_;
    std::array<float, 3> flash_{}, x_{ 0.5f, 0.5f, 0.5f };
    int loopIndex_{ 0 };
};

// =====================================================================================
//  PatternPanel — Grids (top) + Marbles (bottom)
// =====================================================================================
class PatternPanel : public juce::Component {
public:
    explicit PatternPanel(MutadrumsAudioProcessor& p)
        : proc_(p), state_(p.getAPVTS()), xy_(state_, &p.getUndoManager()), steps_(state_), marbles_(state_) {
        auto* um = &p.getUndoManager();
        using namespace Params;

        addAndMakeVisible(mode_);
        mode_.attach(state_, ID::gridsMode, um, { "Map", "Euclidean" });
        addAndMakeVisible(xy_);
        addAndMakeVisible(steps_);

        auto knob = [&](Knob& k, const juce::String& id, const juce::String& tip, juce::Colour c = Theme::accent) {
            addAndMakeVisible(k);
            k.attach(state_, id, um);
            k.setAccent(c);
            if (tip.isNotEmpty()) k.setTooltipText(tip);
        };
        knob(chaos_, ID::gridsChaos, "Random variations added to the pattern each bar.");
        knob(swing_, ID::gridsSwing, "Delays every second 16th note for shuffle / swing feel.");
        knob(lenBD_, ID::gridsLenBD, "Euclidean loop length for the BD lane.", Theme::gridsInst(0));
        knob(lenSD_, ID::gridsLenSD, "Euclidean loop length for the SD lane.", Theme::gridsInst(1));
        knob(lenHH_, ID::gridsLenHH, "Euclidean loop length for the HH lane.", Theme::gridsInst(2));
        knob(densBD_, ID::gridsDensBD, "How many BD hits fire. Turn up for busier kicks.", Theme::gridsInst(0));
        knob(densSD_, ID::gridsDensSD, "How many SD hits fire.", Theme::gridsInst(1));
        knob(densHH_, ID::gridsDensHH, "How many HH hits fire.", Theme::gridsInst(2));

        addAndMakeVisible(model_);
        model_.attach(state_, ID::marblesModel, um);
        model_.combo.setTooltip("Coin Toss: t2 is a steady clock, t1/t3 flip a biased coin.\n"
                                "Polyrhythm: 3-against-4 cross rhythms.\nDrums: generative kick/snare/hat roles.");
        addAndMakeVisible(marbles_);
        knob(mRate_, ID::marblesRate, "Chance clock relative to 16th notes.", Theme::marblesT(0));
        knob(mJitter_, ID::marblesJitter, "Humanise: randomly delays Chance hits.", Theme::marblesT(1));
        knob(mBias_, ID::marblesBias, "Tilts the coin toss towards t1 or t3, and the X voltages low/high.", Theme::marblesT(2));
        knob(mDeja_, ID::marblesDejaVu, "0% = always new random decisions. 100% = locked loop that repeats.", Theme::accent);
        knob(mLen_, ID::marblesLength, "Length of the Deja-Vu loop in steps.", Theme::accent);

        knob(mX_, ID::modGridsX, "Chance X1 moves the map X position.", Theme::mod);
        knob(mY_, ID::modGridsY, "Chance X2 moves the map Y position.", Theme::mod);
        knob(mC_, ID::modGridsChaos, "Chance X3 adds/removes chaos.", Theme::mod);
        knob(mD_, ID::modGridsDens, "Chance t1 gate pushes all densities up when it fires.", Theme::mod);
        for (auto* k : { &mX_, &mY_, &mC_, &mD_ }) k->setBipolar(true);

        modeAttachment_ = std::make_unique<juce::ParameterAttachment>(
            *state_.getParameter(ID::gridsMode), [this](float v) { setEuclid(v > 0.5f); }, nullptr);
        modeAttachment_->sendInitialUpdate();
    }

    void refresh() {
        std::array<bool, 3> gh{}, mh{};
        for (int i = 0; i < 3; ++i) {
            const auto g = proc_.gridsHitCount[static_cast<size_t>(i)].load();
            gh[static_cast<size_t>(i)] = g != lastG_[static_cast<size_t>(i)];
            lastG_[static_cast<size_t>(i)] = g;
            const auto m = proc_.marblesHitCount[static_cast<size_t>(i)].load();
            mh[static_cast<size_t>(i)] = m != lastM_[static_cast<size_t>(i)];
            lastM_[static_cast<size_t>(i)] = m;
        }
        const bool running = proc_.clockRunning.load();
        steps_.update(proc_.currentStep.load(), running, gh);
        xy_.setEffective(proc_.effectiveGridsX.load(), proc_.effectiveGridsY.load());
        marbles_.update(mh, { proc_.marblesX[0].load(), proc_.marblesX[1].load(), proc_.marblesX[2].load() },
                        proc_.marblesStep.load());
    }

    void paint(juce::Graphics& g) override {
        Theme::panelBox(g, gridsArea_.toFloat(), "Topo - pattern generator");
        Theme::panelBox(g, marblesArea_.toFloat(), "Chance - random generator");
        g.setColour(Theme::textFaint);
        g.setFont(Theme::font(9.5f, true));
        g.drawText("CHANCE MODULATES TOPO", modLabel_, juce::Justification::centredLeft);
        g.setColour(Theme::outline);
        g.drawHorizontalLine(modLabel_.getCentreY(), static_cast<float>(modLabel_.getX() + 140), static_cast<float>(modLabel_.getRight()));
    }

    void resized() override {
        auto r = getLocalBounds();
        gridsArea_ = r.removeFromTop(404);
        r.removeFromTop(10);
        marblesArea_ = r;

        // ---- Grids
        auto g = gridsArea_.reduced(12, 0);
        auto head = g.removeFromTop(32);
        mode_.setBounds(head.removeFromRight(170).reduced(0, 5));
        auto body = g.removeFromTop(186);
        xy_.setBounds(body.removeFromLeft(186).reduced(0, 3));
        body.removeFromLeft(10);
        auto top = body.removeFromTop(body.getHeight() / 2);
        const int kw = top.getWidth() / 2;
        chaos_.setBounds(top.removeFromLeft(kw));
        swing_.setBounds(top);
        const int lw = body.getWidth() / 3;
        lenBD_.setBounds(body.removeFromLeft(lw));
        lenSD_.setBounds(body.removeFromLeft(lw));
        lenHH_.setBounds(body);
        g.removeFromTop(8);
        steps_.setBounds(g.removeFromTop(78));
        g.removeFromTop(6);
        auto dens = g.removeFromTop(86);
        const int dw = dens.getWidth() / 3;
        densBD_.setBounds(dens.removeFromLeft(dw));
        densSD_.setBounds(dens.removeFromLeft(dw));
        densHH_.setBounds(dens);

        // ---- Marbles
        auto m = marblesArea_.reduced(12, 0);
        auto mh = m.removeFromTop(32);
        model_.setBounds(mh.removeFromRight(150).reduced(0, 5));
        marbles_.setBounds(m.removeFromTop(100));
        m.removeFromTop(6);
        auto row = m.removeFromTop(80);
        const int w5 = row.getWidth() / 5;
        for (auto* k : { &mRate_, &mJitter_, &mBias_, &mDeja_ }) k->setBounds(row.removeFromLeft(w5));
        mLen_.setBounds(row);
        modLabel_ = m.removeFromTop(16);
        auto row2 = m.removeFromTop(78);
        const int w4 = row2.getWidth() / 4;
        for (auto* k : { &mX_, &mY_, &mC_ }) k->setBounds(row2.removeFromLeft(w4));
        mD_.setBounds(row2);
    }

private:
    void setEuclid(bool e) {
        xy_.setEnabled(!e);
        chaos_.setEnabled(!e);
        lenBD_.setEnabled(e);
        lenSD_.setEnabled(e);
        lenHH_.setEnabled(e);
        repaint();
    }

    MutadrumsAudioProcessor& proc_;
    APVTS& state_;
    juce::Rectangle<int> gridsArea_, marblesArea_, modLabel_;

    Segmented mode_;
    GridsXYPad xy_;
    GridsStepView steps_;
    Knob chaos_{ "Chaos" }, swing_{ "Swing" };
    Knob lenBD_{ "BD Len" }, lenSD_{ "SD Len" }, lenHH_{ "HH Len" };
    Knob densBD_{ "BD Density" }, densSD_{ "SD Density" }, densHH_{ "HH Density" };

    ChoiceBox model_;
    MarblesDisplay marbles_;
    Knob mRate_{ "Rate" }, mJitter_{ "Jitter" }, mBias_{ "Bias" }, mDeja_{ "Deja Vu" }, mLen_{ "Loop" };
    Knob mX_{ "X1>Map X" }, mY_{ "X2>Map Y" }, mC_{ "X3>Chaos" }, mD_{ "t1>Density" };

    std::unique_ptr<juce::ParameterAttachment> modeAttachment_;
    std::array<uint32_t, 3> lastG_{}, lastM_{};
};

} // namespace Mutadrums
