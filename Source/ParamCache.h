#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include <cstring>
#include <memory>

// Shared suite helper (byte-identical in every project).
// Resolves every parameter's std::atomic<float>* once, when the processor is built, so the audio thread can read
// parameters by their PID constant without APVTS's String-keyed lookup (which constructs a juce::String, i.e. can
// allocate, on every call). Lookups compare the PID pointer first (fast path, remembered per entry) and fall back
// to strcmp; neither allocates or locks.
class ParamCache
{
public:
    explicit ParamCache (juce::AudioProcessorValueTreeState& state)
    {
        const auto& all = state.processor.getParameters();
        entries.reset (new Entry[(size_t) juce::jmax (1, all.size())]);
        for (auto* prm : all)
            if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (prm))
            {
                auto& e = entries[(size_t) count++];
                e.id = r->paramID;
                e.value = state.getRawParameterValue (e.id);
            }
    }

    std::atomic<float>* find (const char* id) const noexcept
    {
        for (int i = 0; i < count; ++i)
            if (entries[(size_t) i].alias.load (std::memory_order_relaxed) == id) return entries[(size_t) i].value;
        for (int i = 0; i < count; ++i)
            if (std::strcmp (entries[(size_t) i].id.toRawUTF8(), id) == 0)
            {
                entries[(size_t) i].alias.store (id, std::memory_order_relaxed);
                return entries[(size_t) i].value;
            }
        jassertfalse;   // unknown parameter ID
        return &missing;
    }

    float get (const char* id) const noexcept { return find (id)->load (std::memory_order_relaxed); }

private:
    struct Entry
    {
        juce::String id;
        std::atomic<float>* value = nullptr;
        mutable std::atomic<const char*> alias { nullptr };   // the PID pointer last used to look this one up
    };
    std::unique_ptr<Entry[]> entries;
    int count = 0;
    mutable std::atomic<float> missing { 0.0f };

    JUCE_DECLARE_NON_COPYABLE (ParamCache)
};
