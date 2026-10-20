// Mutadrums - generative drum machine
// Copyright (C) 2026 mtyas
// SPDX-License-Identifier: GPL-3.0-or-later
//
// This program is free software: you can redistribute it and/or modify it under the terms of the
// GNU General Public License as published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version. See the LICENSE file for details.

#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "ui/Theme.h"
#include "ui/Widgets.h"
#include "ui/HeaderBar.h"
#include "ui/PatternPanel.h"
#include "ui/VoicePads.h"
#include "ui/VoicePage.h"
#include "ui/MixerPage.h"
#include "ui/FxPage.h"
#include "ui/PresetBrowser.h"
#include "ui/AboutPage.h"

namespace Mutadrums {

// Everything is laid out at a fixed design size and scaled as a whole when the window resizes.
class MainView : public juce::Component {
public:
    static constexpr int kWidth = 1240;
    static constexpr int kHeight = 800;

    explicit MainView(MutadrumsAudioProcessor& p);

    void paint(juce::Graphics& g) override;
    void resized() override;
    void refresh();
    void selectVoice(int v);
    void setPage(int page);
    bool keyPressed(const juce::KeyPress& key) override;

private:
    MutadrumsAudioProcessor& proc_;
    HeaderBar header_;
    PatternPanel pattern_;
    VoicePads pads_;
    juce::TextButton tabVoice_{ "VOICE" }, tabMixer_{ "MIXER" }, tabFx_{ "EFFECTS" };
    VoicePage voicePage_;
    MixerPage mixerPage_;
    FxPage fxPage_;
    PresetBrowser browser_;
    AboutPage about_;
    int page_{ 0 };
};

class MutadrumsAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit MutadrumsAudioProcessorEditor(MutadrumsAudioProcessor&);
    ~MutadrumsAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    Theme::LookAndFeel lnf_;
    MainView view_;
    juce::TooltipWindow tooltips_{ this, 600 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MutadrumsAudioProcessorEditor)
};

} // namespace Mutadrums
