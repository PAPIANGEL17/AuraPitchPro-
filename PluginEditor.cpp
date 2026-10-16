#include "PluginProcessor.h"
#include "PluginEditor.h"

// ==============================================================================
// // UI IMPLEMENTATION - DELUXE VOCAL WORKSTATION
// ==============================================================================
AuraPitchAudioProcessorEditor::AuraPitchAudioProcessorEditor(AuraPitchAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setLookAndFeel(&luxuryLnf);
    setSize(920, 580);

    setupKnob(centerKnob,     centerLabel,     "PITCH CENTER", " %");
    setupKnob(driftKnob,      driftLabel,      "PITCH DRIFT",  " %");
    setupKnob(vibratoKnob,    vibratoLabel,    "NATURAL EXPR", " x");
    setupKnob(transitionKnob, transitionLabel, "TRANSITION",   " s");
    setupKnob(formantKnob,    formantLabel,    "FORMANT SHIFT"," st");

    addAndMakeVisible(rootCombo);
    rootCombo.addItemList({ "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }, 1);

    addAndMakeVisible(scaleCombo);
    scaleCombo.addItemList({ "Chromatic (Auto-Free)", "Major", "Minor (Urban/Natural)" }, 1);

    centerAtt     = std::make_unique<SliderAttachment>(audioProcessor.apvts, "CENTER_SNAP", centerKnob);
    driftAtt      = std::make_unique<SliderAttachment>(audioProcessor.apvts, "DRIFT_CORR", driftKnob);
    vibratoAtt    = std::make_unique<SliderAttachment>(audioProcessor.apvts, "VIBRATO_PRESERVE", vibratoKnob);
    transitionAtt = std::make_unique<SliderAttachment>(audioProcessor.apvts, "TRANSITION_SMOOTH", transitionKnob);
    formantAtt    = std::make_unique<SliderAttachment>(audioProcessor.apvts, "FORMANT_SHIFT", formantKnob);
    rootAtt       = std::make_unique<ComboAttachment>(audioProcessor.apvts, "ROOT_NOTE", rootCombo);
    scaleAtt      = std::make_unique<ComboAttachment>(audioProcessor.apvts, "SCALE_TYPE", scaleCombo);

    // // CONFIGURAR BARRA DE IMPORTACIÓN Y TRANSPORTE STANDALONE
    addAndMakeVisible(loadBtn);
    loadBtn.onClick = [this]() { openAudioFileChooser(); };

    addAndMakeVisible(playBtn);
    playBtn.onClick = [this]() { audioProcessor.togglePlayback(); };

    addAndMakeVisible(stopBtn);
    stopBtn.onClick = [this]() { audioProcessor.stopPlayback(); };

    addAndMakeVisible(loopBtn);
    loopBtn.setClickingTogglesState(true);
    loopBtn.setToggleState(true, juce::dontSendNotification);
    loopBtn.onClick = [this]()
    {
        bool state = loopBtn.getToggleState();
        audioProcessor.setLooping(state);
        loopBtn.setButtonText(state ? "LOOP: ON" : "LOOP: OFF");
    };

    startTimerHz(60);
}

AuraPitchAudioProcessorEditor::~AuraPitchAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void AuraPitchAudioProcessorEditor::setupKnob(juce::Slider& slider, juce::Label& label,
                                              const juce::String& text, const juce::String& suffix)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 72, 20);
    slider.setTextValueSuffix(suffix);
    slider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xffffdf73));
    slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff0b0d13));
    slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xff232736));
    addAndMakeVisible(slider);

    label.setText(text, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setFont(juce::FontOptions(11.5f, juce::Font::bold));
    label.setColour(juce::Label::textColourId, juce::Colour(0xffb5bac9));
    addAndMakeVisible(label);
}

void AuraPitchAudioProcessorEditor::openAudioFileChooser()
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Select a Vocal Audio File (WAV, MP3, AIFF, FLAC)...",
        juce::File::getSpecialLocation(juce::File::userDesktopDirectory),
        "*.wav;*.mp3;*.aiff;*.flac;*.ogg");

    auto folderChooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
    fileChooser->launchAsync(folderChooserFlags, [this](const juce::FileChooser& chooser)
    {
        auto file = chooser.getResult();
        if (file.existsAsFile())
        {
            audioProcessor.loadAudioFile(file);
            repaint();
        }
    });
}

bool AuraPitchAudioProcessorEditor::isInterestedInFileDrag(const juce::StringArray& files)
{
    for (const auto& f : files)
        if (f.endsWithIgnoreCase(".wav") || f.endsWithIgnoreCase(".mp3") ||
            f.endsWithIgnoreCase(".flac") || f.endsWithIgnoreCase(".aiff"))
            return true;
    return false;
}

void AuraPitchAudioProcessorEditor::filesDropped(const juce::StringArray& files, int, int)
{
    if (files.size() > 0)
        audioProcessor.loadAudioFile(juce::File(files[0]));
}

juce::String AuraPitchAudioProcessorEditor::midiToNoteName(int midiNote) const
{
    const char* names[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    int n = (midiNote + 1200) % 12;
    int oct = (midiNote / 12) - 1;
    return juce::String(names[n]) + juce::String(oct);
}

void AuraPitchAudioProcessorEditor::timerCallback()
{
    float targetNote = audioProcessor.currentMidiNote.load();
    displayedCenterMidi = 0.92f * displayedCenterMidi + 0.08f * targetNote;
    repaint();
}

void AuraPitchAudioProcessorEditor::paint(juce::Graphics& g)
{
    // 1. CHASIS DE ALUMINIO ANODIZADO NEGRO OBSIDIANA
    juce::ColourGradient chassisGrad(juce::Colour(0xff141822), 0.0f, 0.0f,
                                     juce::Colour(0xff090b10), 0.0f, static_cast<float>(getHeight()), false);
    g.setGradientFill(chassisGrad);
    g.fillAll();

    // Bisel exterior dorado de lujo y tornillos rack
    g.setColour(juce::Colour(0xffd4af37).withAlpha(0.75f));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(5.0f), 8.0f, 1.5f);
    g.setColour(juce::Colour(0xff232838));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(8.0f), 6.0f, 1.0f);

    // 2. HEADER BAR & BRANDING
    g.setColour(juce::Colour(0xfff5e6c8));
    g.setFont(juce::FontOptions(24.0f, juce::Font::bold));
    g.drawText("AURAPITCH PRO", 26, 16, 260, 28, juce::Justification::centredLeft);

    g.setColour(juce::Colour(0xffd4af37));
    g.setFont(juce::FontOptions(10.5f, juce::Font::bold));
    g.drawText("DELUXE WSOLA VOCAL PITCH & FORMANT WORKSTATION", 28, 42, 340, 14, juce::Justification::centredLeft);

    // 3. MEDIDOR ESTROBOSCÓPICO DE CENTÉSIMAS Y NOTA ACTUAL (TOP CENTER)
    juce::Rectangle<float> tunerBox(345.0f, 15.0f, 255.0f, 44.0f);
    g.setColour(juce::Colour(0xff08090d));
    g.fillRoundedRectangle(tunerBox, 6.0f);
    g.setColour(juce::Colour(0xffd4af37).withAlpha(0.45f));
    g.drawRoundedRectangle(tunerBox, 6.0f, 1.2f);

    int activeMidi = static_cast<int>(std::round(audioProcessor.currentMidiNote.load()));
    float cents = juce::jlimit(-50.0f, 50.0f, audioProcessor.currentCents.load());

    g.setColour(juce::Colour(0xffffdf73));
    g.setFont(juce::FontOptions(20.0f, juce::Font::bold));
    g.drawText(midiToNoteName(activeMidi), static_cast<int>(tunerBox.getX() + 10), static_cast<int>(tunerBox.getY() + 6), 54, 32, juce::Justification::centred);

    // Barra de precisión de Cents (-50 a +50)
    float meterX = tunerBox.getX() + 72.0f;
    float meterW = 115.0f;
    float meterY = tunerBox.getCentreY();
    g.setColour(juce::Colour(0xff1e2230));
    g.fillRoundedRectangle(meterX, meterY - 4.0f, meterW, 8.0f, 4.0f);
    g.setColour(juce::Colour(0xff26d0ce));
    g.drawVerticalLine(static_cast<int>(meterX + meterW * 0.5f), meterY - 8.0f, meterY + 8.0f);

    float needleX = meterX + (meterW * 0.5f) + (cents / 50.0f) * (meterW * 0.45f);
    g.setColour(std::abs(cents) < 8.0f ? juce::Colour(0xff26d0ce) : juce::Colour(0xffffdf73));
    g.fillEllipse(needleX - 5.0f, meterY - 5.0f, 10.0f, 10.0f);

    // Sibilant Guard LED
    bool sibilant = audioProcessor.isSibilantBypass.load();
    g.setColour(sibilant ? juce::Colour(0xff26d0ce) : juce::Colour(0xff4a5168));
    g.setFont(juce::FontOptions(9.5f, juce::Font::bold));
    g.drawText(sibilant ? "SIBILANT" : "GUARD OK", static_cast<int>(tunerBox.getRight() - 62), static_cast<int>(tunerBox.getY() + 12), 56, 20, juce::Justification::centred);

    // 4. BARRA DE REPRODUCTOR STANDALONE (DRAG & DROP O IMPORTAR AUDIO)
    juce::Rectangle<float> transportBar(26.0f, 68.0f, static_cast<float>(getWidth() - 52), 34.0f);
    g.setColour(juce::Colour(0xff0d0f16));
    g.fillRoundedRectangle(transportBar, 6.0f);
    g.setColour(juce::Colour(0xff252a3a));
    g.drawRoundedRectangle(transportBar, 6.0f, 1.0f);

    g.setColour(audioProcessor.hasLoadedFile() ? juce::Colour(0xffffdf73) : juce::Colour(0xff7d849b));
    g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    juce::String fileStatus = audioProcessor.hasLoadedFile()
        ? ("LOADED: " + audioProcessor.getLoadedFileName() + (audioProcessor.isPlayingAudio() ? "  [PLAYING]" : "  [PAUSED]"))
        : "STANDALONE & DAW PLAYER: Drag & Drop WAV/MP3 here or click IMPORT AUDIO";
    g.drawText(fileStatus, 435, 71, 445, 28, juce::Justification::centredLeft);

    // ==============================================================================
    // // 5. VISOR PIANO-ROLL PROFESIONAL CON BLOBS VOLUMÉTRICOS ESTILO MELODYNE
    // ==============================================================================
    juce::Rectangle<float> displayRect(26.0f, 110.0f, static_cast<float>(getWidth() - 52), 265.0f);
    g.setColour(juce::Colour(0xff07080c));
    g.fillRoundedRectangle(displayRect, 8.0f);

    const float pianoKeyWidth = 54.0f;
    const int numVisibleNotes = 11; // Rango de 11 semitonos en pantalla
    float laneHeight = displayRect.getHeight() / static_cast<float>(numVisibleNotes);
    int centerMidiInt = static_cast<int>(std::round(displayedCenterMidi));
    int rootNote = static_cast<int>(audioProcessor.apvts.getRawParameterValue("ROOT_NOTE")->load());
    int scaleType = static_cast<int>(audioProcessor.apvts.getRawParameterValue("SCALE_TYPE")->load());

    const bool majorMask[12] = {true, false, true, false, true, true, false, true, false, true, false, true};
    const bool minorMask[12] = {true, false, true, true, false, true, false, true, true, false, true, false};

    // Dibujar carriles de notas y teclado lateral izquierdo
    for (int i = 0; i < numVisibleNotes; ++i)
    {
        int noteMidi = centerMidiInt + (numVisibleNotes / 2) - i;
        float laneTop = displayRect.getY() + i * laneHeight;
        int rel = (noteMidi - rootNote + 1200) % 12;
        bool inScale = (scaleType == 0) ? true : (scaleType == 1 ? majorMask[rel] : minorMask[rel]);
        int noteInOct = (noteMidi + 1200) % 12;
        bool isBlackKey = (noteInOct == 1 || noteInOct == 3 || noteInOct == 6 || noteInOct == 8 || noteInOct == 10);

        // Carril horizontal (más claro si pertenece a la escala activa, ej. D Minor)
        g.setColour(inScale ? juce::Colour(0xff10141f) : juce::Colour(0xff08090e));
        g.fillRect(displayRect.getX() + pianoKeyWidth, laneTop, displayRect.getWidth() - pianoKeyWidth, laneHeight);

        g.setColour(juce::Colour(0xff191e2d));
        g.drawHorizontalLine(static_cast<int>(laneTop), displayRect.getX() + pianoKeyWidth, displayRect.getRight());

        // Tecla de Piano a la izquierda
        juce::Rectangle<float> keyRect(displayRect.getX(), laneTop, pianoKeyWidth, laneHeight);
        g.setColour(isBlackKey ? juce::Colour(0xff12151f) : juce::Colour(0xff1e2333));
        g.fillRect(keyRect.reduced(0.5f));

        // Indicador dorado si la nota está en la escala
        if (inScale)
        {
            g.setColour(juce::Colour(0xffd4af37).withAlpha(0.85f));
            g.fillRect(displayRect.getX() + pianoKeyWidth - 4.0f, laneTop + 1.0f, 4.0f, laneHeight - 2.0f);
        }

        g.setColour(inScale ? juce::Colour(0xfff5e6c8) : juce::Colour(0xff52586e));
        g.setFont(juce::FontOptions(11.0f, inScale ? juce::Font::bold : juce::Font::plain));
        g.drawText(midiToNoteName(noteMidi), keyRect.reduced(6.0f, 0.0f), juce::Justification::centredLeft);
    }

    // Líneas verticales de tiempo tipo cuadrícula DAW
    float canvasX = displayRect.getX() + pianoKeyWidth;
    float canvasW = displayRect.getWidth() - pianoKeyWidth;
    g.setColour(juce::Colour(0xff151926));
    for (int gx = 1; gx < 8; ++gx)
    {
        float xPos = canvasX + (canvasW / 8.0f) * static_cast<float>(gx);
        g.drawVerticalLine(static_cast<int>(xPos), displayRect.getY(), displayRect.getBottom());
    }

    // // DIBUJAR CÁPSULAS DE NOTA (BLOBS MELODYNE) Y CURVAS SUAVIZADAS
    const auto& history = audioProcessor.visualHistory;
    int histSize = static_cast<int>(history.size());
    int readIdx = audioProcessor.historyWriteIndex.load();
    float stepX = canvasW / static_cast<float>(histSize);

    auto midiToY = [&](float midiVal) -> float
    {
        float topMidi = static_cast<float>(centerMidiInt + (numVisibleNotes / 2)) + 0.5f;
        float norm = (topMidi - midiVal) / static_cast<float>(numVisibleNotes);
        return juce::jlimit(displayRect.getY() + 4.0f, displayRect.getBottom() - 4.0f,
                            displayRect.getY() + norm * displayRect.getHeight());
    };

    juce::Path blobBodyPath;
    juce::Path originalPitchPath;
    juce::Path correctedPitchPath;
    bool lineActive = false;

    for (int i = 0; i < histSize; ++i)
    {
        int idx = (readIdx + i) % histSize;
        const auto& pt = history[idx];
        float x = canvasX + static_cast<float>(i) * stepX;

        if (pt.voiced && pt.amplitude > 0.015f)
        {
            float yTargetNote = midiToY(pt.targetNoteMidi);
            float yOrig       = midiToY(pt.originalMidi);
            float yCorr       = midiToY(pt.correctedMidi);
            float halfThick   = juce::jlimit(3.0f, laneHeight * 0.45f, pt.amplitude * laneHeight * 0.48f);

            // Relleno de cápsula (Blob) anclado en la nota objetivo de la grilla
            juce::Rectangle<float> sliceRect(x, yTargetNote - halfThick, stepX + 1.2f, halfThick * 2.0f);
            g.setColour(juce::Colour(0xffd4af37).withAlpha(0.28f));
            g.fillRect(sliceRect);

            // Borde luminoso superior e inferior del Blob
            g.setColour(juce::Colour(0xffffdf73).withAlpha(0.65f));
            g.fillRect(x, yTargetNote - halfThick, stepX + 1.0f, 1.5f);
            g.fillRect(x, yTargetNote + halfThick - 1.5f, stepX + 1.0f, 1.5f);

            if (!lineActive)
            {
                originalPitchPath.startNewSubPath(x, yOrig);
                correctedPitchPath.startNewSubPath(x, yCorr);
                lineActive = true;
            }
            else
            {
                originalPitchPath.lineTo(x, yOrig);
                correctedPitchPath.lineTo(x, yCorr);
            }
        }
        else
        {
            lineActive = false;
        }
    }

    // Curva de voz original desafinada (Cian translúcido para contraste claro)
    g.setColour(juce::Colour(0xff26d0ce).withAlpha(0.55f));
    g.strokePath(originalPitchPath, juce::PathStrokeType(1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Curva de voz afinada por AuraPitch Pro (Oro blanco brillante)
    g.setColour(juce::Colour(0xfffff2c6));
    g.strokePath(correctedPitchPath, juce::PathStrokeType(2.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Marco metálico del visor
    g.setColour(juce::Colour(0xffd4af37).withAlpha(0.5f));
    g.drawRoundedRectangle(displayRect, 8.0f, 1.5f);

    // 6. PANEL DE RACK INFERIOR PARA LAS 5 PERILLAS DELUXE
    juce::Rectangle<float> rackPanel(26.0f, 386.0f, static_cast<float>(getWidth() - 52), 172.0f);
    juce::ColourGradient rackGrad(juce::Colour(0xff12151f), rackPanel.getX(), rackPanel.getY(),
                                  juce::Colour(0xff0a0c12), rackPanel.getX(), rackPanel.getBottom(), false);
    g.setGradientFill(rackGrad);
    g.fillRoundedRectangle(rackPanel, 8.0f);
    g.setColour(juce::Colour(0xff262c3f));
    g.drawRoundedRectangle(rackPanel, 8.0f, 1.2f);
}

void AuraPitchAudioProcessorEditor::resized()
{
    // Selectores de Escala (Arriba a la derecha)
    rootCombo.setBounds(615, 22, 85, 30);
    scaleCombo.setBounds(710, 22, 184, 30);

    // Barra de Transporte Standalone (Debajo del header)
    loadBtn.setBounds(36, 73, 118, 24);
    playBtn.setBounds(162, 73, 102, 24);
    stopBtn.setBounds(272, 73, 68, 24);
    loopBtn.setBounds(348, 73, 76, 24);

    // 5 Perillas Deluxe en el rack inferior
    int knobY = 396;
    int knobW = 128;
    int knobH = 128;
    int spacing = 168;
    int startX = 58;

    centerKnob.setBounds(startX, knobY, knobW, knobH);
    centerLabel.setBounds(startX, knobY + knobH + 2, knobW, 20);

    driftKnob.setBounds(startX + spacing, knobY, knobW, knobH);
    driftLabel.setBounds(startX + spacing, knobY + knobH + 2, knobW, 20);

    vibratoKnob.setBounds(startX + spacing * 2, knobY, knobW, knobH);
    vibratoLabel.setBounds(startX + spacing * 2, knobY + knobH + 2, knobW, 20);

    transitionKnob.setBounds(startX + spacing * 3, knobY, knobW, knobH);
    transitionLabel.setBounds(startX + spacing * 3, knobY + knobH + 2, knobW, 20);

    formantKnob.setBounds(startX + spacing * 4, knobY, knobW, knobH);
    formantLabel.setBounds(startX + spacing * 4, knobY + knobH + 2, knobW, 20);
}
