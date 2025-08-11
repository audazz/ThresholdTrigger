#pragma once

#include <JuceHeader.h>

//==============================================================================
class ThresholdTriggerAudioProcessor : public juce::AudioProcessor
{
public:
    //==============================================================================
    ThresholdTriggerAudioProcessor();
    ~ThresholdTriggerAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
#endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    juce::AudioProcessorValueTreeState& getValueTreeState() { return valueTreeState; }
    
    // Getter for current trigger state (for GUI visualization)
    bool getTriggerState() const { return isTriggered; }
    float getCurrentLevel() const { return currentLevel; }
    bool getMidiTriggerState() const { return midiTriggered; }

private:
    //==============================================================================
    juce::AudioProcessorValueTreeState valueTreeState;
    
    juce::String pluginVersion;
  //  juce::StreamingSocket logSocket;
    // Parameters
    std::atomic<float>* thresholdParam;
    std::atomic<float>* attackParam;
    std::atomic<float>* decayParam;
    std::atomic<float>* retriggerParam;
    std::atomic<float>* midiModeParam;
    
    // Internal state
    float currentLevel = 0.0f;
    bool isTriggered = false;
    bool wasTriggered = false;
    
    // Envelope follower
    float envelopeLevel = 0.0f;
    float attackCoeff = 0.0f;
    float decayCoeff = 0.0f;
    
    // Envelope state
    enum EnvelopeState { Attack, Decay, Idle };
    EnvelopeState envelopeState = Idle;
    
    // MIDI trigger state
    bool midiTriggered = false;
    bool wasMidiTriggered = false;
    
    // Sample rate
    double sampleRate = 44100.0;
    
    // Helper functions
    void updateCoefficients();
    float processEnvelope(float inputLevel);
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ThresholdTriggerAudioProcessor)
};
