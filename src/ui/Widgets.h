#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Theme.h"

namespace MutableDrums {

using APVTS = juce::AudioProcessorValueTreeState;

// =====================================================================================
//  Knob — label on top, rotary in the middle, live value underneath.
//  Double-click resets to default, drag for coarse, Shift/Ctrl-drag for fine.
// =====================================================================================
class Knob : public juce::Component, private juce::Slider::Listener {
public:
    explicit Knob(const juce::String& labelText = {}, juce::Colour accentColour = Theme::accent, bool bipolar = false)
        : label_(labelText) {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        slider.setRotaryParameters(juce::MathConstants<float>::pi * 1.22f, juce::MathConstants<float>::pi * 2.78f, true);
        slider.setMouseDragSensitivity(220);
        slider.setVelocityBasedMode(false);
        slider.setPopupDisplayEnabled(false, false, nullptr);
        slider.addListener(this);
        setAccent(accentColour);
        setBipolar(bipolar);
        addAndMakeVisible(slider);
    }
    ~Knob() override { slider.removeListener(this); }

    void attach(APVTS& state, const juce::String& paramID, juce::UndoManager* um = nullptr) {
        attachment_.reset();
        if (auto* p = state.getParameter(paramID)) {
            param_ = p;
            attachment_ = std::make_unique<juce::SliderParameterAttachment>(*p, slider, um);
            if (tooltip_.isEmpty()) slider.setTooltip(p->getName(64));
        }
        repaint();
    }

    void setLabel(const juce::String& t) { if (t != label_) { label_ = t; repaint(); } }
    void setTooltipText(const juce::String& t) { tooltip_ = t; slider.setTooltip(t); }
    void setAccent(juce::Colour c) { slider.getProperties().set("accent", static_cast<juce::int64>(c.getARGB())); slider.repaint(); }
    void setBipolar(bool b) { slider.getProperties().set("bipolar", b); }
    void setShowValue(bool b) { showValue_ = b; repaint(); }

    // Normalised (0..1 range units) modulation offset to draw as a teal arc. NaN/0 hides it.
    void setModOffset(float normalised) {
        if (std::abs(normalised - lastMod_) < 0.001f) return;
        lastMod_ = normalised;
        if (std::abs(normalised) < 0.001f) slider.getProperties().remove("modOffset");
        else slider.getProperties().set("modOffset", normalised);
        slider.repaint();
    }

    juce::Slider slider;

    void resized() override {
        auto r = getLocalBounds();
        const int labelH = label_.isNotEmpty() ? 14 : 0;
        r.removeFromTop(labelH);
        if (showValue_) r.removeFromBottom(14);
        slider.setBounds(r);
    }

    void paint(juce::Graphics& g) override {
        auto r = getLocalBounds();
        if (label_.isNotEmpty()) {
            g.setColour(isEnabled() ? Theme::textDim : Theme::textFaint);
            g.setFont(Theme::label());
            g.drawFittedText(label_.toUpperCase(), r.removeFromTop(14), juce::Justification::centred, 1, 0.8f);
        }
        if (showValue_) {
            g.setColour(slider.isMouseOverOrDragging() ? Theme::text : Theme::textDim.brighter(0.2f));
            g.setFont(Theme::value());
            g.drawFittedText(slider.getTextFromValue(slider.getValue()), r.removeFromBottom(14),
                             juce::Justification::centred, 1, 0.8f);
        }
    }

    void enablementChanged() override { slider.setEnabled(isEnabled()); repaint(); }

private:
    void sliderValueChanged(juce::Slider*) override { repaint(); }
    void sliderDragStarted(juce::Slider*) override { repaint(); }
    void sliderDragEnded(juce::Slider*) override { repaint(); }

    juce::String label_, tooltip_;
    bool showValue_{ true };
    float lastMod_{ 0.0f };
    juce::AudioProcessorParameter* param_{ nullptr };
    std::unique_ptr<juce::SliderParameterAttachment> attachment_;
};

// =====================================================================================
//  ChoiceBox — combo bound to an AudioParameterChoice (or Int), optional label above.
// =====================================================================================
class ChoiceBox : public juce::Component {
public:
    explicit ChoiceBox(const juce::String& labelText = {}) : label_(labelText) {
        addAndMakeVisible(combo);
    }

    void attach(APVTS& state, const juce::String& paramID, juce::UndoManager* um = nullptr) {
        attachment_.reset();
        combo.clear(juce::dontSendNotification);
        if (auto* p = state.getParameter(paramID)) {
            if (auto* c = dynamic_cast<juce::AudioParameterChoice*>(p)) {
                combo.addItemList(c->choices, 1);
            } else if (auto* ip = dynamic_cast<juce::AudioParameterInt*>(p)) {
                const auto range = ip->getRange();
                for (int v = range.getStart(); v <= range.getEnd(); ++v)
                    combo.addItem(static_cast<juce::AudioProcessorParameter*>(ip)->getText(ip->convertTo0to1(static_cast<float>(v)), 64), v - range.getStart() + 1);
            }
            attachment_ = std::make_unique<juce::ComboBoxParameterAttachment>(*dynamic_cast<juce::RangedAudioParameter*>(p), combo, um);
            if (combo.getTooltip().isEmpty()) combo.setTooltip(p->getName(64));
        }
    }

    void setLabel(const juce::String& t) { label_ = t; resized(); repaint(); }

    juce::ComboBox combo;

    void resized() override {
        auto r = getLocalBounds();
        if (label_.isNotEmpty()) r.removeFromTop(15);
        combo.setBounds(r);
    }
    void paint(juce::Graphics& g) override {
        if (label_.isEmpty()) return;
        g.setColour(Theme::textDim);
        g.setFont(Theme::label());
        g.drawText(label_.toUpperCase(), getLocalBounds().removeFromTop(14), juce::Justification::centredLeft);
    }

private:
    juce::String label_;
    std::unique_ptr<juce::ComboBoxParameterAttachment> attachment_;
};

// =====================================================================================
//  ToggleChip — small pill toggle bound to a bool parameter.
// =====================================================================================
class ToggleChip : public juce::TextButton {
public:
    explicit ToggleChip(const juce::String& t, juce::Colour onColour = Theme::accent) : juce::TextButton(t) {
        setClickingTogglesState(true);
        setColour(juce::TextButton::buttonOnColourId, onColour);
    }
    void attach(APVTS& state, const juce::String& paramID, juce::UndoManager* um = nullptr) {
        attachment_.reset();
        if (auto* p = state.getParameter(paramID)) {
            attachment_ = std::make_unique<juce::ButtonParameterAttachment>(*p, *this, um);
            if (getTooltip().isEmpty()) setTooltip(p->getName(64));
        }
    }
    void setOnColour(juce::Colour c) { setColour(juce::TextButton::buttonOnColourId, c); }

private:
    std::unique_ptr<juce::ButtonParameterAttachment> attachment_;
};

// =====================================================================================
//  Segmented — a row of radio buttons bound to a choice parameter.
// =====================================================================================
class Segmented : public juce::Component {
public:
    Segmented() = default;

    void attach(APVTS& state, const juce::String& paramID, juce::UndoManager* um = nullptr,
                juce::StringArray labels = {}) {
        attachment_.reset();
        buttons_.clear();
        auto* p = dynamic_cast<juce::RangedAudioParameter*>(state.getParameter(paramID));
        if (p == nullptr) return;
        param_ = p;
        if (labels.isEmpty()) {
            if (auto* c = dynamic_cast<juce::AudioParameterChoice*>(p)) labels = c->choices;
        }
        for (int i = 0; i < labels.size(); ++i) {
            auto* b = buttons_.add(new juce::TextButton(labels[i]));
            b->setClickingTogglesState(false);
            b->setColour(juce::TextButton::buttonOnColourId, onColour_);
            b->onClick = [this, i] {
                if (attachment_) attachment_->setValueAsCompleteGesture(static_cast<float>(i));
            };
            addAndMakeVisible(b);
        }
        attachment_ = std::make_unique<juce::ParameterAttachment>(*p, [this](float v) { setSelected(juce::roundToInt(v)); }, um);
        attachment_->sendInitialUpdate();
        resized();
    }

    void setOnColour(juce::Colour c) {
        onColour_ = c;
        for (auto* b : buttons_) b->setColour(juce::TextButton::buttonOnColourId, c);
    }

    void resized() override {
        if (buttons_.isEmpty()) return;
        auto r = getLocalBounds();
        const int w = r.getWidth() / buttons_.size();
        for (int i = 0; i < buttons_.size(); ++i)
            buttons_[i]->setBounds(i == buttons_.size() - 1 ? r : r.removeFromLeft(w).withTrimmedRight(2));
    }

private:
    void setSelected(int idx) {
        for (int i = 0; i < buttons_.size(); ++i) buttons_[i]->setToggleState(i == idx, juce::dontSendNotification);
    }
    juce::OwnedArray<juce::TextButton> buttons_;
    juce::Colour onColour_{ Theme::accent };
    juce::RangedAudioParameter* param_{ nullptr };
    std::unique_ptr<juce::ParameterAttachment> attachment_;
};

// =====================================================================================
//  Meter — vertical (or horizontal) peak meter with smooth fall and peak hold.
// =====================================================================================
class Meter : public juce::Component {
public:
    explicit Meter(bool vertical = true) : vertical_(vertical) { setInterceptsMouseClicks(false, false); }

    void setLevel(float linear) {
        const float db = linear > 0.00001f ? juce::Decibels::gainToDecibels(linear) : -100.0f;
        const float norm = juce::jlimit(0.0f, 1.0f, (db + 54.0f) / 60.0f); // -54..+6 dB
        level_ = norm > level_ ? norm : level_ * 0.86f + norm * 0.14f;
        if (norm >= hold_) { hold_ = norm; holdFrames_ = 30; }
        else if (--holdFrames_ < 0) hold_ = juce::jmax(0.0f, hold_ - 0.015f);
        clip_ = db > -0.3f ? 45 : juce::jmax(0, clip_ - 1);
        repaint();
    }

    void paint(juce::Graphics& g) override {
        auto r = getLocalBounds().toFloat();
        g.setColour(Theme::well);
        g.fillRoundedRectangle(r, 2.0f);
        auto fillArea = r.reduced(1.0f);
        const float zeroPos = 54.0f / 60.0f;
        juce::ColourGradient grad = vertical_
            ? juce::ColourGradient(Theme::ok, fillArea.getBottomLeft(), Theme::danger, fillArea.getTopLeft(), false)
            : juce::ColourGradient(Theme::ok, fillArea.getTopLeft(), Theme::danger, fillArea.getTopRight(), false);
        grad.addColour(zeroPos * 0.8, Theme::ok);
        grad.addColour(zeroPos, Theme::accent);
        g.setGradientFill(grad);
        if (vertical_) {
            g.fillRect(fillArea.withTop(fillArea.getBottom() - fillArea.getHeight() * level_));
            const float hy = fillArea.getBottom() - fillArea.getHeight() * hold_;
            if (hold_ > 0.01f) g.fillRect(fillArea.getX(), hy, fillArea.getWidth(), 1.5f);
            if (clip_ > 0) { g.setColour(Theme::danger); g.fillRect(fillArea.withHeight(3.0f)); }
        } else {
            g.fillRect(fillArea.withWidth(fillArea.getWidth() * level_));
            const float hx = fillArea.getX() + fillArea.getWidth() * hold_;
            if (hold_ > 0.01f) g.fillRect(hx, fillArea.getY(), 1.5f, fillArea.getHeight());
        }
    }

private:
    bool vertical_;
    float level_{ 0.0f }, hold_{ 0.0f };
    int holdFrames_{ 0 }, clip_{ 0 };
};

// =====================================================================================
//  DragNumber — compact numeric field: drag up/down to change, double-click to type.
// =====================================================================================
class DragNumber : public juce::Slider {
public:
    DragNumber() {
        setSliderStyle(juce::Slider::LinearBarVertical);
        setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        setMouseDragSensitivity(400);
        setVelocityBasedMode(false);
        setSliderSnapsToMousePosition(false);
        editor_.setJustification(juce::Justification::centred);
        editor_.setFont(Theme::font(15.0f, true));
        editor_.onReturnKey = [this] { commit(); };
        editor_.onFocusLost = [this] { commit(); };
        editor_.onEscapeKey = [this] { editor_.setVisible(false); };
        addChildComponent(editor_);
    }
    void attach(APVTS& state, const juce::String& paramID, juce::UndoManager* um = nullptr) {
        attachment_.reset();
        if (auto* p = dynamic_cast<juce::RangedAudioParameter*>(state.getParameter(paramID)))
            attachment_ = std::make_unique<juce::SliderParameterAttachment>(*p, *this, um);
        setDoubleClickReturnValue(false, 0.0);
    }
    void setDisplayOverride(const juce::String& s) { if (s != override_) { override_ = s; repaint(); } }

    void paint(juce::Graphics& g) override {
        auto r = getLocalBounds().toFloat().reduced(0.5f);
        g.setColour(Theme::well);
        g.fillRoundedRectangle(r, 5.0f);
        g.setColour(isMouseOverOrDragging() && isEnabled() ? Theme::outlineHi : Theme::outline);
        g.drawRoundedRectangle(r, 5.0f, 1.0f);
        g.setColour(isEnabled() ? Theme::text : Theme::textDim);
        g.setFont(Theme::font(15.0f, true));
        g.drawText(override_.isNotEmpty() ? override_ : getTextFromValue(getValue()), getLocalBounds(), juce::Justification::centred);
    }
    void resized() override { editor_.setBounds(getLocalBounds().reduced(2)); }
    void mouseDoubleClick(const juce::MouseEvent&) override {
        if (!isEnabled()) return;
        editor_.setText(juce::String(getValue(), 1), false);
        editor_.setVisible(true);
        editor_.grabKeyboardFocus();
        editor_.selectAll();
    }

private:
    void commit() {
        if (!editor_.isVisible()) return;
        const double v = editor_.getText().getDoubleValue();
        editor_.setVisible(false);
        if (v > 0.0) setValue(v, juce::sendNotificationSync);
    }
    juce::TextEditor editor_;
    juce::String override_;
    std::unique_ptr<juce::SliderParameterAttachment> attachment_;
};

} // namespace MutableDrums
