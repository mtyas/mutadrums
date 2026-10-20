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

class HeaderBar : public juce::Component {
public:
    explicit HeaderBar(MutadrumsAudioProcessor& p) : proc_(p) {
        auto& s = p.getAPVTS();
        auto* um = &p.getUndoManager();

        for (auto* b : { &prev_, &next_, &presetName_, &save_, &undo_, &redo_, &about_ }) addAndMakeVisible(b);
        about_.setTooltip("How Mutadrums works, and credits");
        about_.onClick = [this] { if (onAbout) onAbout(); };
        prev_.setTooltip("Previous preset");
        next_.setTooltip("Next preset");
        presetName_.setTooltip("Browse presets");
        save_.setTooltip("Save the current sound as a user preset");
        undo_.setTooltip("Undo (Ctrl/Cmd+Z)");
        redo_.setTooltip("Redo (Ctrl/Cmd+Shift+Z)");
        presetName_.setColour(juce::TextButton::buttonColourId, Theme::well);
        presetName_.setColour(juce::TextButton::textColourOffId, Theme::text);

        prev_.onClick = [this] { step(-1); };
        next_.onClick = [this] { step(+1); };
        presetName_.onClick = [this] { if (onBrowse) onBrowse(false); };
        save_.onClick = [this] { if (onBrowse) onBrowse(true); };
        undo_.onClick = [this] { proc_.getUndoManager().undo(); };
        redo_.onClick = [this] { proc_.getUndoManager().redo(); };

        run_.attach(s, Params::ID::run, um);
        run_.setTooltip("Start / stop the sequencer. In Host Sync mode it follows your DAW's transport.");
        hostSync_.attach(s, Params::ID::hostSync, um);
        hostSync_.setTooltip("On: follow the DAW tempo and transport. Off: run from the internal BPM.");
        bpm_.attach(s, Params::ID::bpm, um);
        bpm_.setTooltip("Internal tempo - drag up/down or double-click to type");
        master_.attach(s, Params::ID::master, um);
        master_.setTooltipText("Master volume");
        master_.setShowValue(false);
        for (auto* c : std::initializer_list<juce::Component*>{ &run_, &hostSync_, &bpm_, &master_, &meterL_, &meterR_ })
            addAndMakeVisible(c);
    }

    std::function<void(bool focusSave)> onBrowse;
    std::function<void()> onPresetChanged;
    std::function<void()> onAbout;

    void refresh() {
        presetName_.setButtonText(proc_.getCurrentPresetName());
        undo_.setEnabled(proc_.getUndoManager().canUndo());
        redo_.setEnabled(proc_.getUndoManager().canRedo());
        const bool host = proc_.getAPVTS().getRawParameterValue(Params::ID::hostSync)->load() > 0.5f;
        bpm_.setEnabled(!host);
        bpm_.setDisplayOverride(host ? juce::String(proc_.currentBpm.load(), 1) + " BPM" : juce::String());
        const bool running = proc_.clockRunning.load();
        if (running != lastRunning_) { lastRunning_ = running; repaint(); }
        run_.setButtonText(run_.getToggleState() ? (host && !proc_.hostIsPlaying.load() ? "ARMED" : "RUNNING") : "STOPPED");
        meterL_.setLevel(proc_.getMasterPeakL());
        meterR_.setLevel(proc_.getMasterPeakR());
    }

    void paint(juce::Graphics& g) override {
        g.setColour(Theme::panel);
        g.fillRect(getLocalBounds());
        g.setColour(Theme::outline);
        g.drawHorizontalLine(getHeight() - 1, 0.0f, static_cast<float>(getWidth()));

        auto logo = logoArea_;
        const auto logoFont = Theme::font(20.0f, true).withExtraKerningFactor(0.06f);
        g.setFont(logoFont);
        g.setColour(Theme::text);
        const int w1 = logoFont.getStringWidth("MUTA");
        g.drawText("MUTA", logo.removeFromLeft(w1 + 2), juce::Justification::centredLeft);
        g.setColour(Theme::accent);
        const int w2 = logoFont.getStringWidth("DRUMS");
        g.drawText("DRUMS", logo.removeFromLeft(w2 + 8), juce::Justification::centredLeft);
        g.setColour(Theme::textFaint);
        g.setFont(Theme::font(10.5f));
        g.drawText("by mtyas", logo.withTrimmedTop(5), juce::Justification::centredLeft);

        // Running lamp
        auto lamp = lampArea_.toFloat();
        g.setColour(lastRunning_ ? Theme::ok : Theme::textFaint);
        g.fillEllipse(lamp.withSizeKeepingCentre(8.0f, 8.0f));

        g.setColour(Theme::textFaint);
        g.setFont(Theme::font(9.5f, true));
        g.drawText("MASTER", masterLabel_, juce::Justification::centredRight);
    }

    void resized() override {
        auto r = getLocalBounds().reduced(14, 10);
        logoArea_ = r.removeFromLeft(196);

        auto presetBox = r.removeFromLeft(390);
        prev_.setBounds(presetBox.removeFromLeft(32));
        presetBox.removeFromLeft(4);
        save_.setBounds(presetBox.removeFromRight(58));
        presetBox.removeFromRight(4);
        next_.setBounds(presetBox.removeFromRight(32));
        presetBox.removeFromRight(4);
        presetName_.setBounds(presetBox);

        r.removeFromLeft(10);
        undo_.setBounds(r.removeFromLeft(54));
        r.removeFromLeft(4);
        redo_.setBounds(r.removeFromLeft(54));
        r.removeFromLeft(4);
        about_.setBounds(r.removeFromLeft(60));

        // right side: master
        auto meters = r.removeFromRight(10);
        meterR_.setBounds(meters.removeFromRight(4));
        meters.removeFromRight(2);
        meterL_.setBounds(meters.removeFromRight(4));
        r.removeFromRight(4);
        master_.setBounds(r.removeFromRight(40).withTrimmedTop(-6).withTrimmedBottom(-6));
        masterLabel_ = r.removeFromRight(50);
        r.removeFromRight(16);

        bpm_.setBounds(r.removeFromRight(98));
        r.removeFromRight(6);
        hostSync_.setBounds(r.removeFromRight(92));
        r.removeFromRight(6);
        run_.setBounds(r.removeFromRight(92));
        lampArea_ = r.removeFromRight(18);
    }

private:
    void step(int dir) {
        auto& pm = proc_.getPresetManager();
        const int n = static_cast<int>(pm.getAllPresets().size());
        if (n == 0) return;
        const int idx = (pm.getCurrentPresetIndex() + dir + n) % n;
        proc_.loadPreset(idx);
        if (onPresetChanged) onPresetChanged();
    }

    MutadrumsAudioProcessor& proc_;
    juce::Rectangle<int> logoArea_, lampArea_, masterLabel_;
    juce::TextButton prev_{ "<" }, next_{ ">" }, presetName_{ "Init" }, save_{ "Save" }, undo_{ "Undo" }, redo_{ "Redo" }, about_{ "About" };
    ToggleChip run_{ "RUNNING", Theme::ok }, hostSync_{ "HOST SYNC" };
    DragNumber bpm_;
    Knob master_{ {}, Theme::accent };
    Meter meterL_, meterR_;
    bool lastRunning_{ false };
};

} // namespace Mutadrums
