#include <juce_audio_utils/juce_audio_utils.h>
#include "HalftimeCore.h"

class HalftimeProcessor : public juce::AudioProcessor
{
public:
    HalftimeProcessor()
        : AudioProcessor (BusesProperties()
                              .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                              .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
          apvts (*this, nullptr, "params",
                 { std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "cycle", 1 }, "Cycle",
                                                                 juce::StringArray { "1/4", "1/2", "1", "2" }, 1),
                   std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "mix", 1 }, "Mix", 0.0f, 1.0f, 1.0f) })
    {
    }

    void prepareToPlay (double sampleRate, int) override
    {
        sr = sampleRate;
        // longest cycle: 2 bars (8 beats) at the minimum tempo of 30 BPM = 16 s
        core.prepare (2, (int) (sr * 16.0) + 1, (int) (sr * 0.005));
    }

    void releaseResources() override {}

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override
    {
        return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
            && layouts.getMainInputChannelSet() == layouts.getMainOutputChannelSet();
    }

    using AudioProcessor::processBlock;

    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        juce::ScopedNoDenormals noDenormals;
        static constexpr double cycleBeats[] = { 1.0, 2.0, 4.0, 8.0 }; // shortcut: assumes 4/4 bars, use host time signature if needed
        const double beats = cycleBeats[juce::jlimit (0, 3, (int) apvts.getRawParameterValue ("cycle")->load())];

        double bpm = 120.0;
        juce::Optional<double> ppq;
        if (auto* ph = getPlayHead())
            if (auto info = ph->getPosition())
            {
                if (auto b = info->getBpm())
                    bpm = *b;
                if (info->getIsPlaying())
                    ppq = info->getPpqPosition();
            }

        const double samplesPerBeat = sr * 60.0 / juce::jlimit (30.0, 300.0, bpm);
        core.setCycleLength ((int) (beats * samplesPerBeat));

        int hostPos = -1; // stopped or no PPQ: free-run on the internal counter
        if (ppq)
        {
            double inCycle = std::fmod (*ppq, beats);
            if (inCycle < 0.0)
                inCycle += beats;
            hostPos = (int) (inCycle * samplesPerBeat);
        }

        core.process (buffer.getArrayOfWritePointers(), buffer.getNumChannels(), buffer.getNumSamples(), hostPos,
                      apvts.getRawParameterValue ("mix")->load());
    }

    juce::AudioProcessorEditor* createEditor() override { return new juce::GenericAudioProcessorEditor (*this); }
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& dest) override
    {
        if (auto xml = apvts.copyState().createXml())
            copyXmlToBinary (*xml, dest);
    }

    void setStateInformation (const void* data, int size) override
    {
        if (auto xml = getXmlFromBinary (data, size))
            if (xml->hasTagName (apvts.state.getType()))
                apvts.replaceState (juce::ValueTree::fromXml (*xml));
    }

private:
    juce::AudioProcessorValueTreeState apvts;
    HalftimeCore core;
    double sr = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HalftimeProcessor)
};

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new HalftimeProcessor(); }
