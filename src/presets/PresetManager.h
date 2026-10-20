// Mutadrums - generative drum machine
// Copyright (C) 2026 mtyas
// SPDX-License-Identifier: GPL-3.0-or-later
//
// This program is free software: you can redistribute it and/or modify it under the terms of the
// GNU General Public License as published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version. See the LICENSE file for details.

#pragma once
#include <juce_core/juce_core.h>
#include <vector>
#include <string>
#include <memory>
#include "FactoryPresets.h"

namespace Mutadrums {

class PresetManager {
public:
    PresetManager() {
        reloadAllPresets();
    }

    void reloadAllPresets() {
        presets_ = FactoryPresets::getFactoryPresets();
        for (auto& fp : presets_) fp.isFactory = true;
        loadUserPresetsFromDisk();
        rebuildCategories();
        if (currentPresetIndex_ >= static_cast<int>(presets_.size())) {
            currentPresetIndex_ = 0;
        }
    }

    const std::vector<PresetData>& getAllPresets() const { return presets_; }
    int getCurrentPresetIndex() const { return currentPresetIndex_; }
    
    const PresetData& getCurrentPreset() const {
        if (currentPresetIndex_ >= 0 && currentPresetIndex_ < static_cast<int>(presets_.size())) {
            return presets_[currentPresetIndex_];
        }
        static PresetData empty;
        return empty;
    }

    void selectPreset(int index) {
        if (index >= 0 && index < static_cast<int>(presets_.size())) {
            currentPresetIndex_ = index;
        }
    }

    void selectNextPreset() {
        if (!presets_.empty()) {
            currentPresetIndex_ = (currentPresetIndex_ + 1) % static_cast<int>(presets_.size());
        }
    }

    void selectPreviousPreset() {
        if (!presets_.empty()) {
            currentPresetIndex_ = (currentPresetIndex_ - 1 + static_cast<int>(presets_.size())) % static_cast<int>(presets_.size());
        }
    }

    const std::vector<std::string>& getCategories() const { return categories_; }

    std::vector<int> getIndicesForCategory(const std::string& category, const juce::String& filterText = "") const {
        std::vector<int> indices;
        juce::String filterLower = filterText.toLowerCase().trim();

        for (size_t i = 0; i < presets_.size(); ++i) {
            const auto& p = presets_[i];
            bool categoryMatch = (category == "All" || p.category == category);
            if (!categoryMatch) continue;

            if (filterLower.isEmpty()) {
                indices.push_back(static_cast<int>(i));
            } else {
                juce::String name(p.name);
                juce::String author(p.author);
                juce::String desc(p.description);
                if (name.toLowerCase().contains(filterLower) ||
                    author.toLowerCase().contains(filterLower) ||
                    desc.toLowerCase().contains(filterLower)) {
                    indices.push_back(static_cast<int>(i));
                }
            }
        }
        return indices;
    }

    juce::File getUserPresetsDirectory() const {
        auto dir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                       .getChildFile("Mutadrums")
                       .getChildFile("Presets");
        if (!dir.exists()) {
            // First run after the rename: bring user presets over from the old MutableDrums folder
            auto old = dir.getParentDirectory().getSiblingFile("MutableDrums").getChildFile("Presets");
            if (old.isDirectory()) old.copyDirectoryTo(dir);
            else dir.createDirectory();
        }
        return dir;
    }

    void loadUserPresetsFromDisk() {
        auto dir = getUserPresetsDirectory();
        auto files = dir.findChildFiles(juce::File::findFiles, false, "*.json");
        for (const auto& file : files) {
            juce::String jsonContent = file.loadFileAsString();
            auto parsed = juce::JSON::parse(jsonContent);
            if (parsed.isObject()) {
                auto p = deserializePreset(parsed);
                // Check if already in list
                bool alreadyExists = false;
                for (const auto& existing : presets_) {
                    if (!existing.isFactory && existing.name == p.name && existing.category == p.category) {
                        alreadyExists = true;
                        break;
                    }
                }
                if (!alreadyExists) {
                    presets_.push_back(p);
                }
            }
        }
    }

    bool saveUserPreset(const PresetData& presetIn) {
        PresetData preset = presetIn;
        preset.isFactory = false;
        // Save to disk
        auto dir = getUserPresetsDirectory();
        juce::String safeName = juce::File::createLegalFileName(preset.name);
        if (safeName.isEmpty()) safeName = "UserPreset";
        auto file = dir.getChildFile(safeName + ".json");

        auto varData = serializePreset(preset);
        juce::String jsonStr = juce::JSON::toString(varData, true);
        if (file.replaceWithText(jsonStr)) {
            // Update in-memory presets
            bool replaced = false;
            for (size_t i = 0; i < presets_.size(); ++i) {
                if (!presets_[i].isFactory && presets_[i].name == preset.name && presets_[i].category == preset.category) {
                    presets_[i] = preset;
                    currentPresetIndex_ = static_cast<int>(i);
                    replaced = true;
                    break;
                }
            }
            if (!replaced) {
                presets_.push_back(preset);
                currentPresetIndex_ = static_cast<int>(presets_.size()) - 1;
            }
            rebuildCategories();
            return true;
        }
        return false;
    }

    bool deletePreset(int index) {
        if (index >= 0 && index < static_cast<int>(presets_.size())) {
            // Only allow deleting user presets
            if (!presets_[index].isFactory) {
                auto dir = getUserPresetsDirectory();
                juce::String safeName = juce::File::createLegalFileName(presets_[index].name);
                auto file = dir.getChildFile(safeName + ".json");
                if (file.existsAsFile()) {
                    file.deleteFile();
                }
                presets_.erase(presets_.begin() + index);
                if (currentPresetIndex_ >= static_cast<int>(presets_.size())) {
                    currentPresetIndex_ = std::max(0, static_cast<int>(presets_.size()) - 1);
                }
                rebuildCategories();
                return true;
            }
        }
        return false;
    }

    bool exportPresetToFile(const PresetData& preset, const juce::File& targetFile) {
        auto varData = serializePreset(preset);
        juce::String jsonStr = juce::JSON::toString(varData, true);
        return targetFile.replaceWithText(jsonStr);
    }

    bool importPresetFromFile(const juce::File& file) {
        if (file.existsAsFile()) {
            auto parsed = juce::JSON::parse(file.loadFileAsString());
            if (parsed.isObject()) {
                auto p = deserializePreset(parsed);
                return saveUserPreset(p);
            }
        }
        return false;
    }

    // JSON serialization using juce::var
    juce::var serializePreset(const PresetData& p) {
        auto* obj = new juce::DynamicObject();
        obj->setProperty("name", juce::String(p.name));
        obj->setProperty("category", juce::String(p.category));
        obj->setProperty("author", juce::String(p.author));
        obj->setProperty("description", juce::String(p.description));

        obj->setProperty("formatVersion", 2);
        obj->setProperty("gridsX", p.gridsX);
        obj->setProperty("gridsMode", p.gridsOutputMode);
        obj->setProperty("gridsSwing", p.gridsSwing);
        obj->setProperty("gridsLen0", p.gridsEuclideanLength[0]);
        obj->setProperty("gridsLen1", p.gridsEuclideanLength[1]);
        obj->setProperty("gridsLen2", p.gridsEuclideanLength[2]);
        obj->setProperty("lfoRate", p.lfoRate);
        obj->setProperty("lfoShape", p.lfoShape);
        obj->setProperty("gridsY", p.gridsY);
        obj->setProperty("gridsChaos", p.gridsChaos);
        obj->setProperty("gridsDensity0", p.gridsDensity[0]);
        obj->setProperty("gridsDensity1", p.gridsDensity[1]);
        obj->setProperty("gridsDensity2", p.gridsDensity[2]);

        // Marbles -> Grids Modulation
        obj->setProperty("gridsModMapX", p.gridsModMapX);
        obj->setProperty("gridsModMapY", p.gridsModMapY);
        obj->setProperty("gridsModChaos", p.gridsModChaos);
        obj->setProperty("gridsModDensity", p.gridsModDensity);
        obj->setProperty("gridsModSourceMapX", p.gridsModSourceMapX);
        obj->setProperty("gridsModSourceMapY", p.gridsModSourceMapY);
        obj->setProperty("gridsModSourceChaos", p.gridsModSourceChaos);
        obj->setProperty("gridsModSourceDensity", p.gridsModSourceDensity);

        // Tempo & Host Sync
        obj->setProperty("bpm", p.bpm);
        obj->setProperty("syncToHost", p.syncToHost);

        obj->setProperty("marblesRate", p.marblesRate);
        obj->setProperty("marblesJitter", p.marblesJitter);
        obj->setProperty("marblesBias", p.marblesBias);
        obj->setProperty("marblesDejaVu", p.marblesDejaVu);
        obj->setProperty("marblesLoopLength", p.marblesLoopLength);
        obj->setProperty("marblesModel", p.marblesModel);

        obj->setProperty("delayTimeMs", p.delayTimeMs);
        obj->setProperty("delayFeedback", p.delayFeedback);
        obj->setProperty("delayTone", p.delayTone);
        obj->setProperty("delayPingPong", p.delayPingPong);
        obj->setProperty("delayReturnDB", p.delayReturnDB);
        obj->setProperty("delaySync", p.delaySync);
        obj->setProperty("delayDivision", p.delayDivision);
        obj->setProperty("delayWarmth", p.delayWarmth);

        obj->setProperty("reverbDecay", p.reverbDecay);
        obj->setProperty("reverbDamp", p.reverbDamp);
        obj->setProperty("reverbPreDelayMs", p.reverbPreDelayMs);
        obj->setProperty("reverbWidth", p.reverbWidth);
        obj->setProperty("reverbReturnDB", p.reverbReturnDB);
        obj->setProperty("masterVolumeDB", p.masterVolumeDB);

        juce::Array<juce::var> chArray;
        for (int i = 0; i < 8; ++i) {
            auto* chObj = new juce::DynamicObject();
            const auto& ch = p.channels[i];
            chObj->setProperty("engine", ch.engine);
            chObj->setProperty("tune", ch.tune);
            chObj->setProperty("decay", ch.decay);
            chObj->setProperty("tone", ch.tone);
            chObj->setProperty("timbre", ch.timbre);
            chObj->setProperty("morph", ch.morph);
            chObj->setProperty("triggerSource", ch.triggerSource);
            chObj->setProperty("chokeGroup", ch.chokeGroup);
            chObj->setProperty("modSource1", ch.modSource1);
            chObj->setProperty("modDest1", ch.modDest1);
            chObj->setProperty("modAmount1", ch.modAmount1);
            chObj->setProperty("modSource2", ch.modSource2);
            chObj->setProperty("modDest2", ch.modDest2);
            chObj->setProperty("modAmount2", ch.modAmount2);
            chObj->setProperty("modSource3", ch.modSource3);
            chObj->setProperty("modDest3", ch.modDest3);
            chObj->setProperty("modAmount3", ch.modAmount3);
            chObj->setProperty("midiNote", ch.midiNote);
            chObj->setProperty("volumeDB", ch.volumeDB);
            chObj->setProperty("pan", ch.pan);
            chObj->setProperty("eqLowFreq", ch.eqLowFreq);
            chObj->setProperty("eqLowGain", ch.eqLowGain);
            chObj->setProperty("eqMidFreq", ch.eqMidFreq);
            chObj->setProperty("eqMidQ", ch.eqMidQ);
            chObj->setProperty("eqMidGain", ch.eqMidGain);
            chObj->setProperty("eqHighGain", ch.eqHighGain);
            chObj->setProperty("eqHighFreq", ch.eqHighFreq);
            chObj->setProperty("eqBypass", ch.eqBypass);
            chObj->setProperty("drive", ch.drive);
            chObj->setProperty("driveTone", ch.driveTone);
            chObj->setProperty("driveTrimDB", ch.driveTrimDB);
            chObj->setProperty("driveMode", ch.driveMode);
            chObj->setProperty("driveBypass", ch.driveBypass);
            chObj->setProperty("compThresh", ch.compThresh);
            chObj->setProperty("compRatio", ch.compRatio);
            chObj->setProperty("compAttackMs", ch.compAttackMs);
            chObj->setProperty("compReleaseMs", ch.compReleaseMs);
            chObj->setProperty("compMakeupDB", ch.compMakeupDB);
            chObj->setProperty("compBypass", ch.compBypass);
            chObj->setProperty("aux1SendDB", ch.aux1SendDB);
            chObj->setProperty("aux2SendDB", ch.aux2SendDB);
            chObj->setProperty("outputBus", ch.outputBus);
            chArray.add(juce::var(chObj));
        }
        obj->setProperty("channels", chArray);

        return juce::var(obj);
    }

    PresetData deserializePreset(const juce::var& v) {
        PresetData p;
        if (!v.isObject()) return p;

        p.name = v.getProperty("name", "Custom").toString().toStdString();
        p.category = v.getProperty("category", "User").toString().toStdString();
        p.author = v.getProperty("author", "User").toString().toStdString();
        p.description = v.getProperty("description", "").toString().toStdString();

        const int formatVersion = static_cast<int>(v.getProperty("formatVersion", 1));
        p.formatVersion = formatVersion;
        p.gridsX = static_cast<float>(v.getProperty("gridsX", 0.5f));
        p.gridsOutputMode = static_cast<int>(v.getProperty("gridsMode", 0));
        p.gridsSwing = static_cast<float>(v.getProperty("gridsSwing", 0.0f));
        p.gridsEuclideanLength[0] = static_cast<int>(v.getProperty("gridsLen0", 16));
        p.gridsEuclideanLength[1] = static_cast<int>(v.getProperty("gridsLen1", 16));
        p.gridsEuclideanLength[2] = static_cast<int>(v.getProperty("gridsLen2", 16));
        p.lfoRate = static_cast<int>(v.getProperty("lfoRate", 4));
        p.lfoShape = static_cast<int>(v.getProperty("lfoShape", 0));
        p.gridsY = static_cast<float>(v.getProperty("gridsY", 0.5f));
        p.gridsChaos = static_cast<float>(v.getProperty("gridsChaos", 0.0f));
        p.gridsDensity[0] = static_cast<float>(v.getProperty("gridsDensity0", 0.6f));
        p.gridsDensity[1] = static_cast<float>(v.getProperty("gridsDensity1", 0.5f));
        p.gridsDensity[2] = static_cast<float>(v.getProperty("gridsDensity2", 0.7f));

        p.gridsModMapX = static_cast<float>(v.getProperty("gridsModMapX", 0.0f));
        p.gridsModMapY = static_cast<float>(v.getProperty("gridsModMapY", 0.0f));
        p.gridsModChaos = static_cast<float>(v.getProperty("gridsModChaos", 0.0f));
        p.gridsModDensity = static_cast<float>(v.getProperty("gridsModDensity", 0.0f));
        p.gridsModSourceMapX = static_cast<int>(v.getProperty("gridsModSourceMapX", 1));
        p.gridsModSourceMapY = static_cast<int>(v.getProperty("gridsModSourceMapY", 2));
        p.gridsModSourceChaos = static_cast<int>(v.getProperty("gridsModSourceChaos", 3));
        p.gridsModSourceDensity = static_cast<int>(v.getProperty("gridsModSourceDensity", 4));

        p.bpm = static_cast<float>(v.getProperty("bpm", 120.0f));
        p.syncToHost = static_cast<bool>(v.getProperty("syncToHost", true));

        p.marblesRate = static_cast<float>(v.getProperty("marblesRate", 1.0f));
        p.marblesJitter = static_cast<float>(v.getProperty("marblesJitter", 0.0f));
        p.marblesBias = static_cast<float>(v.getProperty("marblesBias", 0.5f));
        p.marblesDejaVu = static_cast<float>(v.getProperty("marblesDejaVu", 0.0f));
        p.marblesLoopLength = static_cast<int>(v.getProperty("marblesLoopLength", 16));
        p.marblesModel = static_cast<int>(v.getProperty("marblesModel", 0));

        p.delayTimeMs = static_cast<float>(v.getProperty("delayTimeMs", 350.0f));
        p.delayFeedback = static_cast<float>(v.getProperty("delayFeedback", 0.45f));
        p.delayTone = static_cast<float>(v.getProperty("delayTone", 0.5f));
        p.delayPingPong = static_cast<float>(v.getProperty("delayPingPong", 0.7f));
        p.delayReturnDB = static_cast<float>(v.getProperty("delayReturnDB", -6.0f));
        p.delaySync = static_cast<bool>(v.getProperty("delaySync", false));
        p.delayDivision = static_cast<int>(v.getProperty("delayDivision", 5));
        p.delayWarmth = static_cast<float>(v.getProperty("delayWarmth", 0.5f));

        p.reverbDecay = static_cast<float>(v.getProperty("reverbDecay", 0.65f));
        p.reverbDamp = static_cast<float>(v.getProperty("reverbDamp", 0.4f));
        p.reverbPreDelayMs = static_cast<float>(v.getProperty("reverbPreDelayMs", 20.0f));
        p.reverbWidth = static_cast<float>(v.getProperty("reverbWidth", 0.85f));
        p.reverbReturnDB = static_cast<float>(v.getProperty("reverbReturnDB", -8.0f));
        p.masterVolumeDB = static_cast<float>(v.getProperty("masterVolumeDB", 0.0f));

        if (v.hasProperty("channels")) {
            auto* chArr = v.getProperty("channels", juce::var()).getArray();
            if (chArr != nullptr) {
                for (int i = 0; i < std::min(8, chArr->size()); ++i) {
                    const auto& chVar = chArr->getReference(i);
                    auto& ch = p.channels[i];
                    ch.engine = static_cast<int>(chVar.getProperty("engine", ch.engine));
                    ch.tune = static_cast<float>(chVar.getProperty("tune", ch.tune));
                    ch.decay = static_cast<float>(chVar.getProperty("decay", ch.decay));
                    ch.tone = static_cast<float>(chVar.getProperty("tone", ch.tone));
                    ch.timbre = static_cast<float>(chVar.getProperty("timbre", ch.timbre));
                    ch.morph = static_cast<float>(chVar.getProperty("morph", ch.morph));
                    ch.triggerSource = static_cast<int>(chVar.getProperty("triggerSource", ch.triggerSource));
                    ch.chokeGroup = static_cast<int>(chVar.getProperty("chokeGroup", ch.chokeGroup));
                    ch.modSource1 = static_cast<int>(chVar.getProperty("modSource1", ch.modSource1));
                    ch.modDest1 = static_cast<int>(chVar.getProperty("modDest1", ch.modDest1));
                    ch.modAmount1 = static_cast<float>(chVar.getProperty("modAmount1", ch.modAmount1));
                    ch.modSource2 = static_cast<int>(chVar.getProperty("modSource2", ch.modSource2));
                    ch.modDest2 = static_cast<int>(chVar.getProperty("modDest2", ch.modDest2));
                    ch.modAmount2 = static_cast<float>(chVar.getProperty("modAmount2", ch.modAmount2));
                    if (chVar.hasProperty("modXAmount") && !chVar.hasProperty("modAmount1")) {
                        ch.modAmount1 = static_cast<float>(chVar.getProperty("modXAmount", 0.0f));
                    }
                    ch.modSource3 = static_cast<int>(chVar.getProperty("modSource3", ch.modSource3));
                    ch.modDest3 = static_cast<int>(chVar.getProperty("modDest3", ch.modDest3));
                    ch.modAmount3 = static_cast<float>(chVar.getProperty("modAmount3", ch.modAmount3));
                    ch.midiNote = static_cast<int>(chVar.getProperty("midiNote", ch.midiNote));
                    if (formatVersion < 2) {
                        // v1: dest 3 = Timbre, 4 = Morph (both now "Character"); sources 4-6 were
                        // decaying Marbles t-pulses, closest new equivalent is Velocity.
                        auto upgrade = [](int& src, int& dst) {
                            if (dst == 4) dst = 3;
                            if (src >= 4 && src <= 6) src = 4;
                        };
                        upgrade(ch.modSource1, ch.modDest1);
                        upgrade(ch.modSource2, ch.modDest2);
                        if (ch.triggerSource == 7) ch.triggerSource = 0;
                    }
                    ch.volumeDB = static_cast<float>(chVar.getProperty("volumeDB", ch.volumeDB));
                    ch.pan = static_cast<float>(chVar.getProperty("pan", ch.pan));
                    ch.eqLowFreq = static_cast<float>(chVar.getProperty("eqLowFreq", ch.eqLowFreq));
                    ch.eqLowGain = static_cast<float>(chVar.getProperty("eqLowGain", ch.eqLowGain));
                    ch.eqMidFreq = static_cast<float>(chVar.getProperty("eqMidFreq", ch.eqMidFreq));
                    ch.eqMidQ = static_cast<float>(chVar.getProperty("eqMidQ", ch.eqMidQ));
                    ch.eqMidGain = static_cast<float>(chVar.getProperty("eqMidGain", ch.eqMidGain));
                    ch.eqHighGain = static_cast<float>(chVar.getProperty("eqHighGain", ch.eqHighGain));
                    ch.eqHighFreq = static_cast<float>(chVar.getProperty("eqHighFreq", ch.eqHighFreq));
                    ch.eqBypass = static_cast<bool>(chVar.getProperty("eqBypass", ch.eqBypass));
                    ch.drive = static_cast<float>(chVar.getProperty("drive", ch.drive));
                    ch.driveTone = static_cast<float>(chVar.getProperty("driveTone", ch.driveTone));
                    ch.driveTrimDB = static_cast<float>(chVar.getProperty("driveTrimDB", ch.driveTrimDB));
                    ch.driveMode = static_cast<int>(chVar.getProperty("driveMode", ch.driveMode));
                    ch.driveBypass = static_cast<bool>(chVar.getProperty("driveBypass", ch.driveBypass));
                    ch.compThresh = static_cast<float>(chVar.getProperty("compThresh", ch.compThresh));
                    ch.compRatio = static_cast<float>(chVar.getProperty("compRatio", ch.compRatio));
                    ch.compAttackMs = static_cast<float>(chVar.getProperty("compAttackMs", ch.compAttackMs));
                    ch.compReleaseMs = static_cast<float>(chVar.getProperty("compReleaseMs", ch.compReleaseMs));
                    ch.compMakeupDB = static_cast<float>(chVar.getProperty("compMakeupDB", ch.compMakeupDB));
                    ch.compBypass = static_cast<bool>(chVar.getProperty("compBypass", ch.compBypass));
                    ch.aux1SendDB = static_cast<float>(chVar.getProperty("aux1SendDB", ch.aux1SendDB));
                    ch.aux2SendDB = static_cast<float>(chVar.getProperty("aux2SendDB", ch.aux2SendDB));
                    ch.outputBus = static_cast<int>(chVar.getProperty("outputBus", ch.outputBus));
                }
            }
        }
        return p;
    }

private:
    void rebuildCategories() {
        categories_.clear();
        categories_.push_back("All");
        for (const auto& p : presets_) {
            if (std::find(categories_.begin(), categories_.end(), p.category) == categories_.end()) {
                categories_.push_back(p.category);
            }
        }
    }

    std::vector<PresetData> presets_;
    int currentPresetIndex_{ 0 };
    std::vector<std::string> categories_;
};

} // namespace Mutadrums
