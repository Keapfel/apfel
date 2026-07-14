#include "PluginEditor.h"

// ─────────────────────────────────────────────────────────────────────────────
// Colour palette
// ─────────────────────────────────────────────────────────────────────────────
static const juce::Colour kBg        { 0xff141414 }; // main background
static const juce::Colour kPanel     { 0xff1e1e1e }; // knob panel area
static const juce::Colour kAccent    { 0xffFF4500 }; // orange-red (chaos)
static const juce::Colour kBlue      { 0xff00aaff }; // blue (normal knobs)
static const juce::Colour kText      { 0xffCCCCCC }; // primary text
static const juce::Colour kTextDim   { 0xff666666 }; // secondary / label text
static const juce::Colour kBtnActive { 0xffFF4500 };
static const juce::Colour kBtnIdle   { 0xff2b2b2b };
static const juce::Colour kBtnBorder { 0xff404040 };

// ─────────────────────────────────────────────────────────────────────────────
// ChaosLookAndFeel
// ─────────────────────────────────────────────────────────────────────────────
ChaosLookAndFeel::ChaosLookAndFeel (bool isChaosKnob) : forChaos (isChaosKnob)
{
    setColour (juce::Slider::thumbColourId,              juce::Colours::white);
    setColour (juce::Slider::rotarySliderFillColourId,   forChaos ? kAccent : kBlue);
    setColour (juce::Slider::rotarySliderOutlineColourId,juce::Colour (0xff3a3a3a));
    setColour (juce::Slider::textBoxTextColourId,        kText);
    setColour (juce::Slider::textBoxOutlineColourId,     juce::Colour (0xff383838));
    setColour (juce::Slider::textBoxBackgroundColourId,  juce::Colour (0xff1a1a1a));
    setColour (juce::TextButton::buttonColourId,         kBtnIdle);
    setColour (juce::TextButton::buttonOnColourId,       kBtnActive);
    setColour (juce::TextButton::textColourOffId,        kTextDim);
    setColour (juce::TextButton::textColourOnId,         juce::Colours::white);
    setColour (juce::Label::textColourId,                kText);
}

void ChaosLookAndFeel::drawRotarySlider (juce::Graphics& g,
                                          int x, int y, int w, int h,
                                          float pos,
                                          float startAng, float endAng,
                                          juce::Slider& /*slider*/)
{
    float radius  = (float)juce::jmin (w, h) * 0.44f;
    float cx      = x + w * 0.5f;
    float cy      = y + h * 0.5f;

    // ── Background glow for chaos knob ────────────────────────────────────
    if (forChaos && pos > 0.01f)
    {
        float glow = radius * 1.15f * (0.5f + pos * 0.5f);
        juce::ColourGradient grad (kAccent.withAlpha (0.18f * pos), cx, cy,
                                   kAccent.withAlpha (0.0f),
                                   cx + glow, cy + glow, true);
        g.setGradientFill (grad);
        g.fillEllipse (cx - glow, cy - glow, glow * 2.0f, glow * 2.0f);
    }

    // ── Base circle ───────────────────────────────────────────────────────
    g.setColour (juce::Colour (0xff252525));
    g.fillEllipse (cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);

    g.setColour (juce::Colour (0xff3e3e3e));
    g.drawEllipse (cx - radius, cy - radius, radius * 2.0f, radius * 2.0f, 1.5f);

    // ── Track arc (background) ────────────────────────────────────────────
    {
        juce::Path track;
        track.addCentredArc (cx, cy, radius * 0.78f, radius * 0.78f,
                             0.0f, startAng, endAng, true);
        g.setColour (juce::Colour (0xff333333));
        g.strokePath (track, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));
    }

    // ── Value arc ─────────────────────────────────────────────────────────
    {
        float fillEnd = startAng + pos * (endAng - startAng);
        juce::Path arc;
        arc.addCentredArc (cx, cy, radius * 0.78f, radius * 0.78f,
                           0.0f, startAng, fillEnd, true);
        g.setColour (forChaos ? kAccent : kBlue);
        g.strokePath (arc, juce::PathStrokeType (3.5f, juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));
    }

    // ── Thumb dot ─────────────────────────────────────────────────────────
    float thumbAngle = startAng + pos * (endAng - startAng);
    float tx = cx + std::sin (thumbAngle) * radius * 0.65f;
    float ty = cy - std::cos (thumbAngle) * radius * 0.65f;
    float tr = forChaos ? 5.5f : 4.0f;

    g.setColour (juce::Colours::white.withAlpha (0.9f));
    g.fillEllipse (tx - tr, ty - tr, tr * 2.0f, tr * 2.0f);
}

void ChaosLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& btn,
                                              const juce::Colour& /*bg*/,
                                              bool highlighted, bool down)
{
    auto b  = btn.getLocalBounds().toFloat().reduced (0.5f);
    bool on = btn.getToggleState();

    juce::Colour fill;
    if      (on)          fill = kBtnActive;
    else if (down)        fill = juce::Colour (0xff555555);
    else if (highlighted) fill = juce::Colour (0xff3a3a3a);
    else                  fill = kBtnIdle;

    g.setColour (fill);
    g.fillRoundedRectangle (b, 5.0f);

    g.setColour (on ? kAccent.brighter (0.3f) : kBtnBorder);
    g.drawRoundedRectangle (b, 5.0f, 1.0f);
}

void ChaosLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& btn, bool, bool)
{
    g.setFont (juce::Font (juce::FontOptions {}.withHeight (11.5f).withStyle ("Bold")));
    g.setColour (btn.getToggleState() ? juce::Colours::white : kTextDim);
    g.drawText (btn.getButtonText(), btn.getLocalBounds(), juce::Justification::centred);
}

juce::Font ChaosLookAndFeel::getLabelFont (juce::Label&)
{
    return juce::Font (juce::FontOptions {}.withHeight (10.5f).withStyle ("Bold"));
}

// ─────────────────────────────────────────────────────────────────────────────
// ChaosChopperAudioProcessorEditor
// ─────────────────────────────────────────────────────────────────────────────
ChaosChopperAudioProcessorEditor::ChaosChopperAudioProcessorEditor (ChaosChopperAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setSize (520, 370);
    setResizable (false, false);

    // ── Title ─────────────────────────────────────────────────────────────
    titleLabel.setText ("CHAOS CHOPPER", juce::dontSendNotification);
    titleLabel.setFont (juce::Font (juce::FontOptions {}.withHeight (22.0f).withStyle ("Bold")));
    titleLabel.setColour (juce::Label::textColourId, kAccent);
    titleLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (titleLabel);

    // ── Chop toggle ────────────────────────────────────────────────────────
    chopToggle.setButtonText ("CHOP");
    chopToggle.setClickingTogglesState (true);
    chopToggle.setLookAndFeel (&lafNormal);
    addAndMakeVisible (chopToggle);
    chopAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>
        (proc.apvts, "chopEnabled", chopToggle);

    // ── Division buttons ───────────────────────────────────────────────────
    for (int i = 0; i < NUM_DIVS; ++i)
    {
        divBtn[i].setButtonText (kDivLabels[i]);
        divBtn[i].setClickingTogglesState (false);
        divBtn[i].setLookAndFeel (&lafNormal);
        divBtn[i].onClick = [this, i]()
        {
            // Set the choice parameter via normalised value
            auto* param = proc.apvts.getParameter ("chopDivision");
            param->setValueNotifyingHost (param->convertTo0to1 ((float)i));
            updateDivisionButtons();
        };
        addAndMakeVisible (divBtn[i]);
    }

    // ── Chaos knob ─────────────────────────────────────────────────────────
    chaosKnob.setSliderStyle (juce::Slider::Rotary);
    chaosKnob.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    chaosKnob.setLookAndFeel (&lafChaos);
    addAndMakeVisible (chaosKnob);
    chaosAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
        (proc.apvts, "chaos", chaosKnob);

    chaosLabel.setText ("CHAOS", juce::dontSendNotification);
    chaosLabel.setFont (juce::Font (juce::FontOptions {}.withHeight (13.0f).withStyle ("Bold")));
    chaosLabel.setColour (juce::Label::textColourId, kAccent);
    chaosLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (chaosLabel);

    // ── Helper lambdas ─────────────────────────────────────────────────────
    auto setupKnob = [&](juce::Slider& s)
    {
        s.setSliderStyle (juce::Slider::Rotary);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 16);
        s.setLookAndFeel (&lafNormal);
        addAndMakeVisible (s);
    };

    auto setupLabel = [&](juce::Label& l, const juce::String& text)
    {
        l.setText (text, juce::dontSendNotification);
        l.setFont (juce::Font (juce::FontOptions {}.withHeight (10.0f).withStyle ("Bold")));
        l.setColour (juce::Label::textColourId, kTextDim);
        l.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (l);
    };

    // ── Side knobs ─────────────────────────────────────────────────────────
    setupKnob (pitchKnob);    setupLabel (pitchLabel,   "PITCH");
    setupKnob (formantKnob);  setupLabel (formantLabel, "FORMANT");
    setupKnob (clipKnob);     setupLabel (clipLabel,    "DRIVE");
    setupKnob (dryWetKnob);   setupLabel (dryWetLabel,  "DRY/WET");

    pitchAtt   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
        (proc.apvts, "pitch",     pitchKnob);
    formantAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
        (proc.apvts, "formant",   formantKnob);
    clipAtt    = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
        (proc.apvts, "clipDrive", clipKnob);
    dryWetAtt  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
        (proc.apvts, "dryWet",    dryWetKnob);

    updateDivisionButtons();
    startTimerHz (20); // poll division param for highlight sync
}

ChaosChopperAudioProcessorEditor::~ChaosChopperAudioProcessorEditor()
{
    stopTimer();
    // Detach LookAndFeel before destruction to avoid dangling pointers
    chaosKnob.setLookAndFeel (nullptr);
    pitchKnob.setLookAndFeel (nullptr);
    formantKnob.setLookAndFeel (nullptr);
    clipKnob.setLookAndFeel (nullptr);
    dryWetKnob.setLookAndFeel (nullptr);
    chopToggle.setLookAndFeel (nullptr);
    for (auto& b : divBtn) b.setLookAndFeel (nullptr);
}

// ─── Timer callback ───────────────────────────────────────────────────────────
void ChaosChopperAudioProcessorEditor::timerCallback()
{
    updateDivisionButtons();
    // Repaint for chaos glow animation
    if (chaosKnob.getValue() > 0.01)
        chaosKnob.repaint();
}

void ChaosChopperAudioProcessorEditor::updateDivisionButtons()
{
    int cur = (int)*proc.apvts.getRawParameterValue ("chopDivision");
    for (int i = 0; i < NUM_DIVS; ++i)
        divBtn[i].setToggleState (i == cur, juce::dontSendNotification);
}

// ─── Paint ───────────────────────────────────────────────────────────────────
void ChaosChopperAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Background
    g.fillAll (kBg);

    // Subtle panel behind the center chaos area
    auto cBounds = chaosKnob.getBounds().expanded (18, 8);
    g.setColour (kPanel);
    g.fillRoundedRectangle (cBounds.toFloat(), 10.0f);
    g.setColour (kAccent.withAlpha (0.18f));
    g.drawRoundedRectangle (cBounds.toFloat(), 10.0f, 1.0f);

    // Separator under title row
    g.setColour (juce::Colour (0xff2e2e2e));
    g.drawHorizontalLine (46, 8.0f, (float)getWidth() - 8);

    // Separator under division row
    g.drawHorizontalLine (90, 8.0f, (float)getWidth() - 8);
}

// ─── Layout ───────────────────────────────────────────────────────────────────
void ChaosChopperAudioProcessorEditor::resized()
{
    const int W = getWidth();   // 520
    //const int H = getHeight();  // 370

    // ── Row 1 : title + chop toggle (y 8..44) ────────────────────────────
    titleLabel.setBounds  (14, 10, 300, 32);
    chopToggle.setBounds  (W - 88, 12, 78, 26);

    // ── Row 2 : division buttons (y 52..88) ──────────────────────────────
    {
        const int btnY = 53, btnH = 28;
        // 5 div buttons spread from x=10 to x=W-10
        // leave gap of 10 between last div button and end (no separate chop btn here)
        const int totalW  = W - 20;                      // 500
        const int btnW    = (totalW - 4 * 4) / 5;        // ≈ 97
        const int gap     = 4;
        for (int i = 0; i < NUM_DIVS; ++i)
            divBtn[i].setBounds (10 + i * (btnW + gap), btnY, btnW, btnH);
    }

    // ── Main area (y 97..365) ─────────────────────────────────────────────
    // Left column: x 10, w 105
    const int mainY  = 97;
    const int knobW  = 105;
    const int knobH  = 108;
    const int lblH   = 14;
    const int lx     = 10;
    const int rx     = W - lx - knobW;   // 405

    // Left labels + knobs
    pitchLabel.setBounds   (lx, mainY,           knobW, lblH);
    pitchKnob.setBounds    (lx, mainY + lblH,     knobW, knobH);
    formantLabel.setBounds (lx, mainY + 140,      knobW, lblH);
    formantKnob.setBounds  (lx, mainY + 140 + lblH, knobW, knobH);

    // Right labels + knobs
    clipLabel.setBounds    (rx, mainY,           knobW, lblH);
    clipKnob.setBounds     (rx, mainY + lblH,     knobW, knobH);
    dryWetLabel.setBounds  (rx, mainY + 140,      knobW, lblH);
    dryWetKnob.setBounds   (rx, mainY + 140 + lblH, knobW, knobH);

    // Centre: chaos knob
    const int cxStart = lx + knobW + 8;
    const int cxEnd   = rx - 8;
    const int cxW     = cxEnd - cxStart;            // ≈ 274
    const int cSize   = juce::jmin (cxW - 4, 220);  // 220
    const int cxOff   = cxStart + (cxW - cSize) / 2;
    const int cMainH  = 262; // total main area height
    const int cyOff   = mainY + (cMainH - cSize - 22) / 2;

    chaosKnob.setBounds  (cxOff, cyOff,          cSize, cSize);
    chaosLabel.setBounds (cxOff, cyOff + cSize,   cSize, 20);
}
