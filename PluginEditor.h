#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

// ==============================================================================
// // DELUXE HARDWARE SKIN (3D BRUSHED METAL, LED TICKS & CHAMPAGNE GOLD)
// ==============================================================================
class LuxuryLookAndFeel : public juce::LookAndFeel_V4
{
public:
    LuxuryLookAndFeel()
    {
        setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff10131a));
        setColour(juce::ComboBox::outlineColourId, juce::Colour(0xffd4af37).withAlpha(0.55f));
        setColour(juce::ComboBox::textColourId, juce::Colour(0xfff5e6c8));
        setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff0d0f14));
        setColour(juce::PopupMenu::textColourId, juce::Colour(0xfff5e6c8));
        setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xffd4af37).withAlpha(0.35f));

        setColour(juce::TextButton::buttonColourId, juce::Colour(0xff161a24));
        setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffd4af37));
        setColour(juce::TextButton::textColourOffId, juce::Colour(0xfff5e6c8));
        setColour(juce::TextButton::textColourOnId, juce::Colour(0xff0b0c10));
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, const float rotaryStartAngle,
                          const float rotaryEndAngle, juce::Slider&) override
    {
        auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(10.0f);
        auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
        auto centreX = bounds.getCentreX();
        auto centreY = bounds.getCentreY();
        auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        // 1. Anillo de ticks LED perimetrales estilo hardware de mastering
        const int numTicks = 25;
        for (int i = 0; i < numTicks; ++i)
        {
            float prop = static_cast<float>(i) / static_cast<float>(numTicks - 1);
            float angle = rotaryStartAngle + prop * (rotaryEndAngle - rotaryStartAngle) - juce::MathConstants<float>::halfPi;
            bool active = (prop <= sliderPos && sliderPos > 0.005f);

            float rIn  = radius - 2.0f;
            float rOut = radius + 2.5f;
            juce::Point<float> p1(centreX + rIn * std::cos(angle), centreY + rIn * std::sin(angle));
            juce::Point<float> p2(centreX + rOut * std::cos(angle), centreY + rOut * std::sin(angle));

            g.setColour(active ? juce::Colour(0xffffdf73) : juce::Colour(0xff252936));
            g.drawLine(juce::Line<float>(p1, p2), active ? 2.0f : 1.2f);
        }

        // 2. Arco continuo luminoso
        float arcRadius = radius - 6.5f;
        juce::Path bgArc;
        bgArc.addCentredArc(centreX, centreY, arcRadius, arcRadius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour(juce::Colour(0xff141720));
        g.strokePath(bgArc, juce::PathStrokeType(3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        if (sliderPos > 0.001f)
        {
            juce::Path valArc;
            valArc.addCentredArc(centreX, centreY, arcRadius, arcRadius, 0.0f, rotaryStartAngle, toAngle, true);
            g.setColour(juce::Colour(0xffd4af37));
            g.strokePath(valArc, juce::PathStrokeType(3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // 3. Cuerpo metálico 3D de la perilla con sombra exterior y bisel
        float knobRadius = radius - 12.0f;
        juce::Rectangle<float> knobRect(centreX - knobRadius, centreY - knobRadius, knobRadius * 2.0f, knobRadius * 2.0f);

        g.setColour(juce::Colours::black.withAlpha(0.75f));
        g.fillEllipse(knobRect.translated(0.0f, 3.5f));

        juce::ColourGradient metalGrad(juce::Colour(0xff2c3140), centreX - knobRadius * 0.5f, centreY - knobRadius,
                                       juce::Colour(0xff0e1017), centreX + knobRadius * 0.5f, centreY + knobRadius, false);
        g.setGradientFill(metalGrad);
        g.fillEllipse(knobRect);

        // Anillo interior de bronce/oro
        g.setColour(juce::Colour(0xffd4af37).withAlpha(0.45f));
        g.drawEllipse(knobRect, 1.5f);
        g.setColour(juce::Colour(0xff090a0f));
        g.fillEllipse(knobRect.reduced(5.0f));

        // 4. Línea indicadora láser en la perilla
        float pointerAngle = toAngle - juce::MathConstants<float>::halfPi;
        juce::Point<float> pStart(centreX + (knobRadius * 0.28f) * std::cos(pointerAngle),
                                  centreY + (knobRadius * 0.28f) * std::sin(pointerAngle));
        juce::Point<float> pEnd(centreX + (knobRadius - 4.0f) * std::cos(pointerAngle),
                                centreY + (knobRadius - 4.0f) * std::sin(pointerAngle));

        g.setColour(juce::Colour(0xfffff0c2));
        g.drawLine(juce::Line<float>(pStart, pEnd), 2.5f);
    }
};

// ==============================================================================
// // DELUXE EDITOR WITH MELODYNE PIANO-ROLL, STROBE TUNER & STANDALONE PLAYER
// ==============================================================================
class AuraPitchAudioProcessorEditor : public juce::AudioProcessorEditor,
                                      public juce::FileDragAndDropTarget,
                                      private juce::Timer
{
public:
    AuraPitchAudioProcessorEditor(AuraPitchAudioProcessor&);
    ~AuraPitchAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    // Soporte Drag & Drop de archivos WAV/MP3 directo a la ventana
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;

private:
    void timerCallback() override;
    void setupKnob(juce::Slider& slider, juce::Label& label, const juce::String& text, const juce::String& suffix);
    void openAudioFileChooser();
    juce::String midiToNoteName(int midiNote) const;

    AuraPitchAudioProcessor& audioProcessor;
    LuxuryLookAndFeel luxuryLnf;

    juce::Slider centerKnob, driftKnob, vibratoKnob, transitionKnob, formantKnob;
    juce::Label centerLabel, driftLabel, vibratoLabel, transitionLabel, formantLabel;
    juce::ComboBox rootCombo, scaleCombo;

    // Botones de transporte e importación de audio
    juce::TextButton loadBtn { "IMPORT AUDIO" };
    juce::TextButton playBtn { "PLAY / PAUSE" };
    juce::TextButton stopBtn { "STOP" };
    juce::TextButton loopBtn { "LOOP: ON" };
    std::unique_ptr<juce::FileChooser> fileChooser;

    float displayedCenterMidi = 62.0f; // Suavizado visual de cámara para el Piano Roll

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<SliderAttachment> centerAtt, driftAtt, vibratoAtt, transitionAtt, formantAtt;
    std::unique_ptr<ComboAttachment> rootAtt, scaleAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AuraPitchAudioProcessorEditor)
};
