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
//  Full-window preset browser: categories | presets, search, load / save / delete
// =====================================================================================
class PresetBrowser : public juce::Component {
public:
    explicit PresetBrowser(MutadrumsAudioProcessor& p) : proc_(p), catModel_(*this), presetModel_(*this) {
        addAndMakeVisible(categories_);
        addAndMakeVisible(presets_);
        categories_.setModel(&catModel_);
        presets_.setModel(&presetModel_);
        categories_.setRowHeight(30);
        presets_.setRowHeight(40);
        for (auto* lb : { &categories_, &presets_ }) {
            lb->setColour(juce::ListBox::backgroundColourId, Theme::well);
            lb->setColour(juce::ListBox::outlineColourId, Theme::outline);
            lb->setOutlineThickness(1);
        }

        search_.setTextToShowWhenEmpty("Search presets...", Theme::textFaint);
        search_.onTextChange = [this] { refreshPresets(); };
        search_.setFont(Theme::font(14.0f));
        addAndMakeVisible(search_);

        for (auto* b : { &load_, &close_, &del_, &saveBtn_ }) addAndMakeVisible(b);
        load_.setColour(juce::TextButton::buttonColourId, Theme::accent.withAlpha(0.9f));
        load_.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff111111));
        load_.onClick = [this] { loadSelected(); };
        close_.onClick = [this] { if (onClose) onClose(); };
        del_.onClick = [this] { deleteSelected(); };
        del_.setTooltip("Delete the selected user preset (factory presets can't be deleted)");

        saveName_.setTextToShowWhenEmpty("Preset name", Theme::textFaint);
        saveCat_.setTextToShowWhenEmpty("Category", Theme::textFaint);
        for (auto* t : { &saveName_, &saveCat_ }) { t->setFont(Theme::font(14.0f)); addAndMakeVisible(t); }
        saveBtn_.setColour(juce::TextButton::buttonColourId, Theme::ok.withAlpha(0.85f));
        saveBtn_.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff111111));
        saveBtn_.onClick = [this] { saveCurrent(); };
        saveName_.onReturnKey = [this] { saveCurrent(); };
    }

    std::function<void()> onClose;
    std::function<void()> onLoaded;

    void open(bool focusSave = false) {
        proc_.getPresetManager().reloadAllPresets();
        refreshCategories();
        refreshPresets();
        saveName_.setText(proc_.getCurrentPresetName(), false);
        if (saveCat_.isEmpty()) saveCat_.setText("User", false);
        setVisible(true);
        toFront(true);
        if (focusSave) { saveName_.grabKeyboardFocus(); saveName_.selectAll(); }
        else search_.grabKeyboardFocus();
    }

    void paint(juce::Graphics& g) override {
        g.fillAll(Theme::bg.withAlpha(0.88f));
        Theme::panelBox(g, box_.toFloat(), "Presets", Theme::accent);
        g.setColour(Theme::textDim);
        g.setFont(Theme::label());
        g.drawText("CATEGORIES", catLabel_, juce::Justification::centredLeft);
        g.drawText("SAVE CURRENT SOUND AS", saveLabel_, juce::Justification::centredLeft);
        // Description of the selected preset
        const int sel = selectedPresetIndex();
        if (sel >= 0) {
            const auto& pd = proc_.getPresetManager().getAllPresets()[static_cast<size_t>(sel)];
            g.setColour(Theme::text);
            g.setFont(Theme::font(14.0f, true));
            g.drawText(pd.name, descArea_.withHeight(18), juce::Justification::centredLeft);
            g.setColour(Theme::textDim);
            g.setFont(Theme::font(12.5f));
            g.drawFittedText(pd.description.empty() ? juce::String("No description") : juce::String(pd.description),
                             descArea_.withTrimmedTop(20), juce::Justification::topLeft, 3);
        }
    }

    void resized() override {
        box_ = getLocalBounds().reduced(60, 40);
        auto r = box_.reduced(14, 0);
        auto head = r.removeFromTop(38);
        close_.setBounds(head.removeFromRight(80).reduced(0, 7));
        r.removeFromBottom(12);

        auto bottom = r.removeFromBottom(112);
        auto saveCol = bottom.removeFromRight(330);
        descArea_ = bottom.reduced(0, 8);
        saveLabel_ = saveCol.removeFromTop(22);
        saveName_.setBounds(saveCol.removeFromTop(30));
        saveCol.removeFromTop(6);
        auto sr = saveCol.removeFromTop(30);
        saveBtn_.setBounds(sr.removeFromRight(90));
        sr.removeFromRight(6);
        saveCat_.setBounds(sr);

        r.removeFromBottom(8);
        auto left = r.removeFromLeft(200);
        catLabel_ = left.removeFromTop(22);
        categories_.setBounds(left);
        r.removeFromLeft(12);
        auto top = r.removeFromTop(30);
        load_.setBounds(top.removeFromRight(90));
        top.removeFromRight(6);
        del_.setBounds(top.removeFromRight(80));
        top.removeFromRight(10);
        search_.setBounds(top);
        r.removeFromTop(8);
        presets_.setBounds(r);
    }

    bool keyPressed(const juce::KeyPress& k) override {
        if (k == juce::KeyPress::escapeKey) { if (onClose) onClose(); return true; }
        if (k == juce::KeyPress::returnKey && !saveName_.hasKeyboardFocus(false)) { loadSelected(); return true; }
        return false;
    }

private:
    // ---- list models
    struct CatModel : public juce::ListBoxModel {
        explicit CatModel(PresetBrowser& o) : owner(o) {}
        int getNumRows() override { return static_cast<int>(owner.proc_.getPresetManager().getCategories().size()); }
        void paintListBoxItem(int row, juce::Graphics& g, int w, int h, bool selected) override {
            const auto& cats = owner.proc_.getPresetManager().getCategories();
            if (row < 0 || row >= static_cast<int>(cats.size())) return;
            if (selected) { g.setColour(Theme::accent.withAlpha(0.18f)); g.fillRect(0, 0, w, h); }
            g.setColour(selected ? Theme::accent : Theme::text);
            g.setFont(Theme::font(13.5f, selected));
            g.drawText(cats[static_cast<size_t>(row)], 12, 0, w - 16, h, juce::Justification::centredLeft);
        }
        void selectedRowsChanged(int) override { owner.refreshPresets(); }
        PresetBrowser& owner;
    };

    struct PresetModel : public juce::ListBoxModel {
        explicit PresetModel(PresetBrowser& o) : owner(o) {}
        int getNumRows() override { return static_cast<int>(owner.filtered_.size()); }
        void paintListBoxItem(int row, juce::Graphics& g, int w, int h, bool selected) override {
            if (row < 0 || row >= static_cast<int>(owner.filtered_.size())) return;
            const int idx = owner.filtered_[static_cast<size_t>(row)];
            const auto& pd = owner.proc_.getPresetManager().getAllPresets()[static_cast<size_t>(idx)];
            const bool current = idx == owner.proc_.getPresetManager().getCurrentPresetIndex();
            if (selected) { g.setColour(Theme::accent.withAlpha(0.16f)); g.fillRect(0, 0, w, h); }
            g.setColour(Theme::outline.withAlpha(0.6f));
            g.drawHorizontalLine(h - 1, 0.0f, static_cast<float>(w));
            if (current) { g.setColour(Theme::accent); g.fillEllipse(8.0f, h * 0.5f - 3.5f, 7.0f, 7.0f); }
            g.setColour(Theme::text);
            g.setFont(Theme::font(14.0f, true));
            g.drawText(pd.name, 24, 3, w - 140, h / 2, juce::Justification::bottomLeft);
            g.setColour(Theme::textDim);
            g.setFont(Theme::font(11.5f));
            g.drawText(pd.description, 24, h / 2 + 1, w - 140, h / 2 - 4, juce::Justification::topLeft);
            g.setColour(pd.isFactory ? Theme::textFaint : Theme::ok);
            g.setFont(Theme::font(11.0f, true));
            g.drawText(juce::String(pd.category).toUpperCase() + (pd.isFactory ? "" : "  -  USER"), w - 140, 0, 128, h,
                       juce::Justification::centredRight);
        }
        void selectedRowsChanged(int) override { owner.repaint(); }
        void listBoxItemDoubleClicked(int, const juce::MouseEvent&) override { owner.loadSelected(); }
        void returnKeyPressed(int) override { owner.loadSelected(); }
        PresetBrowser& owner;
    };

    void refreshCategories() {
        categories_.updateContent();
        if (categories_.getSelectedRow() < 0) categories_.selectRow(0);
        categories_.repaint();
    }

    void refreshPresets() {
        auto& pm = proc_.getPresetManager();
        const auto& cats = pm.getCategories();
        const int c = categories_.getSelectedRow();
        const std::string cat = (c >= 0 && c < static_cast<int>(cats.size())) ? cats[static_cast<size_t>(c)] : "All";
        filtered_ = pm.getIndicesForCategory(cat, search_.getText());
        presets_.updateContent();
        // select the current preset if visible
        const int cur = pm.getCurrentPresetIndex();
        for (size_t i = 0; i < filtered_.size(); ++i)
            if (filtered_[i] == cur) { presets_.selectRow(static_cast<int>(i)); break; }
        presets_.repaint();
        repaint();
    }

    int selectedPresetIndex() const {
        const int row = presets_.getSelectedRow();
        return (row >= 0 && row < static_cast<int>(filtered_.size())) ? filtered_[static_cast<size_t>(row)] : -1;
    }

    void loadSelected() {
        const int idx = selectedPresetIndex();
        if (idx < 0) return;
        proc_.loadPreset(idx);
        if (onLoaded) onLoaded();
        presets_.repaint();
    }

    void deleteSelected() {
        const int idx = selectedPresetIndex();
        if (idx < 0) return;
        auto& pm = proc_.getPresetManager();
        if (pm.getAllPresets()[static_cast<size_t>(idx)].isFactory) return;
        pm.deletePreset(idx);
        refreshCategories();
        refreshPresets();
    }

    void saveCurrent() {
        auto name = saveName_.getText().trim();
        if (name.isEmpty()) { saveName_.grabKeyboardFocus(); return; }
        auto cat = saveCat_.getText().trim();
        if (cat.isEmpty()) cat = "User";
        auto preset = proc_.captureState();
        preset.name = name.toStdString();
        preset.category = cat.toStdString();
        preset.author = "User";
        if (proc_.getPresetManager().saveUserPreset(preset)) {
            proc_.setCurrentPresetName(name);
            refreshCategories();
            refreshPresets();
            if (onLoaded) onLoaded();
        }
    }

    MutadrumsAudioProcessor& proc_;
    CatModel catModel_;
    PresetModel presetModel_;
    juce::ListBox categories_, presets_;
    juce::TextEditor search_, saveName_, saveCat_;
    juce::TextButton load_{ "Load" }, close_{ "Close" }, del_{ "Delete" }, saveBtn_{ "Save" };
    std::vector<int> filtered_;
    juce::Rectangle<int> box_, catLabel_, saveLabel_, descArea_;
};

} // namespace Mutadrums
