#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
class LevelMeter : public juce::Component, public juce::Timer
{
public:
    LevelMeter(ThresholdTriggerAudioProcessor& processor);
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;
    
    void setThreshold(float threshold) { thresholdLevel = threshold; }
    
private:
    ThresholdTriggerAudioProcessor& audioProcessor;
    float currentLevel = 0.0f;
    float thresholdLevel = 0.0f;
    bool isTriggered = false;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LevelMeter)
};

//==============================================================================
class ThresholdTriggerAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    ThresholdTriggerAudioProcessorEditor (ThresholdTriggerAudioProcessor&);
    ~ThresholdTriggerAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    ThresholdTriggerAudioProcessor& audioProcessor;
    
    // Controls
    juce::Slider thresholdSlider;
    juce::Slider attackSlider;
    juce::Slider decaySlider;
    juce::ToggleButton retriggerToggle;
    juce::ComboBox triggerModeCombo;
    
    juce::Label thresholdLabel;
    juce::Label attackLabel;
    juce::Label decayLabel;
    juce::Label retriggerLabel;
    juce::Label triggerModeLabel;
    
    // Attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> thresholdAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> decayAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> retriggerAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> triggerModeAttachment;
    
    // Level meter
    LevelMeter levelMeter;
    
    // Styling
    juce::Colour backgroundColour = juce::Colour(0xff2d2d2d);
    juce::Colour sliderColour = juce::Colour(0xff4a90e2);
    juce::Colour textColour = juce::Colours::white;
    
    void setupSlider(juce::Slider& slider, juce::Label& label, const juce::String& labelText);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ThresholdTriggerAudioProcessorEditor)
};
