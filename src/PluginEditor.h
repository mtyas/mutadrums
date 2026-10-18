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

namespace MutableDrums {

// Everything is laid out at a fixed design size and scaled as a whole when the window resizes.
class MainView : public juce::Component {
public:
    static constexpr int kWidth = 1240;
    static constexpr int kHeight = 800;

    explicit MainView(MutableDrumsAudioProcessor& p);

    void paint(juce::Graphics& g) override;
    void resized() override;
    void refresh();
    void selectVoice(int v);
    void setPage(int page);
    bool keyPressed(const juce::KeyPress& key) override;

private:
    MutableDrumsAudioProcessor& proc_;
    HeaderBar header_;
    PatternPanel pattern_;
    VoicePads pads_;
    juce::TextButton tabVoice_{ "VOICE" }, tabMixer_{ "MIXER" }, tabFx_{ "EFFECTS" };
    VoicePage voicePage_;
    MixerPage mixerPage_;
    FxPage fxPage_;
    PresetBrowser browser_;
    int page_{ 0 };
};

class MutableDrumsAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit MutableDrumsAudioProcessorEditor(MutableDrumsAudioProcessor&);
    ~MutableDrumsAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    Theme::LookAndFeel lnf_;
    MainView view_;
    juce::TooltipWindow tooltips_{ this, 600 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MutableDrumsAudioProcessorEditor)
};

} // namespace MutableDrums
