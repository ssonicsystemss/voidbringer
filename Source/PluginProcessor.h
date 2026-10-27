#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "ParamCache.h"
#include "NoteStack.h"
#include <juce_dsp/juce_dsp.h>
#include "Params.h"
#include "OwnSound.h"

struct LevelMeter
{
    std::atomic<float> peak[2] { 0.0f, 0.0f };
    void update (int ch, float v) { if (v > peak[ch].load (std::memory_order_relaxed)) peak[ch].store (v, std::memory_order_relaxed); }
    float take (int ch) { return peak[ch].exchange (0.0f); }
};

// Cytomic / Simper TPT state-variable filter (one channel)
struct Svf
{
    float ic1 = 0, ic2 = 0;
    void reset() { ic1 = ic2 = 0; }
    // returns lowpass in lp, bandpass in bp
    inline void process (float v0, float g, float k, float& lp, float& bp)
    {
        const float a1 = 1.0f / (1.0f + g * (g + k)), a2 = g * a1, a3 = g * a2;
        const float v3 = v0 - ic2;
        const float v1 = a1 * ic1 + a2 * v3;
        const float v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.0f * v1 - ic1;
        ic2 = 2.0f * v2 - ic2;
        lp = v2; bp = v1;
    }
};

class VoidbringerProcessor : public juce::AudioProcessor
{
public:
    VoidbringerProcessor();
    ~VoidbringerProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return Branding::productName; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return juce::jmax (0.5, p (PID::ampRelease) * 0.001 + 0.1); }   // amp release (ms)

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return Branding::presetName; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    Sonic::OwnSound own { { false, 60 } };   // OWN SOUND: the user's own sample
    ParamCache params { apvts };   // atomic<float>* of every parameter, resolved once (ParamCache.h)
    LevelMeter outMeter;
    std::atomic<float> clipAmount { 0.0f };    // fraction of samples in the soft-clip knee (last block)

    // oscilloscope ring: written on the audio thread, read by the GUI (relaxed atomics: lock-free, race-free)
    static constexpr int scopeSize = 2048;
    std::atomic<float> scope[scopeSize] {};
    std::atomic<int> scopeWrite { 0 };

private:
    float p (const char* id) const { return params.get (id); }
    void handleMidi (const juce::MidiMessage&);
    void noteOn (int note);
    void noteOff (int note);
    void render (int startBase, int numBase);   // into the oversampled block
    float lfoValue (int wave);

    static constexpr int osOrder = 2;            // 4x oversampling for the FM core
    static constexpr int osFactor = 1 << osOrder;
    double sr = 44100.0, srOS = 176400.0;
    int maxBlock = 512;
    std::unique_ptr<juce::dsp::Oversampling<float>> os;
    float* up[2] { nullptr, nullptr };

    // voice state: voices[0] is the mono/legato voice; with Polyphony = On every key gets its own voice
    struct Voice
    {
        float targetNote = 60.0f, curNote = 60.0f;
        int   key = -1;                        // MIDI key (poly mode)
        bool  active = false, keyDown = false;
        uint32_t age = 0;
        juce::ADSR amp;
        float fmEnv = 0, pitchEnv = 0, filtEnv = 0;
        double carPh[2] {}, modPh[2] {};
        float modPrev[2][2] {};
        Svf svf[2][3];
        float dcX[2] {}, dcY[2] {};
        void resetState()
        {
            for (int c = 0; c < 2; ++c)
            {
                dcX[c] = dcY[c] = 0; carPh[c] = modPh[c] = 0; modPrev[c][0] = modPrev[c][1] = 0;
                for (auto& f : svf[c]) f.reset();
            }
        }
    };
    static constexpr int maxVoices = 6;
    bool polyOn() const { return (int) p (PID::polyphony) == 1; }
    void polyNoteOn (int note);
    void polyNoteOff (int note);
    std::array<Voice, maxVoices> voices;
    uint32_t ageCounter = 0;
    bool lastPoly = false;
    float polyComp = 1.0f;

    NoteStack heldNotes;   // allocation-free (NoteStack.h)
    bool sustain = false;                  // CC64
    std::array<bool, 128> sustainedOff {}; // notes released while the pedal was down
    juce::MidiBuffer chunkMidi;            // preallocated: MIDI slice for a block bigger than maxBlock
    float bend = 0.0f;
    double lfoPh = 0.0;
    float lfoHold = 0.0f;
    juce::Random rng { 1337 };
    float dcR = 0.9999f;
    double bpm = 150.0;

    // multiband compressor ("OTT-ish") at base rate
    float bandLp1[2] {}, bandLp2[2] {}, bandEnv[2][3] {};
    juce::SmoothedValue<float> outG, ottS;
    juce::AudioBuffer<float> monoTmp;   // used when the host gives a mono output bus

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoidbringerProcessor)
};
