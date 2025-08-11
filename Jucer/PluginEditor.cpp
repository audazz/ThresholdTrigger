#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
LevelMeter::LevelMeter(ThresholdTriggerAudioProcessor& processor)
    : audioProcessor(processor)
{
    startTimerHz(60); // 60 FPS refresh rate
}

void LevelMeter::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Background
    g.setColour(juce::Colour(0xff1a1a1a));
    g.fillRoundedRectangle(bounds, 4.0f);
    
    // Border
    g.setColour(juce::Colour(0xff404040));
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
    
    // Level bar
    if (currentLevel > 0.0f)
    {
        float levelHeight = juce::jmap(juce::Decibels::gainToDecibels(currentLevel), 
                                      -60.0f, 0.0f, 0.0f, bounds.getHeight());
        
        juce::Rectangle<float> levelRect(bounds.getX() + 2, 
                                        bounds.getBottom() - levelHeight - 2,
                                        bounds.getWidth() - 4, 
                                        levelHeight);
        
        // Color based on trigger state
        if (isTriggered)
            g.setColour(juce::Colour(0xff4CAF50)); // Green when triggered
        else
            g.setColour(juce::Colour(0xff2196F3)); // Blue when not triggered
            
        g.fillRoundedRectangle(levelRect, 2.0f);
    }
    
    // Threshold line
    float thresholdHeight = juce::jmap(thresholdLevel, -60.0f, 0.0f, 0.0f, bounds.getHeight());
    float thresholdY = bounds.getBottom() - thresholdHeight;
    
    g.setColour(juce::Colour(0xffFF5722)); // Orange threshold line
    g.drawLine(bounds.getX() + 2, thresholdY, bounds.getRight() - 2, thresholdY, 2.0f);
    
    // Threshold value text
    g.setColour(juce::Colours::white);
    g.setFont(10.0f);
    g.drawText(juce::String(thresholdLevel, 1) + " dB", 
               bounds.getX() + 4, thresholdY - 12, 60, 12, 
               juce::Justification::left);
}

void LevelMeter::resized()
{
}

void LevelMeter::timerCallback()
{
    currentLevel = audioProcessor.getCurrentLevel();
    isTriggered = audioProcessor.getTriggerState();
    repaint();
}

//==============================================================================
ThresholdTriggerAudioProcessorEditor::ThresholdTriggerAudioProcessorEditor (ThresholdTriggerAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), levelMeter(p)
{
    setSize (400, 300);
    
    // Setup sliders and labels
    setupSlider(thresholdSlider, thresholdLabel, "Threshold");
    setupSlider(attackSlider, attackLabel, "Attack");
    setupSlider(decaySlider, decayLabel, "Decay");
    
    // Setup retrigger toggle
    addAndMakeVisible(retriggerToggle);
    addAndMakeVisible(retriggerLabel);
    
    retriggerToggle.setButtonText("Retrigger");
    retriggerToggle.setColour(juce::ToggleButton::textColourId, textColour);
    retriggerToggle.setColour(juce::ToggleButton::tickColourId, sliderColour);
    retriggerToggle.setColour(juce::ToggleButton::tickDisabledColourId, juce::Colour(0xff404040));
    
    retriggerLabel.setText("Allow Retrigger", juce::dontSendNotification);
    retriggerLabel.setFont(juce::Font(12.0f));
    retriggerLabel.setColour(juce::Label::textColourId, textColour);
    retriggerLabel.setJustificationType(juce::Justification::centred);
    
    // Setup trigger mode combo
    addAndMakeVisible(triggerModeCombo);
    addAndMakeVisible(triggerModeLabel);
    
    triggerModeCombo.addItem("Audio", 1);
    triggerModeCombo.addItem("MIDI", 2);
    triggerModeCombo.addItem("Audio + MIDI", 3);
    triggerModeCombo.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff1a1a1a));
    triggerModeCombo.setColour(juce::ComboBox::textColourId, textColour);
    triggerModeCombo.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff404040));
    triggerModeCombo.setColour(juce::ComboBox::arrowColourId, sliderColour);
    
    triggerModeLabel.setText("Trigger Mode", juce::dontSendNotification);
    triggerModeLabel.setFont(juce::Font(12.0f));
    triggerModeLabel.setColour(juce::Label::textColourId, textColour);
    triggerModeLabel.setJustificationType(juce::Justification::centred);
    
    // Create parameter attachments
    thresholdAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "threshold", thresholdSlider);
    attackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "attack", attackSlider);
    decayAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "decay", decaySlider);
    retriggerAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.getValueTreeState(), "retrigger", retriggerToggle);
    triggerModeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.getValueTreeState(), "midiMode", triggerModeCombo);
    
    // Setup level meter
    addAndMakeVisible(levelMeter);
    
    // Update threshold display on level meter
    thresholdSlider.onValueChange = [this]()
    {
        levelMeter.setThreshold(thresholdSlider.getValue());
    };
    
    // Initial threshold value
    levelMeter.setThreshold(thresholdSlider.getValue());
}

ThresholdTriggerAudioProcessorEditor::~ThresholdTriggerAudioProcessorEditor()
{
}

void ThresholdTriggerAudioProcessorEditor::setupSlider(juce::Slider& slider, juce::Label& label, const juce::String& labelText)
{
    addAndMakeVisible(slider);
    addAndMakeVisible(label);
    
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
    slider.setColour(juce::Slider::rotarySliderFillColourId, sliderColour);
    slider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff404040));
    slider.setColour(juce::Slider::textBoxTextColourId, textColour);
    slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff1a1a1a));
    slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xff404040));
    
    label.setText(labelText, juce::dontSendNotification);
    label.setFont(juce::Font(14.0f, juce::Font::bold));
    label.setColour(juce::Label::textColourId, textColour);
    label.setJustificationType(juce::Justification::centred);
    label.attachToComponent(&slider, false);
}

//==============================================================================
void ThresholdTriggerAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll(backgroundColour);
    
    // Title
    g.setColour(textColour);
    g.setFont(juce::Font(20.0f, juce::Font::bold));
    g.drawText("Threshold Trigger", 0, 10, getWidth(), 30, juce::Justification::centred);
    
    // Decorative elements
    g.setColour(sliderColour);
    g.fillRoundedRectangle(20, 45, getWidth() - 40, 2, 1.0f);
}

void ThresholdTriggerAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop(60); // Space for title
    
    // Level meter area
    auto meterBounds = bounds.removeFromRight(80);
    meterBounds.reduce(10, 10);
    levelMeter.setBounds(meterBounds);
    
    // Controls area
    auto controlsBounds = bounds.reduced(20, 10);
    
    // Arrange sliders in a row
    auto sliderWidth = controlsBounds.getWidth() / 3;
    auto sliderHeight = 120;
    
    // Position sliders
    thresholdSlider.setBounds(controlsBounds.getX(), 
                             controlsBounds.getY() + 20, 
                             sliderWidth - 10, 
                             sliderHeight);
    
    attackSlider.setBounds(controlsBounds.getX() + sliderWidth, 
                          controlsBounds.getY() + 20, 
                          sliderWidth - 10, 
                          sliderHeight);
    
    decaySlider.setBounds(controlsBounds.getX() + sliderWidth * 2, 
                         controlsBounds.getY() + 20, 
                         sliderWidth - 10, 
                         sliderHeight);
    
    // Position retrigger toggle and trigger mode combo below sliders
    auto toggleBounds = controlsBounds.removeFromBottom(80);
    
    // Left side: retrigger toggle
    auto retriggerBounds = toggleBounds.removeFromLeft(toggleBounds.getWidth() / 2);
    retriggerToggle.setBounds(retriggerBounds.getX() + (retriggerBounds.getWidth() - 100) / 2, 
                             retriggerBounds.getY() + 10, 
                             100, 24);
    
    retriggerLabel.setBounds(retriggerToggle.getX(),
                            retriggerToggle.getBottom() + 2,
                            retriggerToggle.getWidth(),
                            16);
    
    // Right side: trigger mode combo
    auto modeBounds = toggleBounds;
    triggerModeCombo.setBounds(modeBounds.getX() + (modeBounds.getWidth() - 120) / 2,
                              modeBounds.getY() + 10,
                              120, 24);
    
    triggerModeLabel.setBounds(triggerModeCombo.getX(),
                              triggerModeCombo.getBottom() + 2,
                              triggerModeCombo.getWidth(),
                              16);
}