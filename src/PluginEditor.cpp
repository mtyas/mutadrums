// Mutadrums - generative drum machine
// Copyright (C) 2026 mtyas
// SPDX-License-Identifier: GPL-3.0-or-later
//
// This program is free software: you can redistribute it and/or modify it under the terms of the
// GNU General Public License as published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version. See the LICENSE file for details.

#include "PluginEditor.h"

namespace Mutadrums {

// =====================================================================================
//  MainView
// =====================================================================================
MainView::MainView(MutadrumsAudioProcessor& p)
    : proc_(p), header_(p), pattern_(p), pads_(p), voicePage_(p), mixerPage_(p), fxPage_(p), browser_(p) {
    setWantsKeyboardFocus(true);

    for (auto* c : std::initializer_list<juce::Component*>{ &header_, &pattern_, &pads_, &voicePage_, &mixerPage_, &fxPage_ })
        addAndMakeVisible(c);

    int i = 0;
    for (auto* t : { &tabVoice_, &tabMixer_, &tabFx_ }) {
        addAndMakeVisible(t);
        t->setRadioGroupId(4711);
        t->setClickingTogglesState(false);
        t->onClick = [this, i] { setPage(i); };
        ++i;
    }
    tabVoice_.setTooltip("Sound, modulation and channel processing of the selected voice");
    tabMixer_.setTooltip("All 8 channels, effect returns and master");
    tabFx_.setTooltip("Delay and reverb");

    pads_.onSelect = [this](int v) { selectVoice(v); };
    mixerPage_.onSelect = [this](int v) { selectVoice(v); };
    header_.onBrowse = [this](bool save) { browser_.open(save); };
    header_.onPresetChanged = [this] { header_.refresh(); };

    addChildComponent(browser_);
    browser_.onClose = [this] { browser_.setVisible(false); grabKeyboardFocus(); };
    browser_.onLoaded = [this] { header_.refresh(); };

    addChildComponent(about_);
    header_.onAbout = [this] { about_.open(); about_.grabKeyboardFocus(); };
    about_.onClose = [this] { about_.setVisible(false); grabKeyboardFocus(); };

    selectVoice(p.selectedVoice.load());
    setPage(0);
}

void MainView::selectVoice(int v) {
    v = juce::jlimit(0, Params::kNumVoices - 1, v);
    proc_.selectedVoice.store(v);
    pads_.setSelected(v);
    voicePage_.setVoice(v);
}

void MainView::setPage(int page) {
    page_ = page;
    tabVoice_.setToggleState(page == 0, juce::dontSendNotification);
    tabMixer_.setToggleState(page == 1, juce::dontSendNotification);
    tabFx_.setToggleState(page == 2, juce::dontSendNotification);
    voicePage_.setVisible(page == 0);
    mixerPage_.setVisible(page == 1);
    fxPage_.setVisible(page == 2);
}

void MainView::refresh() {
    header_.refresh();
    pattern_.refresh();
    pads_.refresh();
    if (voicePage_.isVisible()) voicePage_.refresh();
    if (mixerPage_.isVisible()) mixerPage_.refresh();
    if (fxPage_.isVisible()) fxPage_.refresh();
}

bool MainView::keyPressed(const juce::KeyPress& key) {
    const auto mods = key.getModifiers();
    if (mods.isCommandDown() && key.getKeyCode() == 'Z') {
        if (mods.isShiftDown()) proc_.getUndoManager().redo();
        else proc_.getUndoManager().undo();
        return true;
    }
    if (mods.isCommandDown() && key.getKeyCode() == 'Y') {
        proc_.getUndoManager().redo();
        return true;
    }
    return false;
}

void MainView::paint(juce::Graphics& g) {
    g.fillAll(Theme::bg);
}

void MainView::resized() {
    auto r = getLocalBounds();
    header_.setBounds(r.removeFromTop(56));
    browser_.setBounds(getLocalBounds());
    about_.setBounds(getLocalBounds());
    r.reduce(10, 8);

    pattern_.setBounds(r.removeFromLeft(400));
    r.removeFromLeft(10);

    pads_.setBounds(r.removeFromTop(104));
    r.removeFromTop(8);
    auto tabs = r.removeFromTop(28);
    const int tw = 120;
    tabVoice_.setBounds(tabs.removeFromLeft(tw));
    tabs.removeFromLeft(6);
    tabMixer_.setBounds(tabs.removeFromLeft(tw));
    tabs.removeFromLeft(6);
    tabFx_.setBounds(tabs.removeFromLeft(tw));
    r.removeFromTop(8);
    voicePage_.setBounds(r);
    mixerPage_.setBounds(r);
    fxPage_.setBounds(r);
}

// =====================================================================================
//  Editor
// =====================================================================================
MutadrumsAudioProcessorEditor::MutadrumsAudioProcessorEditor(MutadrumsAudioProcessor& p)
    : AudioProcessorEditor(&p), view_(p) {
    setLookAndFeel(&lnf_);
    addAndMakeVisible(view_);
    tooltips_.setLookAndFeel(&lnf_);

    setResizable(true, true);
    const double ratio = static_cast<double>(MainView::kWidth) / MainView::kHeight;
    setResizeLimits(930, 600, 2480, 1600);
    if (auto* c = getConstrainer()) c->setFixedAspectRatio(ratio);
    setSize(MainView::kWidth, MainView::kHeight);
    startTimerHz(30);
}

MutadrumsAudioProcessorEditor::~MutadrumsAudioProcessorEditor() {
    stopTimer();
    tooltips_.setLookAndFeel(nullptr);
    setLookAndFeel(nullptr);
}

void MutadrumsAudioProcessorEditor::paint(juce::Graphics& g) { g.fillAll(Theme::bg); }

void MutadrumsAudioProcessorEditor::resized() {
    const float scale = static_cast<float>(getWidth()) / static_cast<float>(MainView::kWidth);
    view_.setBounds(0, 0, MainView::kWidth, MainView::kHeight);
    view_.setTransform(juce::AffineTransform::scale(scale));
}

void MutadrumsAudioProcessorEditor::timerCallback() { view_.refresh(); }

} // namespace Mutadrums
