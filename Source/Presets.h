// voidbringer factory presets, picked from the PRESET box in the top-right corner of the header.
// Each preset lists only the parameters it changes (real values: index for choices, 0/1 for switches);
// every other parameter goes back to its default. Deliberately editor-only (getNumPrograms() stays 1), so a host
// program change can never overwrite a saved sound. The render test checks every ID exists and every preset is clean.
#pragma once
#include "Params.h"
#include <utility>
#include <vector>

struct FactoryPreset
{
    const char* name;
    std::vector<std::pair<const char*, float>> values;
};

inline const std::vector<FactoryPreset>& factoryPresets()
{
    static const std::vector<FactoryPreset> presets {
        { "Rawstyle Terror Screech", {} },   // the factory defaults
        { "Uptempo Screech 200", { { "glide", 30.0f }, { "fmAmount", 60.0f }, { "feedback", 65.0f }, { "lfoSync", 1.0f }, { "lfoDivision", 4.0f }, { "lfoDepth", 40.0f }, { "lfoDestination", 1.0f }, { "drive", 30.0f }, { "ottAmount", 55.0f }, { "cutoff", 2400.0f }, { "resonance", 55.0f }, { "ampRelease", 60.0f }, { "outputGain", -6.0f } } },
        { "Terror Laser", { { "pitchEnvAmount", 36.0f }, { "pitchEnvTime", 60.0f }, { "fmEnvAmount", 80.0f }, { "fmEnvDecay", 120.0f }, { "modulatorRatio", 3.0f }, { "feedback", 70.0f }, { "drive", 36.0f }, { "filterType", 0.0f }, { "cutoff", 6000.0f }, { "resonance", 30.0f }, { "outputGain", -6.0f } } },
        { "Formant Talker", { { "filterType", 2.0f }, { "fmAmount", 40.0f }, { "feedback", 35.0f }, { "lfoDestination", 2.0f }, { "lfoDepth", 45.0f }, { "lfoDivision", 2.0f }, { "cutoff", 1200.0f }, { "resonance", 60.0f }, { "drive", 20.0f } } },
        { "Wobble Screech 1/16", { { "lfoWave", 2.0f }, { "lfoDivision", 4.0f }, { "lfoDestination", 2.0f }, { "lfoDepth", 70.0f }, { "cutoff", 2000.0f }, { "resonance", 60.0f }, { "drive", 28.0f } } },
        { "Hoover Mono", { { "modulatorRatio", 0.5f }, { "fmAmount", 55.0f }, { "feedback", 30.0f }, { "stereoDetune", 30.0f }, { "glide", 120.0f }, { "lfoSync", 0.0f }, { "lfoRate", 5.0f }, { "lfoDestination", 0.0f }, { "lfoDepth", 10.0f }, { "filterType", 0.0f }, { "cutoff", 3500.0f }, { "drive", 18.0f }, { "ottAmount", 30.0f } } },
        { "Clean FM Lead", { { "fmAmount", 25.0f }, { "feedback", 20.0f }, { "drive", 8.0f }, { "distortionMix", 50.0f }, { "ottAmount", 20.0f }, { "filterType", 0.0f }, { "cutoff", 8000.0f }, { "resonance", 20.0f }, { "lfoDepth", 0.0f }, { "outputGain", -4.0f } } },
        { "Siren Screech", { { "lfoSync", 1.0f }, { "lfoDivision", 1.0f }, { "lfoWave", 1.0f }, { "lfoDestination", 0.0f }, { "lfoDepth", 35.0f }, { "fmAmount", 50.0f }, { "feedback", 60.0f }, { "drive", 26.0f }, { "cutoff", 2200.0f }, { "outputGain", -6.0f } } },
        { "Metal Shard", { { "modulatorRatio", 3.5f }, { "modulatorFine", 15.0f }, { "fmAmount", 70.0f }, { "feedback", 75.0f }, { "fmEnvAmount", 60.0f }, { "fmEnvDecay", 250.0f }, { "filterType", 1.0f }, { "cutoff", 3200.0f }, { "resonance", 50.0f }, { "drive", 32.0f }, { "outputGain", -7.0f } } },
        { "Random Glitch 1/32", { { "lfoWave", 4.0f }, { "lfoDivision", 5.0f }, { "lfoDestination", 1.0f }, { "lfoDepth", 75.0f }, { "fmAmount", 55.0f }, { "feedback", 50.0f }, { "cutoff", 2600.0f }, { "drive", 28.0f }, { "outputGain", -6.0f } } },
        { "Vowel Wah Triplet", { { "filterType", 2.0f }, { "lfoWave", 0.0f }, { "lfoDivision", 6.0f }, { "lfoDestination", 2.0f }, { "lfoDepth", 60.0f }, { "cutoff", 1000.0f }, { "resonance", 65.0f }, { "fmAmount", 45.0f }, { "drive", 22.0f } } },
        { "Pitch Dive Kick-Lead", { { "voiceMode", 0.0f }, { "pitchEnvAmount", -24.0f }, { "pitchEnvTime", 150.0f }, { "fmEnvAmount", 70.0f }, { "fmEnvDecay", 200.0f }, { "drive", 30.0f }, { "filterType", 0.0f }, { "cutoff", 5000.0f }, { "ampAttack", 1.0f }, { "outputGain", -6.0f } } },
        { "Dark Growl Bass", { { "octave", -1.0f }, { "modulatorRatio", 1.0f }, { "fmAmount", 60.0f }, { "feedback", 40.0f }, { "filterType", 0.0f }, { "cutoff", 900.0f }, { "resonance", 40.0f }, { "filterEnvAmount", 40.0f }, { "drive", 34.0f }, { "ottAmount", 50.0f }, { "outputGain", -6.0f } } },
        { "Poly Screech Chords", { { "polyphony", 1.0f }, { "fmAmount", 35.0f }, { "feedback", 35.0f }, { "stereoDetune", 20.0f }, { "filterType", 0.0f }, { "cutoff", 4000.0f }, { "drive", 18.0f }, { "ampRelease", 300.0f }, { "outputGain", -8.0f } } },
        { "Short Stab Pluck", { { "voiceMode", 0.0f }, { "ampAttack", 1.0f }, { "ampDecay", 180.0f }, { "ampSustain", 0.0f }, { "ampRelease", 80.0f }, { "fmEnvAmount", 60.0f }, { "fmEnvDecay", 150.0f }, { "filterType", 0.0f }, { "cutoff", 4500.0f }, { "drive", 24.0f } } },
        { "Slow Swell Screech", { { "ampAttack", 900.0f }, { "ampRelease", 700.0f }, { "lfoDivision", 2.0f }, { "lfoDepth", 25.0f }, { "fmAmount", 50.0f }, { "cutoff", 2000.0f }, { "drive", 24.0f }, { "outputGain", -6.0f } } },
    };
    return presets;
}

// Message thread only (called from the editor). Remembers the name in the state so the box shows it again.
inline void applyFactoryPreset (juce::AudioProcessorValueTreeState& s, int index)
{
    const auto& all = factoryPresets();
    const auto& p = all[(size_t) juce::jlimit (0, (int) all.size() - 1, index)];
    for (auto* prm : s.processor.getParameters())
        if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (prm))
        {
            float v = r->getDefaultValue();
            for (const auto& [id, real] : p.values)
                if (r->paramID == id) v = r->convertTo0to1 (real);
            r->beginChangeGesture();
            r->setValueNotifyingHost (v);
            r->endChangeGesture();
        }
    s.state.setProperty ("preset", juce::String (p.name), nullptr);
}
