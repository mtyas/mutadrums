// Mutadrums - generative drum machine
// Copyright (C) 2026 mtyas
// SPDX-License-Identifier: GPL-3.0-or-later
//
// This program is free software: you can redistribute it and/or modify it under the terms of the
// GNU General Public License as published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version. See the LICENSE file for details.

#pragma once
#include "Widgets.h"

namespace Mutadrums {

// =====================================================================================
//  Full-window About page: how everything works, credits and licence.
// =====================================================================================
class AboutPage : public juce::Component {
public:
    AboutPage() {
        setWantsKeyboardFocus(true);
        viewport_.setViewedComponent(&content_, false);
        viewport_.setScrollBarsShown(true, false);
        viewport_.setScrollBarThickness(8);
        addAndMakeVisible(viewport_);
        close_.onClick = [this] { if (onClose) onClose(); };
        github_.setURL(juce::URL("https://github.com/mtyas/mutadrums"));
        github_.setFont(Theme::font(12.5f, true), false, juce::Justification::centredRight);
        github_.setColour(juce::HyperlinkButton::textColourId, Theme::accent);
        addAndMakeVisible(close_);
        addAndMakeVisible(github_);
    }

    std::function<void()> onClose;

    void open() {
        viewport_.setViewPosition(0, 0);
        setVisible(true);
        toFront(true);
    }

    void paint(juce::Graphics& g) override {
        g.fillAll(Theme::bg.withAlpha(0.92f));
        Theme::panelBox(g, box_.toFloat());

        auto head = box_.reduced(24, 0).removeFromTop(64);
        const auto logoFont = Theme::font(26.0f, true).withExtraKerningFactor(0.06f);
        g.setFont(logoFont);
        g.setColour(Theme::text);
        const int w1 = logoFont.getStringWidth("MUTA");
        g.drawText("MUTA", head.removeFromLeft(w1 + 2), juce::Justification::centredLeft);
        g.setColour(Theme::accent);
        const int w2 = logoFont.getStringWidth("DRUMS");
        g.drawText("DRUMS", head.removeFromLeft(w2 + 10), juce::Justification::centredLeft);
        g.setColour(Theme::textDim);
        g.setFont(Theme::font(13.0f));
        g.drawText("by mtyas  -  version " + versionString(), head.withTrimmedTop(6), juce::Justification::centredLeft);
    }

    void resized() override {
        box_ = getLocalBounds().reduced(110, 40);
        auto r = box_.reduced(24, 0);
        auto head = r.removeFromTop(64);
        close_.setBounds(head.removeFromRight(80).withSizeKeepingCentre(80, 28));
        head.removeFromRight(12);
        github_.setBounds(head.removeFromRight(260).withSizeKeepingCentre(260, 28));
        r.removeFromBottom(18);
        viewport_.setBounds(r);
        content_.layout(r.getWidth() - 14);
    }

    // Close on Escape
    bool keyPressed(const juce::KeyPress& k) override {
        if (k == juce::KeyPress::escapeKey) { if (onClose) onClose(); return true; }
        return false;
    }

private:
    static juce::String versionString() {
       #ifdef JucePlugin_VersionString
        return JucePlugin_VersionString;
       #else
        return "dev";
       #endif
    }

    // Headings + wrapped paragraphs, measured with TextLayout so the page scrolls to fit.
    class Content : public juce::Component {
    public:
        Content() {
            add("What it is",
                "Mutadrums is an 8-voice generative drum machine. Two pattern generators decide when things play, "
                "twelve drum models decide what they sound like, and a channel strip per voice, a mixer and two "
                "effects shape the result. Every control is a host parameter, so all of it can be automated, and "
                "Undo / Redo covers everything.");
            add("Getting started",
                "Load the Init Kit preset (the first preset, in the Init category): a plain kit with no processing. "
                "Press play in your DAW (with HOST SYNC on) or switch HOST SYNC off and press RUNNING to use the internal "
                "BPM. Kick, snare and closed hat follow Grids straight away. Click any pad to select and hear that voice, "
                "then shape it on the VOICE page.");
            add("Grids - topographic patterns",
                "The XY pad moves you around a map of drum patterns: nearby points sound related, so dragging slowly "
                "morphs one groove into another. The three Density controls set how busy the kick (BD), snare (SD) "
                "and hat (HH) lanes are; Chaos adds random variation; Swing delays every second 16th. The 32-step "
                "preview shows exactly what will play, accents in brighter colours. In Euclidean mode each lane "
                "instead spreads its hits evenly over its own Length.");
            add("Marbles - random gates and voltages",
                "Marbles produces three gate streams (t1-t3) and three random values (X1-X3). Rate runs it at 1/4x to "
                "4x the 16th-note clock, Jitter humanises the timing, Bias leans the coin tosses, and Deja Vu with Loop "
                "Length makes the randomness repeat. Model picks Coin Toss, Polyrhythm or Drums behaviour. The "
                "\"Marbles modulates Grids\" knobs let X1/X2/X3/t1 move the Grids map, chaos and density.");
            add("Voices and engines",
                "Each of the 8 voices runs one of 12 engines: 808 and 909-style kicks and snares, closed and open hats, "
                "a modal resonator (woodblock to tom to bell), a clap, and Braids-style kick, snare, cymbal and bell. "
                "Tune, Decay, Tone and Character shape the sound; Tone and Character are relabelled per engine "
                "(e.g. Snappy, Drive, Inharmonic).\n\n"
                "Trigger chooses what plays the voice: a Grids lane, a Marbles gate, or MIDI / pad only. Voices in the "
                "same Choke group cut each other off, like a closed hat stopping an open hat. Each voice has its own "
                "MIDI note (GM defaults: 36, 38, 42, 46, 39, 45, 48, 49); MIDI notes and pad clicks always play a voice, "
                "sample-accurately, whatever its trigger.");
            add("Modulation",
                "Each voice has three slots: Source -> Destination x Amount. Sources are Marbles X1-X3, Velocity, "
                "Random, the global tempo-synced LFO and a Bar Ramp. Destinations are Tune, Decay, Tone, Character, "
                "Level and Pan. Sources are sampled once per hit, so every hit gets its own variation and nothing "
                "jumps in the middle of a decaying sound. The teal ring on a knob shows what the last hit received.");
            add("Channel strip: EQ, Drive, Compressor",
                "The EQ has a low shelf, a peaking mid and a high shelf. Drag the L, M and H points on the curve to set "
                "frequency and gain together, use the mouse wheel over M to change its Q, and double-click a point to "
                "reset its gain. Changes glide smoothly in the audio, so sweeping a band never zippers.\n\n"
                "Drive saturates in Tape, Tube or Clip flavour, with Tone (dark to bright) and Trim. The compressor "
                "has threshold, ratio, attack, release and makeup, with a gain-reduction meter.");
            add("Mixer and separate outputs",
                "The MIXER page shows all 8 channels with insert switches, effect sends, pan, fader and meter, plus "
                "the Delay and Reverb returns and Master.\n\n"
                "Each voice's Output selector (on the VOICE page and every mixer strip) sends it to Main or to one of "
                "seven extra stereo outputs, Out 2 to Out 8. Enable those outputs in your DAW to give voices their own "
                "mixer channels. Separate outputs carry the channel after its fader and pan, at the same level as on "
                "Main, without the master volume or safety limiter. If an output is not enabled in the host the voice "
                "plays through Main, so nothing ever goes silent. Delay and reverb returns always play on Main.");
            add("Effects",
                "The delay is a stereo / ping-pong tape delay: tempo-synced or free time, feedback, tone, ping-pong "
                "amount and Tape (wow and warmth), with filtering inside the loop so each repeat darkens. The reverb "
                "is an 8-line feedback delay network with decay set as a real RT60 (0.3 to 12 s), brightness, "
                "pre-delay and width. Both show a live picture of what they are doing.");
            add("Presets, undo and shortcuts",
                "Use the arrows to step through presets or click the name to browse by category and search. Save "
                "stores the current sound as a user preset. Undo: Ctrl/Cmd+Z. Redo: Ctrl/Cmd+Shift+Z or Ctrl+Y. "
                "Double-click any knob to reset it and hover "
                "over anything for a tooltip. The window can be resized from the corner.");
            add("Credits - Emilie Gillet",
                "Mutadrums exists because Emilie Gillet designed the Mutable Instruments Eurorack modules and "
                "released their source code for everyone to learn from. Its heart is her work:\n\n"
                "  - Grids (2012): the topographic drum sequencer. The drum maps and Euclidean tables used here are "
                "her data, taken from the Grids firmware.\n"
                "  - Marbles (2018): the random sampler, with its Deja Vu looping, t and X generators.\n"
                "  - Plaits (2016) and Braids (2012): the 808 and 909-style drum models, hi-hats, modal resonator, "
                "clap and the Braids percussion voices are built on her synthesis designs.\n\n"
                "The ideas, the sound and the musical thinking behind these modules are hers; any mistakes in this "
                "adaptation are mine. Thank you, Emilie.\n\n"
                "Mutable Instruments is a registered trademark. Mutadrums is an independent project by mtyas and is "
                "not affiliated with or endorsed by Mutable Instruments or Emilie Gillet. Module names are used only "
                "to credit where the ideas come from.");
            add("Also built with",
                "JUCE (juce.com), the CLAP plugin standard and clap-juce-extensions (free-audio), and the Inter "
                "typeface by Rasmus Andersson (SIL Open Font License).");
            add("Licence",
                "Mutadrums is free software: you can redistribute it and/or modify it under the terms of the GNU "
                "General Public License, version 3 or (at your option) any later version. It is distributed in the "
                "hope that it will be useful, but WITHOUT ANY WARRANTY. Source code, issues and releases: "
                "github.com/mtyas/mutadrums.");
        }

        void layout(int width) {
            width_ = juce::jmax(200, width);
            int y = 4;
            for (auto& s : sections_) {
                s.headY = y;
                y += 26;
                juce::AttributedString as;
                as.setLineSpacing(3.0f);
                as.append(s.body, Theme::font(13.5f), Theme::textDim);
                as.setWordWrap(juce::AttributedString::byWord);
                s.layout.createLayout(as, static_cast<float>(width_ - 8));
                s.bodyY = y;
                y += static_cast<int>(std::ceil(s.layout.getHeight())) + 22;
            }
            setSize(width_, y);
            repaint();
        }

        void paint(juce::Graphics& g) override {
            for (auto& s : sections_) {
                const bool credit = s.heading.startsWith("Credits");
                g.setColour(credit ? Theme::accent : Theme::text);
                g.setFont(Theme::font(15.0f, true));
                g.drawText(s.heading, 0, s.headY, width_, 22, juce::Justification::centredLeft);
                s.layout.draw(g, juce::Rectangle<float>(0.0f, static_cast<float>(s.bodyY),
                                                        static_cast<float>(width_ - 8), s.layout.getHeight()));
            }
        }

    private:
        struct Section { juce::String heading, body; juce::TextLayout layout; int headY{ 0 }, bodyY{ 0 }; };
        void add(const juce::String& h, const juce::String& b) { sections_.push_back({ h, b, {}, 0, 0 }); }
        std::vector<Section> sections_;
        int width_{ 600 };
    };

    juce::Rectangle<int> box_;
    juce::Viewport viewport_;
    Content content_;
    juce::TextButton close_{ "Close" };
    juce::HyperlinkButton github_{ "github.com/mtyas/mutadrums", juce::URL() };
};

} // namespace Mutadrums
