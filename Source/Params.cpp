#include "Params.h"
#include "OwnSound.h"

using APF = juce::AudioParameterFloat;
using APC = juce::AudioParameterChoice;
using API = juce::AudioParameterInt;
using Attr = juce::AudioParameterFloatAttributes;

namespace
{
    juce::ParameterID id (const char* s) { return juce::ParameterID { s, 1 }; }
    juce::String sign (float v, int dec) { return (v > 0.0005f ? "+" : "") + juce::String (v, dec); }

    std::unique_ptr<APF> dbParam (const char* pid, const char* name, float lo, float hi, float def)
    {
        return std::make_unique<APF> (id (pid), name, juce::NormalisableRange<float> (lo, hi, 0.01f), def,
            Attr().withLabel ("dB").withStringFromValueFunction ([] (float v, int) { return sign (v, 1) + " dB"; }));
    }

    std::unique_ptr<APF> pctParam (const char* pid, const char* name, float lo, float hi, float def)
    {
        const bool bipolar = lo < 0.0f;
        return std::make_unique<APF> (id (pid), name, juce::NormalisableRange<float> (lo, hi, 0.1f), def,
            Attr().withLabel ("%").withStringFromValueFunction ([bipolar] (float v, int)
            { return (bipolar ? sign (v, 0) : juce::String (juce::roundToInt (v))) + " %"; }));
    }

    std::unique_ptr<APF> msParam (const char* pid, const char* name, float lo, float hi, float centre, float def)
    {
        juce::NormalisableRange<float> r (lo, hi);
        r.setSkewForCentre (centre);
        return std::make_unique<APF> (id (pid), name, r, def,
            Attr().withLabel ("ms").withStringFromValueFunction ([] (float v, int)
            { return v < 10.0f ? juce::String (v, 1) + " ms" : juce::String (juce::roundToInt (v)) + " ms"; }));
    }

    std::unique_ptr<APF> hzParam (const char* pid, const char* name, float lo, float hi, float centre, float def, int decimalsBelow10 = 0)
    {
        juce::NormalisableRange<float> r (lo, hi);
        r.setSkewForCentre (centre);
        return std::make_unique<APF> (id (pid), name, r, def,
            Attr().withLabel ("Hz")
                .withStringFromValueFunction ([decimalsBelow10] (float v, int)
                {
                    if (v < 10.0f && decimalsBelow10 > 0) return juce::String (v, decimalsBelow10) + " Hz";
                    return v < 1000.0f ? juce::String (juce::roundToInt (v)) + " Hz" : juce::String (v / 1000.0f, 2) + " kHz";
                })
                .withValueFromStringFunction ([] (const juce::String& t)
                {
                    const float v = t.getFloatValue();
                    return t.containsIgnoreCase ("k") ? v * 1000.0f : v;
                }));
    }

    std::unique_ptr<APF> stParam (const char* pid, const char* name, float lo, float hi, float def, float step)
    {
        return std::make_unique<APF> (id (pid), name, juce::NormalisableRange<float> (lo, hi, step), def,
            Attr().withLabel ("semitones").withStringFromValueFunction ([step] (float v, int)
            { return sign (v, step < 1.0f ? 1 : 0) + " semitones"; }));
    }

    std::unique_ptr<APF> ratioParam (const char* pid, const char* name, float lo, float hi, float centre, float def)
    {
        juce::NormalisableRange<float> r (lo, hi, 0.01f);
        r.setSkewForCentre (centre);
        return std::make_unique<APF> (id (pid), name, r, def,
            Attr().withLabel ("x").withStringFromValueFunction ([] (float v, int) { return "x " + juce::String (v, 2); })
                  .withValueFromStringFunction ([] (const juce::String& t) { return t.retainCharacters ("0123456789.").getFloatValue(); }));
    }
}

// Default values = "Rawstyle Terror Screech" preset
juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    // ---- voice
    l.add (std::make_unique<APC> (id (PID::voiceMode), "Voice Mode", juce::StringArray { "Mono (retrigger)", "Legato (slide)" }, 1));
    l.add (msParam (PID::glide, "Glide Time", 0.0f, 1000.0f, 150.0f, 45.0f));
    l.add (std::make_unique<APF> (id (PID::bendRange), "Pitch Bend Range", juce::NormalisableRange<float> (0.0f, 24.0f, 1.0f), 2.0f,
            Attr().withLabel ("semitones").withStringFromValueFunction ([] (float v, int) { return juce::String (juce::roundToInt (v)) + " semitones"; })));
    l.add (std::make_unique<API> (id (PID::octave), "Octave Shift", -2, 2, 0,
            juce::AudioParameterIntAttributes().withLabel ("octaves")
                .withStringFromValueFunction ([] (int v, int) { return (v > 0 ? "+" : "") + juce::String (v) + " octaves"; })));
    // ---- FM operators
    l.add (ratioParam (PID::carRatio, "Carrier Frequency Ratio", 0.5f, 8.0f, 2.0f, 1.0f));
    l.add (ratioParam (PID::modRatio, "Modulator Frequency Ratio", 0.25f, 16.0f, 2.5f, 2.0f));
    l.add (std::make_unique<APF> (id (PID::modFine), "Modulator Fine Tune", juce::NormalisableRange<float> (-100.0f, 100.0f, 0.1f), 7.0f,
            Attr().withLabel ("cents").withStringFromValueFunction ([] (float v, int) { return sign (v, 1) + " cents"; })));
    l.add (pctParam (PID::fmAmount, "FM Amount", 0.0f, 100.0f, 45.0f));
    l.add (pctParam (PID::feedback, "Modulator Feedback", 0.0f, 100.0f, 55.0f));
    l.add (std::make_unique<APF> (id (PID::stereoDetune), "Stereo Detune", juce::NormalisableRange<float> (0.0f, 50.0f, 0.1f), 12.0f,
            Attr().withLabel ("cents").withStringFromValueFunction ([] (float v, int) { return juce::String (v, 1) + " cents"; })));
    // ---- modulation envelopes
    l.add (pctParam (PID::fmEnvAmount, "FM Envelope Amount", -100.0f, 100.0f, 40.0f));
    l.add (msParam (PID::fmEnvDecay, "FM Envelope Decay Time", 5.0f, 5000.0f, 400.0f, 260.0f));
    l.add (stParam (PID::pitchEnvAmount, "Pitch Envelope Amount", -48.0f, 48.0f, 12.0f, 0.1f));
    l.add (msParam (PID::pitchEnvTime, "Pitch Envelope Time", 1.0f, 2000.0f, 200.0f, 90.0f));
    // ---- LFO
    l.add (std::make_unique<APC> (id (PID::lfoWave), "LFO Waveform", juce::StringArray { "Sine", "Triangle", "Square", "Saw Down", "Random Steps" }, 0));
    l.add (std::make_unique<APC> (id (PID::lfoSync), "LFO Tempo Sync", juce::StringArray { "Off (free Hz)", "On (host tempo)" }, 1));
    l.add (hzParam (PID::lfoRate, "LFO Rate", 0.05f, 30.0f, 5.0f, 6.0f, 2));
    l.add (std::make_unique<APC> (id (PID::lfoDivision), "LFO Sync Division", lfoDivisionNames(), 3));
    l.add (pctParam (PID::lfoDepth, "LFO Depth", 0.0f, 100.0f, 30.0f));
    l.add (std::make_unique<APC> (id (PID::lfoDest), "LFO Destination", juce::StringArray { "Pitch", "FM Amount", "Filter Cutoff" }, 1));
    // ---- filter
    l.add (std::make_unique<APC> (id (PID::filterType), "Filter Type", juce::StringArray { "Resonant Lowpass", "Resonant Bandpass", "Formant (Vowel)" }, 1));
    l.add (hzParam (PID::cutoff, "Filter Cutoff", 40.0f, 18000.0f, 1500.0f, 1900.0f));
    l.add (pctParam (PID::resonance, "Filter Resonance", 0.0f, 100.0f, 45.0f));
    l.add (pctParam (PID::filterEnvAmount, "Filter Envelope Amount", -100.0f, 100.0f, 25.0f));
    l.add (msParam (PID::filterEnvDecay, "Filter Envelope Decay Time", 5.0f, 5000.0f, 400.0f, 300.0f));
    // ---- distortion / compression / output
    l.add (dbParam (PID::drive, "Distortion Drive", 0.0f, 48.0f, 24.0f));
    l.add (pctParam (PID::distMix, "Distortion Mix", 0.0f, 100.0f, 100.0f));
    l.add (pctParam (PID::ott, "Multiband Compression Amount", 0.0f, 100.0f, 40.0f));
    l.add (dbParam (PID::outGain, "Output Gain", -36.0f, 6.0f, -5.0f));
    l.add (std::make_unique<APC> (id (PID::softClip), "Output Soft Clipper", juce::StringArray { "Off", "On" }, 1));
    // ---- amp envelope
    l.add (msParam (PID::ampAttack, "Amp Attack Time", 0.5f, 5000.0f, 200.0f, 1.0f));
    l.add (msParam (PID::ampDecay, "Amp Decay Time", 1.0f, 5000.0f, 400.0f, 300.0f));
    l.add (pctParam (PID::ampSustain, "Amp Sustain Level", 0.0f, 100.0f, 90.0f));
    l.add (msParam (PID::ampRelease, "Amp Release Time", 1.0f, 5000.0f, 400.0f, 90.0f));
    // ---- v3, appended: Off = the classic mono/legato screech (old projects unchanged); On = up to 6 voices for chords
    l.add (std::make_unique<APC> (id (PID::polyphony), "Polyphony", juce::StringArray { "Off (mono screech)", "On (chords, 6 voices)" }, 0));

    // ---- appended (2026-10-17): OWN SOUND, the user's own sample (ownLevel, ownTune, ownKey, ownMode)
    Sonic::OwnSound::addParameters (l, true);
    return l;
}
