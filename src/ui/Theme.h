// Mutadrums - generative drum machine
// Copyright (C) 2026 mtyas
// SPDX-License-Identifier: GPL-3.0-or-later
//
// This program is free software: you can redistribute it and/or modify it under the terms of the
// GNU General Public License as published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version. See the LICENSE file for details.

#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "BinaryData.h"

namespace Mutadrums {
namespace Theme {

// ---- Palette ------------------------------------------------------------------------
inline const juce::Colour bg        { 0xff0e1014 };
inline const juce::Colour panel     { 0xff161920 };
inline const juce::Colour panelHi   { 0xff1c2029 };
inline const juce::Colour well      { 0xff0b0d11 };
inline const juce::Colour outline   { 0xff272c37 };
inline const juce::Colour outlineHi { 0xff3a4150 };
inline const juce::Colour text      { 0xffe7e9ee };
inline const juce::Colour textDim   { 0xff8a92a3 };
inline const juce::Colour textFaint { 0xff5a6170 };
inline const juce::Colour accent    { 0xffffb547 };
inline const juce::Colour mod       { 0xff48d6c3 };
inline const juce::Colour danger    { 0xffff5d5d };
inline const juce::Colour ok        { 0xff6ee787 };

inline juce::Colour voice(int i) {
    static const juce::uint32 c[8] = { 0xffff6b5b, 0xffffa94d, 0xffffd43b, 0xff9be36d,
                                       0xff3fd5b6, 0xff4dabf7, 0xff9d7bff, 0xfff783c6 };
    return juce::Colour(c[(i % 8 + 8) % 8]);
}
inline juce::Colour gridsInst(int i) {
    static const juce::uint32 c[3] = { 0xffff6b5b, 0xffffa94d, 0xffffd43b };
    return juce::Colour(c[juce::jlimit(0, 2, i)]);
}
inline juce::Colour marblesT(int i) {
    static const juce::uint32 c[3] = { 0xff4dabf7, 0xff9d7bff, 0xff3fd5b6 };
    return juce::Colour(c[juce::jlimit(0, 2, i)]);
}

// ---- Fonts --------------------------------------------------------------------------
// Embedded Inter (SIL Open Font License) so the plugin looks the same on every system
inline juce::Typeface::Ptr typeface(bool bold) {
    static juce::Typeface::Ptr regular = juce::Typeface::createSystemTypefaceFor(BinaryData::InterRegular_ttf, BinaryData::InterRegular_ttfSize);
    static juce::Typeface::Ptr semibold = juce::Typeface::createSystemTypefaceFor(BinaryData::InterSemiBold_ttf, BinaryData::InterSemiBold_ttfSize);
    return bold ? semibold : regular;
}
inline juce::Font font(float size, bool bold = false) {
    return juce::Font(typeface(bold)).withHeight(size);
}
inline juce::Font label() { return font(10.5f, true); }
inline juce::Font value() { return font(11.0f); }
inline juce::Font title() { return font(11.5f, true).withExtraKerningFactor(0.08f); }

// ---- Painting helpers ---------------------------------------------------------------
inline void panelBox(juce::Graphics& g, juce::Rectangle<float> r, const juce::String& heading = {},
                     juce::Colour headingColour = textDim) {
    g.setColour(panel);
    g.fillRoundedRectangle(r, 8.0f);
    g.setColour(outline);
    g.drawRoundedRectangle(r.reduced(0.5f), 8.0f, 1.0f);
    if (heading.isNotEmpty()) {
        g.setColour(headingColour);
        g.setFont(title());
        g.drawText(heading.toUpperCase(), r.reduced(12.0f, 0).withHeight(26.0f).translated(0, 2),
                   juce::Justification::centredLeft);
    }
}

inline void wellBox(juce::Graphics& g, juce::Rectangle<float> r, float radius = 6.0f) {
    g.setColour(well);
    g.fillRoundedRectangle(r, radius);
    g.setColour(outline);
    g.drawRoundedRectangle(r.reduced(0.5f), radius, 1.0f);
}

// =====================================================================================
//  LookAndFeel
// =====================================================================================
class LookAndFeel : public juce::LookAndFeel_V4 {
public:
    LookAndFeel() {
        setColour(juce::ResizableWindow::backgroundColourId, bg);
        setColour(juce::ComboBox::backgroundColourId, panelHi);
        setColour(juce::ComboBox::outlineColourId, outline);
        setColour(juce::ComboBox::textColourId, text);
        setColour(juce::ComboBox::arrowColourId, textDim);
        setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff1b1f27));
        setColour(juce::PopupMenu::textColourId, text);
        setColour(juce::PopupMenu::headerTextColourId, accent);
        setColour(juce::PopupMenu::highlightedBackgroundColourId, accent.withAlpha(0.22f));
        setColour(juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
        setColour(juce::TextButton::buttonColourId, panelHi);
        setColour(juce::TextButton::buttonOnColourId, accent);
        setColour(juce::TextButton::textColourOffId, textDim);
        setColour(juce::TextButton::textColourOnId, juce::Colour(0xff111111));
        setColour(juce::Label::textColourId, text);
        setColour(juce::TextEditor::backgroundColourId, well);
        setColour(juce::TextEditor::textColourId, text);
        setColour(juce::TextEditor::outlineColourId, outline);
        setColour(juce::TextEditor::focusedOutlineColourId, accent);
        setColour(juce::TextEditor::highlightColourId, accent.withAlpha(0.35f));
        setColour(juce::CaretComponent::caretColourId, accent);
        setColour(juce::ListBox::backgroundColourId, well);
        setColour(juce::ScrollBar::thumbColourId, outlineHi);
        setColour(juce::TooltipWindow::backgroundColourId, juce::Colour(0xff242936));
        setColour(juce::TooltipWindow::textColourId, text);
        setColour(juce::TooltipWindow::outlineColourId, outlineHi);
        setColour(juce::Slider::textBoxTextColourId, text);
        setColour(juce::Slider::textBoxBackgroundColourId, well);
        setColour(juce::Slider::textBoxOutlineColourId, outline);
        setColour(juce::AlertWindow::backgroundColourId, panel);
        setColour(juce::AlertWindow::textColourId, text);
        setColour(juce::AlertWindow::outlineColourId, outlineHi);
    }

    // Rotary knob: background track, value arc (accent), optional modulation arc (teal).
    // Slider properties used: "accent" (colour as int64), "modOffset" (normalised, may be absent),
    // "bipolar" (bool: arc grows from the centre).
    void drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float pos,
                          float startAngle, float endAngle, juce::Slider& s) override {
        auto bounds = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y),
                                             static_cast<float>(w), static_cast<float>(h)).reduced(2.0f);
        const float size = juce::jmin(bounds.getWidth(), bounds.getHeight());
        auto r = bounds.withSizeKeepingCentre(size, size);
        const float stroke = juce::jmax(2.5f, size * 0.085f);
        const float radius = size * 0.5f - stroke * 0.5f;
        const auto c = r.getCentre();

        juce::Colour acc = accent;
        if (s.getProperties().contains("accent"))
            acc = juce::Colour(static_cast<juce::uint32>(static_cast<juce::int64>(s.getProperties()["accent"])));
        const bool enabled = s.isEnabled();
        if (!enabled) acc = acc.withSaturation(0.0f).withAlpha(0.5f);

        auto arc = [&](float a0, float a1, float rad) {
            juce::Path p;
            p.addCentredArc(c.x, c.y, rad, rad, 0.0f, a0, a1, true);
            return p;
        };

        // Track
        g.setColour(outline);
        g.strokePath(arc(startAngle, endAngle, radius), juce::PathStrokeType(stroke, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Value arc
        const bool bipolar = s.getProperties().getWithDefault("bipolar", false);
        const float angle = startAngle + pos * (endAngle - startAngle);
        const float from = bipolar ? (startAngle + endAngle) * 0.5f : startAngle;
        if (std::abs(angle - from) > 0.001f) {
            g.setColour(acc);
            g.strokePath(arc(juce::jmin(from, angle), juce::jmax(from, angle), radius),
                         juce::PathStrokeType(stroke, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // Modulation arc (thin, inside the value arc)
        if (s.getProperties().contains("modOffset")) {
            const float off = static_cast<float>(static_cast<double>(s.getProperties()["modOffset"]));
            if (std::abs(off) > 0.002f) {
                const float p2 = juce::jlimit(0.0f, 1.0f, pos + off);
                const float a2 = startAngle + p2 * (endAngle - startAngle);
                g.setColour(mod);
                g.strokePath(arc(juce::jmin(angle, a2), juce::jmax(angle, a2), radius - stroke * 1.25f),
                             juce::PathStrokeType(stroke * 0.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            }
        }

        // Cap
        const float capR = radius - stroke * 1.9f;
        if (capR > 3.0f) {
            g.setColour(s.isMouseOverOrDragging() && enabled ? panelHi.brighter(0.15f) : panelHi);
            g.fillEllipse(c.x - capR, c.y - capR, capR * 2.0f, capR * 2.0f);
            g.setColour(outlineHi);
            g.drawEllipse(c.x - capR, c.y - capR, capR * 2.0f, capR * 2.0f, 1.0f);
            // Pointer
            juce::Path ptr;
            const float pl = capR * 0.62f;
            ptr.addRoundedRectangle(-1.25f, -capR + 2.5f, 2.5f, pl, 1.2f);
            g.setColour(enabled ? text : textFaint);
            g.fillPath(ptr, juce::AffineTransform::rotation(angle).translated(c.x, c.y));
        }
    }

    void drawLinearSlider(juce::Graphics& g, int x, int y, int w, int h, float sliderPos, float, float,
                          juce::Slider::SliderStyle style, juce::Slider& s) override {
        juce::Colour acc = accent;
        if (s.getProperties().contains("accent"))
            acc = juce::Colour(static_cast<juce::uint32>(static_cast<juce::int64>(s.getProperties()["accent"])));
        const bool bipolar = s.getProperties().getWithDefault("bipolar", false);
        auto r = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h));

        if (style == juce::Slider::LinearVertical) {
            auto track = r.withSizeKeepingCentre(6.0f, r.getHeight());
            g.setColour(well);
            g.fillRoundedRectangle(track, 3.0f);
            g.setColour(outline);
            g.drawRoundedRectangle(track, 3.0f, 1.0f);
            auto fill = track.withTop(sliderPos);
            g.setColour(acc.withAlpha(0.85f));
            g.fillRoundedRectangle(fill, 3.0f);
            // Fader cap
            auto cap = juce::Rectangle<float>(r.getWidth() * 0.8f, 12.0f).withCentre({ r.getCentreX(), sliderPos });
            g.setColour(juce::Colour(0xffd9dce3));
            g.fillRoundedRectangle(cap, 3.0f);
            g.setColour(juce::Colour(0xff222630));
            g.drawLine(cap.getX() + 3.0f, cap.getCentreY(), cap.getRight() - 3.0f, cap.getCentreY(), 1.5f);
        } else {
            auto track = r.withSizeKeepingCentre(r.getWidth(), 6.0f);
            g.setColour(well);
            g.fillRoundedRectangle(track, 3.0f);
            g.setColour(outline);
            g.drawRoundedRectangle(track, 3.0f, 1.0f);
            const float from = bipolar ? track.getCentreX() : track.getX();
            auto fill = juce::Rectangle<float>(juce::jmin(from, sliderPos), track.getY(), std::abs(sliderPos - from), track.getHeight());
            g.setColour(acc);
            g.fillRoundedRectangle(fill, 3.0f);
            if (bipolar) {
                g.setColour(textFaint);
                g.fillRect(track.getCentreX() - 0.5f, track.getY() - 3.0f, 1.0f, track.getHeight() + 6.0f);
            }
            g.setColour(juce::Colour(0xffd9dce3));
            g.fillEllipse(juce::Rectangle<float>(12.0f, 12.0f).withCentre({ sliderPos, track.getCentreY() }));
        }
    }

    void drawComboBox(juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box) override {
        auto r = juce::Rectangle<float>(0, 0, static_cast<float>(w), static_cast<float>(h)).reduced(0.5f);
        g.setColour(box.isMouseOver(true) ? panelHi.brighter(0.08f) : panelHi);
        g.fillRoundedRectangle(r, 5.0f);
        g.setColour(box.hasKeyboardFocus(true) ? accent.withAlpha(0.6f) : outline);
        g.drawRoundedRectangle(r, 5.0f, 1.0f);
        juce::Path arrow;
        const float ax = static_cast<float>(w) - 13.0f, ay = static_cast<float>(h) * 0.5f;
        arrow.addTriangle(ax - 4.0f, ay - 2.0f, ax + 4.0f, ay - 2.0f, ax, ay + 3.0f);
        g.setColour(textDim);
        g.fillPath(arrow);
    }

    juce::Typeface::Ptr getTypefaceForFont(const juce::Font& f) override {
        if (f.getTypefaceName() == juce::Font::getDefaultSansSerifFontName())
            return typeface(f.isBold());
        return juce::LookAndFeel_V4::getTypefaceForFont(f);
    }

    juce::Font getComboBoxFont(juce::ComboBox& b) override { return font(juce::jmin(13.0f, b.getHeight() * 0.5f)); }

    void positionComboBoxText(juce::ComboBox& box, juce::Label& label) override {
        label.setBounds(6, 1, box.getWidth() - 24, box.getHeight() - 2);
        label.setFont(getComboBoxFont(box));
    }

    juce::Font getPopupMenuFont() override { return font(13.0f); }

    void drawButtonBackground(juce::Graphics& g, juce::Button& b, const juce::Colour& base, bool over, bool down) override {
        auto r = b.getLocalBounds().toFloat().reduced(0.5f);
        const bool on = b.getToggleState();
        juce::Colour onCol = b.findColour(juce::TextButton::buttonOnColourId);
        juce::Colour c = on ? onCol : base;
        if (over && !on) c = c.brighter(0.08f);
        if (down) c = c.darker(0.1f);
        g.setColour(c);
        g.fillRoundedRectangle(r, juce::jmin(6.0f, r.getHeight() * 0.5f));
        g.setColour(on ? onCol.brighter(0.2f) : outline);
        g.drawRoundedRectangle(r, juce::jmin(6.0f, r.getHeight() * 0.5f), 1.0f);
    }

    void drawButtonText(juce::Graphics& g, juce::TextButton& b, bool, bool) override {
        const auto f = getTextButtonFont(b, b.getHeight());
        g.setFont(f);
        g.setColour(b.findColour(b.getToggleState() ? juce::TextButton::textColourOnId : juce::TextButton::textColourOffId)
                        .withMultipliedAlpha(b.isEnabled() ? 1.0f : 0.45f));
        g.drawFittedText(b.getButtonText(), b.getLocalBounds().reduced(3, 1), juce::Justification::centred, 1, 0.6f);
    }

    juce::Font getTextButtonFont(juce::TextButton&, int h) override { return font(juce::jmin(12.5f, h * 0.55f), true); }

    void drawTooltip(juce::Graphics& g, const juce::String& t, int w, int h) override {
        auto r = juce::Rectangle<float>(0, 0, static_cast<float>(w), static_cast<float>(h));
        g.setColour(findColour(juce::TooltipWindow::backgroundColourId));
        g.fillRoundedRectangle(r, 5.0f);
        g.setColour(findColour(juce::TooltipWindow::outlineColourId));
        g.drawRoundedRectangle(r.reduced(0.5f), 5.0f, 1.0f);
        juce::AttributedString s;
        s.setJustification(juce::Justification::centredLeft);
        s.append(t, font(12.5f), text);
        juce::TextLayout tl;
        tl.createLayout(s, static_cast<float>(w) - 16.0f);
        tl.draw(g, r.reduced(8.0f, 6.0f));
    }

    juce::Rectangle<int> getTooltipBounds(const juce::String& t, juce::Point<int> pos, juce::Rectangle<int> parent) override {
        juce::AttributedString s;
        s.append(t, font(12.5f));
        juce::TextLayout tl;
        tl.createLayout(s, 280.0f);
        const int w = static_cast<int>(tl.getWidth() + 18.0f), h = static_cast<int>(tl.getHeight() + 14.0f);
        return juce::Rectangle<int>(pos.x > parent.getCentreX() ? pos.x - (w + 12) : pos.x + 14,
                                    pos.y > parent.getCentreY() ? pos.y - (h + 8) : pos.y + 18, w, h)
            .constrainedWithin(parent);
    }

    void drawPopupMenuBackground(juce::Graphics& g, int w, int h) override {
        g.fillAll(findColour(juce::PopupMenu::backgroundColourId));
        g.setColour(outlineHi);
        g.drawRect(0, 0, w, h);
    }
};

} // namespace Theme
} // namespace Mutadrums
