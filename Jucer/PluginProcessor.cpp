#include "PluginProcessor.h"
#include "PluginEditor.h"

/*
#define LOG(msg) NETDBG(msg,0);
#define NETDBG(str, msg) do { \
    juce::String message = juce::String(str) + juce::String(msg); \
    logSocket.write(message.toRawUTF8(), static_cast<int> (message.getNumBytesAsUTF8())); \
} while (false)
*/
//==============================================================================
ThresholdTriggerAudioProcessor::ThresholdTriggerAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
#endif
      valueTreeState(*this, nullptr, "Parameters", createParameterLayout())
{
    // Get parameter pointers
    thresholdParam = valueTreeState.getRawParameterValue("threshold");
    attackParam = valueTreeState.getRawParameterValue("attack");
    decayParam = valueTreeState.getRawParameterValue("decay");
    retriggerParam = valueTreeState.getRawParameterValue("retrigger");
    midiModeParam = valueTreeState.getRawParameterValue("midiMode");
/*
    if (logSocket.connect("127.0.0.1", 6000))
        {
            logSocket.write("Hello, Debug Window!\n", 22);
        }
        
*/
          pluginVersion = ProjectInfo::versionString;
          
}

ThresholdTriggerAudioProcessor::~ThresholdTriggerAudioProcessor()
{
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout ThresholdTriggerAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "threshold",
        "Threshold",
        juce::NormalisableRange<float>(-60.0f, 0.0f, 0.1f),
        -20.0f,
        "dB"
    ));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "attack",
        "Attack",
        juce::NormalisableRange<float>(0.1f, 1000.0f, 0.1f, 0.3f),
        10.0f,
        "ms"
    ));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "decay",
        "Decay",
        juce::NormalisableRange<float>(1.0f, 5000.0f, 1.0f, 0.3f),
        500.0f,
        "ms"
    ));
    
    layout.add(std::make_unique<juce::AudioParameterBool>(
        "retrigger",
        "Retrigger",
        true
    ));
    
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "midiMode",
        "Trigger Mode",
        juce::StringArray { "Audio", "MIDI", "Audio + MIDI" },
        0  // Default to Audio mode
    ));
    
    return layout;
}

//==============================================================================
const juce::String ThresholdTriggerAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool ThresholdTriggerAudioProcessor::acceptsMidi() const
{
    return true;  // Enable MIDI input
}

bool ThresholdTriggerAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool ThresholdTriggerAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double ThresholdTriggerAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int ThresholdTriggerAudioProcessor::getNumPrograms()
{
    return 1;
}

int ThresholdTriggerAudioProcessor::getCurrentProgram()
{
    return 0;
}

void ThresholdTriggerAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String ThresholdTriggerAudioProcessor::getProgramName (int index)
{
    return {};
}

void ThresholdTriggerAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void ThresholdTriggerAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    this->sampleRate = sampleRate;
    updateCoefficients();
}

void ThresholdTriggerAudioProcessor::releaseResources()
{
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool ThresholdTriggerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    #if ! JucePlugin_IsSynth
     if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
      && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    #endif

    return true;
  #endif
}
#endif

void ThresholdTriggerAudioProcessor::updateCoefficients()
{
    float attackTimeMs = *attackParam;
    float decayTimeMs = *decayParam;
    
    // Convert time constants to coefficients
    attackCoeff = 1.0f - std::exp(-1.0f / (attackTimeMs * 0.001f * sampleRate));
    decayCoeff = 1.0f - std::exp(-1.0f / (decayTimeMs * 0.001f * sampleRate));
}

float ThresholdTriggerAudioProcessor::processEnvelope(float inputLevel)
{
    bool allowRetrigger = *retriggerParam > 0.5f;
    int triggerMode = static_cast<int>(*midiModeParam);  // 0=Audio, 1=MIDI, 2=Audio+MIDI
    
    // Determine trigger source based on mode
    bool shouldTrigger = false;
    bool wasTriggeredPreviously = false;
   /* DBG("midiTriggered: " << (midiTriggered ? "Yes" : "No")
        << ", wasMidiTriggered: " << (wasMidiTriggered ? "Yes" : "No")
        << ", shouldTrigger: " << (shouldTrigger ? "Yes" : "No")
        << ", wasTriggeredPreviously: " << (wasTriggeredPreviously ? "Yes" : "No"));*/
    switch (triggerMode)
    {
        case 0: // Audio only
            shouldTrigger = isTriggered;
            wasTriggeredPreviously = wasTriggered;
           
            break;
        case 1: // MIDI only
            shouldTrigger = midiTriggered;
            wasTriggeredPreviously = wasMidiTriggered;
        
            break;
        case 2: // Audio + MIDI (either can trigger)
            shouldTrigger = isTriggered || midiTriggered;
            wasTriggeredPreviously = wasTriggered || wasMidiTriggered;
           
            break;
    }
   /* DBG("triggerMode: " << triggerMode
        << ", shouldTrigger: " << (shouldTrigger ? "Yes" : "No")
        << ", wasTriggeredPreviously: " << (wasTriggeredPreviously ? "Yes" : "No"));*/
    
    // Detect new trigger edge using the selected trigger sources
    bool newTriggerDetected = shouldTrigger && !wasTriggeredPreviously;
   /*
    DBG("newTriggerDetected: " << (newTriggerDetected ? "YES" : "NO") 
        << ", envelopeState: " << envelopeState 
        << ", envelopeLevel: " << envelopeLevel);
    */
    // Trigger logic: start attack when new trigger is detected
    if (newTriggerDetected)
    {
        envelopeState = Attack;
       
    }
    
    // Process envelope based on current state
    switch (envelopeState)
    {
        case Attack:
            // Rise towards 1.0 with attack time
            envelopeLevel += attackCoeff * (1.0f - envelopeLevel);
            
            // Switch to decay when we reach near-peak or trigger stops
            if (envelopeLevel >= 0.99f || !shouldTrigger)
            {
                envelopeState = Decay;
            }
            break;
            
        case Decay:
            // Fall towards 0.0 with decay time - this controls the volume throughout decay
            envelopeLevel += decayCoeff * (0.0f - envelopeLevel);
            
            // Switch to idle when envelope is essentially zero
            if (envelopeLevel <= 0.001f)
            {
                envelopeLevel = 0.0f;
                envelopeState = Idle;
            }
            
            // Retrigger if allowed and new trigger detected during decay
            if (allowRetrigger && newTriggerDetected)
            {
                envelopeState = Attack;
            }
            break;
            
        case Idle:
            // Stay at zero until triggered
            envelopeLevel = 0.0f;
            break;
    }
    
    return envelopeLevel;
}

void ThresholdTriggerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear unused output channels
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Update coefficients if parameters changed
    updateCoefficients();
    
    float thresholdDb = *thresholdParam;
    float thresholdLinear = juce::Decibels::decibelsToGain(thresholdDb);
    
    // Process each sample
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        // Process MIDI events at this exact sample position
        for (const auto metadata : midiMessages)
        {
            if (metadata.samplePosition == sample)
            {
           
                auto message = metadata.getMessage();
                
                if (message.isNoteOn())
                {
                    midiTriggered = true;
                  
                }
                else if (message.isNoteOff())
                {
                    midiTriggered = false;
                  
                }
            }
            
        }
    
        // Calculate RMS level across all channels
        float rmsSquared = 0.0f;
        for (int channel = 0; channel < totalNumInputChannels; ++channel)
        {
            float sampleValue = buffer.getSample(channel, sample);
            rmsSquared += sampleValue * sampleValue;
        }
        
        currentLevel = std::sqrt(rmsSquared / totalNumInputChannels);
        
        // Check audio threshold (update current state)
        isTriggered = currentLevel >= thresholdLinear;
      /*  if(midiTriggered)
            isTriggered = midiTriggered;
        else
           isTriggered = currentLevel >= thresholdLinear;*/
        // Process envelope (uses wasTriggered and wasMidiTriggered from previous sample)
        float envelopeOutput = processEnvelope(currentLevel);
        
        // Apply envelope to output
        for (int channel = 0; channel < totalNumInputChannels; ++channel)
        {
            float input = buffer.getSample(channel, sample);
            float output = input * envelopeOutput;
            buffer.setSample(channel, sample, output);
        }
        
        // Store current states as "previous" for next sample (AFTER processEnvelope)
        wasTriggered = isTriggered;
        wasMidiTriggered = midiTriggered;
    }
}

//==============================================================================
bool ThresholdTriggerAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* ThresholdTriggerAudioProcessor::createEditor()
{
    return new ThresholdTriggerAudioProcessorEditor (*this);
}

//==============================================================================
void ThresholdTriggerAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = valueTreeState.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void ThresholdTriggerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName (valueTreeState.state.getType()))
            valueTreeState.replaceState (juce::ValueTree::fromXml (*xmlState));
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ThresholdTriggerAudioProcessor();
}
