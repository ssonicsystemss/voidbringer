#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

namespace Branding
{
    // what the user sees (the identity codes in CMakeLists.txt never change)
    inline constexpr const char* productName = "voidbringer";
    inline constexpr const char* companyName = "sonicsystems";
    inline constexpr const char* displayName = "voidbringer";
    inline constexpr const char* presetName  = "Rawstyle Terror Screech";
}

namespace PID
{
    // voice
    inline constexpr const char* voiceMode     = "voiceMode";
    inline constexpr const char* glide         = "glide";
    inline constexpr const char* bendRange     = "bendRange";
    inline constexpr const char* octave        = "octave";
    // FM operators
    inline constexpr const char* carRatio      = "carrierRatio";
    inline constexpr const char* modRatio      = "modulatorRatio";
    inline constexpr const char* modFine       = "modulatorFine";
    inline constexpr const char* fmAmount      = "fmAmount";
    inline constexpr const char* feedback      = "feedback";
    inline constexpr const char* stereoDetune  = "stereoDetune";
    // modulation envelopes
    inline constexpr const char* fmEnvAmount   = "fmEnvAmount";
    inline constexpr const char* fmEnvDecay    = "fmEnvDecay";
    inline constexpr const char* pitchEnvAmount= "pitchEnvAmount";
    inline constexpr const char* pitchEnvTime  = "pitchEnvTime";
    // LFO
    inline constexpr const char* lfoWave       = "lfoWave";
    inline constexpr const char* lfoSync       = "lfoSync";
    inline constexpr const char* lfoRate       = "lfoRate";
    inline constexpr const char* lfoDivision   = "lfoDivision";
    inline constexpr const char* lfoDepth      = "lfoDepth";
    inline constexpr const char* lfoDest       = "lfoDestination";
    // filter
    inline constexpr const char* filterType    = "filterType";
    inline constexpr const char* cutoff        = "cutoff";
    inline constexpr const char* resonance     = "resonance";
    inline constexpr const char* filterEnvAmount = "filterEnvAmount";
    inline constexpr const char* filterEnvDecay  = "filterEnvDecay";
    // distortion / compression / output
    inline constexpr const char* drive         = "drive";
    inline constexpr const char* distMix       = "distortionMix";
    inline constexpr const char* ott           = "ottAmount";
    inline constexpr const char* outGain       = "outputGain";
    inline constexpr const char* softClip      = "softClip";
    // amp envelope
    inline constexpr const char* ampAttack     = "ampAttack";
    inline constexpr const char* ampDecay      = "ampDecay";
    inline constexpr const char* ampSustain    = "ampSustain";
    inline constexpr const char* ampRelease    = "ampRelease";
    // v3 (appended)
    inline constexpr const char* polyphony     = "polyphony";
}

// LFO tempo divisions (in beats = quarter notes)
inline const juce::StringArray& lfoDivisionNames()
{
    static const juce::StringArray n { "1/1 Bar", "1/2 Note", "1/4 Note", "1/8 Note", "1/16 Note", "1/32 Note", "1/8 Triplet", "1/16 Triplet" };
    return n;
}
inline double lfoDivisionBeats (int i)
{
    static const double b[] = { 4.0, 2.0, 1.0, 0.5, 0.25, 0.125, 1.0 / 3.0, 1.0 / 6.0 };
    return b[juce::jlimit (0, 7, i)];
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
