#pragma once
// OWN SOUND: the user's own sample (WAV / AIFF / FLAC / MP3 / OGG) inside a sonicsystems instrument. One copy for
// voidbringer (header-only).
//
// - 4 parameters, appended at the END of the plugin's layout: ownLevel, ownTune, ownKey (fixed / follows notes),
//   ownMode (layer on top of the plugin's sound / replace it).
// - The sample is kept inside the plugin state (FLAC, base64, property "ownSound"), so a project or a MY PRESETS
//   file still has it when the original file is moved or deleted. Max 20 s.
// - Audio: the processor calls trigger() / release() where its engine starts / ends a sound (or addMidi() for plain
//   note-driven instruments), then render() at the end of processBlock. Real-time safe: no allocation or blocking
//   lock on the audio thread (the sample is swapped under a SpinLock that the audio thread only try-locks).
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <algorithm>
#include <array>
#include <vector>
#include <atomic>
#include <memory>

namespace Sonic
{
namespace OwnID
{
    inline constexpr const char* level = "ownLevel";
    inline constexpr const char* tune  = "ownTune";
    inline constexpr const char* key   = "ownKey";
    inline constexpr const char* mode  = "ownMode";
}

class OwnSound
{
public:
    struct Config
    {
        bool mono = true;        // a new hit fades the previous one out (kicks, drums); false = up to 16 at once
        int rootNote = 60;       // the note that plays the sample at its own pitch (C5 in FL) when it follows notes
        bool reverse = false;    // plays the sample backwards (warpback: a reverse bass that ends on the kick)
    };
    explicit OwnSound (Config c = {}) : cfg (c) {}

    // note-off ends the sound (25 ms fade) - e.g. synths, or a kick in Note End = CUT. The processor may change it
    // every block (audio thread only).
    bool gate = false;
    // Replace mode clears the whole plugin output before the sample is added. Plugins that mute only one part of
    // their own sound (e.g. the kick in rutherford, one drum in meteorbeat) set this to false and ask replacing().
    bool replaceClearsAll = true;
    // the plugin's latency (oversampling) in samples: the sample starts that much later, in time with the engine
    int latency = 0;

    static void addParameters (juce::AudioProcessorValueTreeState::ParameterLayout& l, bool followNotesByDefault)
    {
        using namespace juce;
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { OwnID::level, 1 }, "Own Sound Level",
               NormalisableRange<float> (-40.0f, 6.0f, 0.1f), 0.0f,
               AudioParameterFloatAttributes().withLabel ("dB").withStringFromValueFunction ([] (float v, int)
               { return v <= -39.95f ? String ("-inf") : String (v, 1) + " dB"; })));
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { OwnID::tune, 1 }, "Own Sound Tune",
               NormalisableRange<float> (-24.0f, 24.0f, 0.01f), 0.0f,
               AudioParameterFloatAttributes().withLabel ("st").withStringFromValueFunction ([] (float v, int)
               { return (v > 0.004f ? "+" : "") + String (v, 2) + " st"; })));
        l.add (std::make_unique<AudioParameterChoice> (ParameterID { OwnID::key, 1 }, "Own Sound Pitch",
               StringArray { "Fixed", "Follows Notes" }, followNotesByDefault ? 1 : 0));
        l.add (std::make_unique<AudioParameterChoice> (ParameterID { OwnID::mode, 1 }, "Own Sound Mode",
               StringArray { "Layer", "Replace" }, 0));
    }

    // processor constructor, after the APVTS exists
    void attach (juce::AudioProcessorValueTreeState& s)
    {
        apvts = &s;
        level = s.getRawParameterValue (OwnID::level);
        tune  = s.getRawParameterValue (OwnID::tune);
        key   = s.getRawParameterValue (OwnID::key);
        mode  = s.getRawParameterValue (OwnID::mode);
        jassert (level != nullptr && tune != nullptr && key != nullptr && mode != nullptr);   // addParameters() missing
    }

    void prepare (double sampleRate)
    {
        hostRate = sampleRate > 0.0 ? sampleRate : 44100.0;
        const juce::SpinLock::ScopedLockType sl (lock);
        for (auto& v : voices) v.active = false;
        numEvents = 0;
    }

    // ------------------------------------------------------------------ message thread
    static constexpr double maxSeconds = 20.0;
    static juce::String fileTypes() { return "*.wav;*.aif;*.aiff;*.flac;*.mp3;*.ogg"; }

    // returns an error text for the status line, empty = loaded
    juce::String loadFile (const juce::File& f)
    {
        juce::AudioFormatManager fm;
        fm.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> r (fm.createReaderFor (f));
        if (r == nullptr) return "can't read " + f.getFileName() + " (use WAV, AIFF, FLAC, MP3 or OGG)";
        if (r->lengthInSamples <= 0 || r->sampleRate <= 0.0) return f.getFileName() + " is empty";
        auto s = readAll (*r, f.getFileNameWithoutExtension());
        const bool cut = (double) r->lengthInSamples > maxSeconds * r->sampleRate + 1.0;
        store (*s);
        install (std::move (s));
        return cut ? "only the first 20 seconds were kept" : juce::String();
    }

    void clear()
    {
        if (apvts != nullptr)
        {
            apvts->state.removeProperty ("ownSound", nullptr);
            apvts->state.removeProperty ("ownSoundName", nullptr);
            apvts->state.removeProperty ("ownSoundGain", nullptr);
        }
        install (nullptr);
    }

    // after setStateInformation (replaceState): brings the stored sample back, or removes it
    void restore()
    {
        if (apvts == nullptr) return;
        const auto text = apvts->state.getProperty ("ownSound").toString();
        juce::MemoryBlock mb;
        if (text.isEmpty() || ! mb.fromBase64Encoding (text)) { install (nullptr); return; }
        juce::FlacAudioFormat flac;
        std::unique_ptr<juce::AudioFormatReader> r (flac.createReaderFor (new juce::MemoryInputStream (mb, false), true));
        if (r == nullptr || r->lengthInSamples <= 0) { install (nullptr); return; }
        auto s = readAll (*r, apvts->state.getProperty ("ownSoundName").toString());
        s->data.applyGain ((float) (double) apvts->state.getProperty ("ownSoundGain", 1.0));
        install (std::move (s));
    }

    // tests: put a buffer in directly (stored in the state like a loaded file)
    void setSound (const juce::AudioBuffer<float>& data, double rate, const juce::String& name)
    {
        auto s = std::make_unique<Sample>();
        s->data.makeCopyOf (data);
        s->rate = rate;
        s->name = name;
        store (*s);
        install (std::move (s));
    }

    bool hasSound() const { return loaded.load(); }
    juce::String name() const { return apvts != nullptr ? apvts->state.getProperty ("ownSoundName").toString() : juce::String(); }
    double seconds() const { return seconds_.load(); }
    std::atomic<int> hits { 0 };   // counts triggers, for the GUI

    // the waveform for the GUI: `points` peak values 0..1 (empty = no sample)
    std::vector<float> peaks (int points) const
    {
        std::vector<float> out ((size_t) juce::jmax (0, points), 0.0f);
        {
            const juce::SpinLock::ScopedLockType sl (lock);
            if (current == nullptr || out.empty()) return {};
            const auto& d = current->data;
            const juce::int64 len = d.getNumSamples();
            for (int i = 0; i < points; ++i)
            {
                const int a = (int) (len * i / points), b = juce::jmax (a + 1, (int) (len * (i + 1) / points));
                for (int ch = 0; ch < d.getNumChannels(); ++ch)
                    out[(size_t) i] = juce::jmax (out[(size_t) i], d.getMagnitude (ch, a, (int) juce::jmin<juce::int64> (b, len) - a));
            }
        }
        const float top = *std::max_element (out.begin(), out.end());
        if (top > 0.0f) for (auto& v : out) v /= top;
        return out;
    }

    // ------------------------------------------------------------------ audio thread
    bool replacing() const noexcept { return loaded.load (std::memory_order_relaxed) && mode != nullptr && mode->load() > 0.5f; }

    // `semis` = extra pitch on top of Tune (e.g. a rising roll); `fit` (reverse only) = the sound must end exactly
    // this many samples later: a longer sample starts part-way in, a shorter one waits
    void trigger (int at, int note, float velocity, float semis = 0.0f, int fit = 0) noexcept { push ({ at, note, velocity, true, semis, fit }); }
    void release (int at, int note) noexcept { push ({ at, note, 0.0f, false }); }   // note -1 = every voice

    // plain note-driven instruments: every note-on plays the sample
    void addMidi (const juce::MidiBuffer& midi) noexcept
    {
        for (const auto md : midi)
        {
            const auto m = md.getMessage();
            if (m.isNoteOn()) trigger (md.samplePosition, m.getNoteNumber(), m.getFloatVelocity());
            else if (m.isNoteOff()) release (md.samplePosition, m.getNoteNumber());
            else if (m.isAllNotesOff() || m.isAllSoundOff()) release (md.samplePosition, -1);
        }
    }

    // adds the sample voices to the buffer (Replace + replaceClearsAll: clears the plugin's own sound first)
    void render (juce::AudioBuffer<float>& buffer) noexcept
    {
        const int n = buffer.getNumSamples();
        if (replaceClearsAll && replacing()) buffer.clear();
        const juce::SpinLock::ScopedTryLockType sl (lock);
        if (! sl.isLocked() || current == nullptr) { numEvents = 0; return; }
        const int due = n;
        if (resetVoices) { for (auto& v : voices) v.active = false; resetVoices = false; }

        const float lv = level->load();
        const float target = lv <= -39.95f ? 0.0f : juce::Decibels::decibelsToGain (lv);
        const float semis = tune->load();
        const bool follow = key->load() > 0.5f;

        // events arrive in time order from one source; a plugin mixing MIDI + engine hits may not: keep them sorted
        std::stable_sort (events.begin(), events.begin() + numEvents, [] (const Event& a, const Event& b) { return a.at < b.at; });
        int e = 0, pos = 0;
        while (pos < n)
        {
            while (e < numEvents && events[(size_t) e].at <= pos) apply (events[(size_t) e++], semis, follow);
            const int end = e < numEvents ? juce::jlimit (pos + 1, n, events[(size_t) e].at) : n;
            renderVoices (buffer, pos, end - pos, target);
            pos = end;
        }
        // events pushed past this block by the latency wait for the next one
        int kept = 0;
        for (; e < numEvents; ++e) { auto ev = events[(size_t) e]; ev.at = juce::jmax (0, ev.at - due); events[(size_t) kept++] = ev; }
        numEvents = kept;
    }

private:
    struct Sample { juce::AudioBuffer<float> data; double rate = 44100.0; juce::String name; };
    struct Event { int at, note; float vel; bool on; float st = 0.0f; int fit = 0; };
    struct Voice
    {
        bool active = false, releasing = false;
        int note = 60, wait = 0;
        double pos = 0.0, inc = 1.0;
        float vel = 1.0f, env = 1.0f, envStep = 0.0f;
    };

    std::unique_ptr<Sample> readAll (juce::AudioFormatReader& r, const juce::String& name) const
    {
        const auto len = (int) juce::jmin<juce::int64> (r.lengthInSamples, (juce::int64) (maxSeconds * r.sampleRate));
        const int ch = r.numChannels > 1 ? 2 : 1;
        auto s = std::make_unique<Sample>();
        s->data.setSize (ch, len);
        s->data.clear();
        r.read (&s->data, 0, len, 0, true, ch > 1);
        s->rate = r.sampleRate;
        s->name = name;
        return s;
    }

    void store (const Sample& s)
    {
        if (apvts == nullptr) return;
        // FLAC is 24-bit integer: a float file louder than 0 dBFS is scaled down for storage and back up on load
        const float peak = s.data.getMagnitude (0, s.data.getNumSamples());
        const float down = peak > 1.0f ? 1.0f / peak : 1.0f;
        juce::AudioBuffer<float> scaled;
        scaled.makeCopyOf (s.data);
        scaled.applyGain (down);
        juce::MemoryBlock mb;
        {
            auto* os = new juce::MemoryOutputStream (mb, false);
            juce::FlacAudioFormat flac;
            const double rate = juce::jlimit (8000.0, 192000.0, s.rate);
            std::unique_ptr<juce::AudioFormatWriter> w (flac.createWriterFor (os, rate, (unsigned int) s.data.getNumChannels(), 24, {}, 0));
            if (w == nullptr) { delete os; return; }
            w->writeFromAudioSampleBuffer (scaled, 0, scaled.getNumSamples());
        }   // the writer flushes and deletes the stream here
        apvts->state.setProperty ("ownSound", mb.toBase64Encoding(), nullptr);
        apvts->state.setProperty ("ownSoundName", s.name, nullptr);
        if (down < 1.0f) apvts->state.setProperty ("ownSoundGain", 1.0 / down, nullptr);
        else apvts->state.removeProperty ("ownSoundGain", nullptr);
    }

    void install (std::unique_ptr<Sample> s)
    {
        seconds_.store (s != nullptr ? s->data.getNumSamples() / s->rate : 0.0);
        {
            const juce::SpinLock::ScopedLockType sl (lock);
            std::swap (current, s);
            resetVoices = true;
            loaded.store (current != nullptr);
        }
        // the old sample (now in `s`) is freed here, on the message thread
    }

    void push (Event ev) noexcept { ev.at += juce::jmax (0, latency); if (numEvents < (int) events.size()) events[(size_t) numEvents++] = ev; }

    void apply (const Event& ev, float semis, bool follow) noexcept
    {
        const float fast = 1.0f / (float) (0.004 * hostRate), slow = 1.0f / (float) (0.025 * hostRate);
        if (! ev.on)
        {
            if (ev.note < 0) { for (auto& v : voices) if (v.active) { v.releasing = true; v.envStep = fast; } return; }
            if (gate) for (auto& v : voices) if (v.active && ! v.releasing && v.note == ev.note) { v.releasing = true; v.envStep = slow; }
            return;
        }
        if (cfg.mono) for (auto& v : voices) if (v.active && ! v.releasing) { v.releasing = true; v.envStep = fast; }
        Voice* slot = nullptr;
        for (auto& v : voices) if (! v.active) { slot = &v; break; }
        if (slot == nullptr)   // all busy: take the one furthest into its sound
        {
            slot = &voices[0];
            for (auto& v : voices) if (v.pos > slot->pos) slot = &v;
        }
        const double st = (double) semis + (double) ev.st + (follow ? (double) (ev.note - cfg.rootNote) : 0.0);
        *slot = {};
        slot->active = true;
        slot->note = ev.note;
        slot->inc = current->rate / hostRate * std::pow (2.0, st / 12.0);
        if (cfg.reverse)
        {
            const double last = (double) (current->data.getNumSamples() - 1), total = last / slot->inc;   // output samples
            slot->pos = last;
            if (ev.fit > 0 && total > (double) ev.fit) slot->pos = (double) ev.fit * slot->inc;
            else if (ev.fit > 0) slot->wait = (int) ((double) ev.fit - total);
        }
        slot->vel = juce::jlimit (0.0f, 1.0f, 0.3f + 0.7f * ev.vel / 0.787f);   // FL's default velocity (100) = full level
        hits.fetch_add (1, std::memory_order_relaxed);
    }

    void renderVoices (juce::AudioBuffer<float>& buffer, int start, int num, float target) noexcept
    {
        const auto& d = current->data;
        const int len = d.getNumSamples(), srcCh = d.getNumChannels(), outCh = buffer.getNumChannels();
        const float* sl = d.getReadPointer (0);
        const float* sr = d.getReadPointer (srcCh > 1 ? 1 : 0);
        float* ol = buffer.getWritePointer (0, start);
        float* orr = outCh > 1 ? buffer.getWritePointer (1, start) : nullptr;
        const float g0 = gain, step = (target - g0) / (float) juce::jmax (1, num);
        gain = target;

        auto at = [len] (const float* p, int i) { return p[juce::jlimit (0, len - 1, i)]; };
        auto hermite = [] (float xm1, float x0, float x1, float x2, float t)
        {
            const float c1 = 0.5f * (x1 - xm1), c2 = xm1 - 2.5f * x0 + 2.0f * x1 - 0.5f * x2, c3 = 0.5f * (x2 - xm1) + 1.5f * (x0 - x1);
            return ((c3 * t + c2) * t + c1) * t + x0;
        };

        for (auto& v : voices)
        {
            if (! v.active) continue;
            for (int i = 0; i < num; ++i)
            {
                if (v.wait > 0) { --v.wait; continue; }
                const int k = (int) v.pos;
                if (k >= len - 1 || v.pos < 0.0) { v.active = false; break; }
                const float t = (float) (v.pos - k);
                const float l = hermite (at (sl, k - 1), sl[k], sl[k + 1], at (sl, k + 2), t);
                const float r = srcCh > 1 ? hermite (at (sr, k - 1), sr[k], sr[k + 1], at (sr, k + 2), t) : l;
                if (v.releasing) { v.env -= v.envStep; if (v.env <= 0.0f) { v.active = false; break; } }
                const float g = (g0 + step * (float) i) * v.vel * v.env;
                if (orr != nullptr) { ol[i] += l * g; orr[i] += r * g; }
                else ol[i] += 0.5f * (l + r) * g;
                v.pos += cfg.reverse ? -v.inc : v.inc;
            }
        }
    }

    Config cfg;
    juce::AudioProcessorValueTreeState* apvts = nullptr;
    std::atomic<float>* level = nullptr;
    std::atomic<float>* tune = nullptr;
    std::atomic<float>* key = nullptr;
    std::atomic<float>* mode = nullptr;
    double hostRate = 44100.0;

    mutable juce::SpinLock lock;
    std::unique_ptr<Sample> current;   // guarded by `lock`
    std::atomic<bool> loaded { false };
    std::atomic<double> seconds_ { 0.0 };
    bool resetVoices = false;
    std::array<Voice, 16> voices {};
    std::array<Event, 256> events {};
    int numEvents = 0;
    float gain = 1.0f;

    JUCE_DECLARE_NON_COPYABLE (OwnSound)
};
}
