// Offline render + GUI screenshot test tool.
// Usage: VoidbringerRender <demo.wav> [gui.png]
// Plays an 8-bar uptempo screech phrase at 200 BPM through the default preset
// ("Rawstyle Terror Screech"); bars 5-8 get a simple reference kick mixed in (outside the plugin).
// Checks: no NaN/Inf, plugin output peak <= 0 dBFS (also with extreme settings), state save/restore.
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "Presets.h"
#include "PluginEditor.h"
#include "OwnSoundTest.h"

using namespace juce;

struct FixedPlayHead : AudioPlayHead
{
    double bpm = 200.0;
    Optional<PositionInfo> getPosition() const override { PositionInfo p; p.setBpm (bpm); p.setIsPlaying (true); return p; }
};

static void resetDefaults (VoidbringerProcessor& p) { for (auto* prm : p.getParameters()) prm->setValueNotifyingHost (prm->getDefaultValue()); }
static void setParam (VoidbringerProcessor& p, const char* id, float v) { auto* prm = p.apvts.getParameter (id); prm->setValueNotifyingHost (prm->convertTo0to1 (v)); }

static bool writeWav (const AudioBuffer<float>& b, double sr, const String& path)
{
    File f = File::getCurrentWorkingDirectory().getChildFile (path);
    f.deleteFile();
    WavAudioFormat wav;
    std::unique_ptr<AudioFormatWriter> w (wav.createWriterFor (new FileOutputStream (f), sr, (unsigned) b.getNumChannels(), 24, {}, 0));
    if (w == nullptr) return false;
    w->writeFromAudioSampleBuffer (b, 0, b.getNumSamples());
    std::printf ("WAV written: %s\n", f.getFullPathName().toRawUTF8());
    return true;
}

struct Note { int step, len, note; };   // 1/16 steps; overlap with the next note = legato slide

static void renderSeq (VoidbringerProcessor& proc, const MidiBuffer& seq, AudioBuffer<float>& out, int block)
{
    AudioBuffer<float> buf (2, block);
    const int total = out.getNumSamples();
    for (int pos = 0; pos < total; pos += block)
    {
        const int n = jmin (block, total - pos);
        AudioBuffer<float> view (buf.getArrayOfWritePointers(), 2, n);
        MidiBuffer m;
        m.addEvents (seq, pos, n, -pos);
        proc.processBlock (view, m);
        for (int ch = 0; ch < 2; ++ch) out.copyFrom (ch, pos, view, ch, 0, n);
    }
}

static void analyse (const AudioBuffer<float>& b, const char* what, int& nan, double& peak)
{
    nan = 0; peak = 0;
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            const float v = b.getSample (ch, i);
            if (! std::isfinite (v)) ++nan; else peak = jmax (peak, (double) std::abs (v));
        }
    std::printf ("%s: peak %.4f (%.2f dBFS), RMS L %.1f dBFS, NaN/Inf %d\n", what, peak, Decibels::gainToDecibels (peak, -100.0),
                 Decibels::gainToDecibels (b.getRMSLevel (0, 0, b.getNumSamples()), -100.0f), nan);
}

int main (int argc, char** argv)
{
    ScopedJuceInitialiser_GUI init;
    const String wavPath = argc > 1 ? argv[1] : "demo.wav";
    const String pngPath = argc > 2 ? argv[2] : "";
    const double sr = 44100.0, bpm = 200.0, step = 60.0 / bpm / 4.0;
    const int block = 256;

    const std::vector<Note> riff = {   // 4 bars, F minor
        { 0, 2, 65 }, { 2, 2, 65 }, { 4, 1, 63 }, { 6, 2, 65 }, { 8, 4, 68 }, { 11, 3, 67 }, { 14, 2, 63 },
        { 16, 2, 65 }, { 18, 2, 65 }, { 20, 2, 70 }, { 22, 2, 68 }, { 24, 3, 67 }, { 26, 4, 72 }, { 30, 2, 70 },
        { 32, 2, 65 }, { 34, 2, 65 }, { 36, 1, 63 }, { 38, 2, 65 }, { 40, 4, 68 }, { 43, 3, 72 }, { 46, 2, 75 },
        { 48, 6, 77 }, { 53, 3, 75 }, { 56, 4, 72 }, { 59, 5, 68 }, { 63, 1, 65 } };
    const int bars = 8;
    const int total = (int) ((bars * 16 * step + 0.6) * sr);
    MidiBuffer seq;
    for (int rep = 0; rep < 2; ++rep)
        for (auto& n : riff)
        {
            seq.addEvent (MidiMessage::noteOn (1, n.note, (uint8) 110), (int) ((rep * 64 + n.step) * step * sr));
            seq.addEvent (MidiMessage::noteOff (1, n.note), (int) ((rep * 64 + n.step + n.len) * step * sr) - (int) (0.004 * sr));
        }

    FixedPlayHead ph; ph.bpm = bpm;
    VoidbringerProcessor proc;
    proc.setPlayHead (&ph);
    proc.setPlayConfigDetails (0, 2, sr, block);
    proc.prepareToPlay (sr, block);
    resetDefaults (proc);
    AudioBuffer<float> out (2, total);
    const auto t0 = Time::getMillisecondCounterHiRes();
    renderSeq (proc, seq, out, block);
    const double ms = Time::getMillisecondCounterHiRes() - t0;
    int nan; double peak;
    analyse (out, "Plugin output (default preset)", nan, peak);
    std::printf ("Render %.2f s audio in %.0f ms (%.1fx realtime)\n", total / sr, ms, total / sr * 1000.0 / ms);
    const bool audible = peak > 0.05;

    // stress test: extreme settings
    VoidbringerProcessor st;
    st.setPlayHead (&ph);
    st.setPlayConfigDetails (0, 2, sr, block);
    st.prepareToPlay (sr, block);
    setParam (st, PID::fmAmount, 100); setParam (st, PID::feedback, 100); setParam (st, PID::fmEnvAmount, 100);
    setParam (st, PID::modRatio, 16); setParam (st, PID::carRatio, 8); setParam (st, PID::resonance, 100);
    setParam (st, PID::drive, 48); setParam (st, PID::outGain, 6); setParam (st, PID::pitchEnvAmount, 48);
    setParam (st, PID::lfoDepth, 100); setParam (st, PID::lfoDest, 0); setParam (st, PID::lfoWave, 4);
    setParam (st, PID::cutoff, 18000); setParam (st, PID::filterType, 0); setParam (st, PID::ott, 100); setParam (st, PID::octave, 2);
    AudioBuffer<float> so (2, (int) (4 * sr));
    renderSeq (st, seq, so, block);
    int nan2; double peak2;
    analyse (so, "Stress test (extremes, soft clip on)", nan2, peak2);

    // demo mix: bars 5-8 with a reference kick under the screech
    AudioBuffer<float> mix (out);
    for (int beat = 16; beat < bars * 4; ++beat)
    {
        const int start = (int) (beat * 4 * step * sr);
        double phs = 0.0;
        for (int i = 0; i < (int) (4 * step * sr * 0.95) && start + i < total; ++i)
        {
            const double t = i / sr;
            phs += MathConstants<double>::twoPi * (55.0 + 300.0 * std::exp (-t * 40.0)) / sr;
            const float v = (float) (0.55 * std::tanh (2.5 * std::sin (phs) * std::exp (-t * 5.0) * std::min (1.0, t * 2000.0)));
            const float duck = (float) (1.0 - 0.7 * std::exp (-t * 12.0));
            for (int ch = 0; ch < 2; ++ch) mix.setSample (ch, start + i, out.getSample (ch, start + i) * duck * 0.8f + v);
        }
    }
    const float mixPeak = mix.getMagnitude (0, total);
    if (mixPeak > 0.97f) mix.applyGain (0.97f / mixPeak);
    std::printf ("Demo mix peak before safety trim %.3f\n", mixPeak);
    if (! writeWav (mix, sr, wavPath)) return 2;

    // state save / restore
    setParam (proc, PID::fmAmount, 77.0f); setParam (proc, PID::filterType, 2.0f);
    MemoryBlock state; proc.getStateInformation (state);
    VoidbringerProcessor proc2; proc2.setStateInformation (state.getData(), (int) state.getSize());
    bool stateOk = true;
    for (auto* prm : proc.getParameters())
        if (auto* a = dynamic_cast<AudioProcessorParameterWithID*> (prm))
            if (std::abs (a->getValue() - proc2.apvts.getParameter (a->paramID)->getValue()) > 1e-5f) stateOk = false;
    std::printf ("State save/restore: %s (%d params)\n", stateOk ? "OK" : "FAILED", proc.getParameters().size());

    resetDefaults (proc);
    for (auto* prm : proc.getParameters())
        if (auto* a = dynamic_cast<RangedAudioParameter*> (prm))
            std::printf ("PARAM|%s|%s|%s|%s|%s\n", a->paramID.toRawUTF8(), a->getName (100).toRawUTF8(),
                         a->getText (0.0f, 64).toRawUTF8(), a->getText (1.0f, 64).toRawUTF8(), a->getText (a->getDefaultValue(), 64).toRawUTF8());

    // ---- factory presets (Presets.h): every ID must exist; each preset gets 2 s of a held note at 150 BPM
    //      (playing transport) and must stay finite, audible and <= 0 dBFS
    bool presetsOk = true;
    {
        struct PresetHead : AudioPlayHead
        {
            double ppq = 0.0;
            Optional<PositionInfo> getPosition() const override
            {
                PositionInfo p; p.setBpm (150.0); p.setPpqPosition (ppq); p.setIsPlaying (true);
                p.setTimeSignature (AudioPlayHead::TimeSignature { 4, 4 });
                return p;
            }
        };
        const double psr = 48000.0;
        const int pblock = 512, pblocks = (int) (2.0 * psr / pblock);
        const auto& presets = factoryPresets();
        for (int pi = 0; pi < (int) presets.size(); ++pi)
        {
            VoidbringerProcessor pp;
            PresetHead head;
            pp.setPlayHead (&head);
            pp.setPlayConfigDetails (0, 2, psr, pblock);
            pp.prepareToPlay (psr, pblock);
            int missing = 0;
            for (const auto& [pid, v] : presets[(size_t) pi].values)
                if (pp.apvts.getParameter (pid) == nullptr) { ++missing; std::printf ("  unknown parameter ID %s\n", pid); }
            applyFactoryPreset (pp.apvts, pi);
            AudioBuffer<float> pb (2, pblock);
            float pk = 0.0f; double sum = 0.0; int bad = 0, cnt = 0;
            for (int b = 0; b < pblocks; ++b)
            {
                MidiBuffer m;
                if (b == 0) m.addEvent (MidiMessage::noteOn (1, 53, 0.9f), 0);                 // F3, held 1.5 s
                if (b == (int) (1.5 * psr / pblock)) m.addEvent (MidiMessage::noteOff (1, 53), 0);
                pb.clear();
                head.ppq = b * pblock / psr * 2.5;   // 150 BPM = 2.5 beats per second
                pp.processBlock (pb, m);
                for (int ch = 0; ch < 2; ++ch)
                    for (int i = 0; i < pblock; ++i)
                    {
                        const float v = pb.getSample (ch, i);
                        if (! std::isfinite (v)) { ++bad; continue; }
                        pk = jmax (pk, std::abs (v)); sum += (double) v * v; ++cnt;
                    }
            }
            pp.setPlayHead (nullptr);
            const double prms = std::sqrt (sum / jmax (1, cnt));
            const bool good = missing == 0 && bad == 0 && pk <= 1.0f && prms > 0.005;
            presetsOk = presetsOk && good;
            std::printf ("Preset %-26s peak %.3f rms %.3f %s\n", presets[(size_t) pi].name, pk, prms, good ? "OK" : "FAILED");
        }
    }

    // ---- user presets (UserPresets.h) + resizable window: save -> change a knob -> load restores it (window size
    //      kept); a stored 75 % opens the editor at 75 % of its 100 % size
    {
        VoidbringerProcessor up;
        RangedAudioParameter* prm = nullptr;
        for (auto* q : up.getParameters()) if (auto* fp = dynamic_cast<AudioParameterFloat*> (q)) { prm = fp; break; }
        bool upOk = prm != nullptr;
        if (prm != nullptr)
        {
            prm->setValueNotifyingHost (0.123f);
            const float v0 = prm->getValue();
            const auto file = File::getSpecialLocation (File::tempDirectory).getChildFile ("voidbringer_user_preset_test.vbpreset");
            upOk = Vb::UserPresets::saveTo (up, file);
            prm->setValueNotifyingHost (0.9f);
            up.apvts.state.setProperty ("uiScale", 1.2, nullptr);
            upOk = upOk && Vb::UserPresets::loadFrom (up, file, [&up] { return up.apvts.state; });
            upOk = upOk && std::abs (prm->getValue() - v0) < 1.0e-4f
                        && std::abs ((double) up.apvts.state.getProperty ("uiScale") - 1.2) < 1.0e-6;
            file.deleteFile();
        }
        up.apvts.state.setProperty ("uiScale", 1.0, nullptr);
        int w100 = 0, w75 = 0;
        { std::unique_ptr<AudioProcessorEditor> e (up.createEditor()); w100 = e->getWidth(); }
        up.apvts.state.setProperty ("uiScale", 0.75, nullptr);
        { std::unique_ptr<AudioProcessorEditor> e (up.createEditor()); w75 = e->getWidth(); upOk = upOk && e->isResizable(); }
        const bool sizeOk = w100 > 0 && std::abs (w75 - 0.75 * w100) <= 2.0;
        std::printf ("User preset save/load: %s, window 100 %% = %d px, 75 %% = %d px %s\n", upOk ? "OK" : "FAILED", w100, w75,
                     sizeOk ? "OK" : "FAILED");
        if (! upOk || ! sizeOk) { std::printf ("RESULT: FAIL\n"); return 1; }
    }

    if (! Sonic::ownSoundSelfTest<VoidbringerProcessor> (sr, block, 60, 1.2f)) { std::printf ("RESULT: FAIL\n"); return 1; }

    if (pngPath.isNotEmpty())
    {
        std::unique_ptr<AudioProcessorEditor> ed (proc.createEditor());
        if (auto* te = dynamic_cast<VoidbringerEditor*> (ed.get()))
        {
            int at = (int) (9.0 * step * sr);
            for (int s = 0; s < 400; ++s, ++at) if (out.getSample (0, at) <= 0 && out.getSample (0, at + 1) > 0) break;
            std::vector<float> w (512);
            for (int i = 0; i < 512; ++i) w[(size_t) i] = 0.5f * (out.getSample (0, at + i) + out.getSample (1, at + i));
            te->prepareScreenshot (w.data(), 512);
        }
        auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
        File pf = File::getCurrentWorkingDirectory().getChildFile (pngPath);
        pf.deleteFile();
        FileOutputStream os (pf);
        PNGImageFormat png;
        if (! png.writeImageToStream (img, os)) return 3;
        std::printf ("GUI screenshot: %s (%dx%d)\n", pf.getFullPathName().toRawUTF8(), img.getWidth(), img.getHeight());
    }

    // ---- regression: sustain pedal holds a released note until the pedal goes up
    bool sustainOk = true;
    {
        VoidbringerProcessor sp;
        sp.setPlayConfigDetails (0, 2, 48000.0, 512);
        sp.prepareToPlay (48000.0, 512);
        setParam (sp, PID::ampRelease, 20.0f);
        AudioBuffer<float> sb (2, 512);
        auto blockPeak = [&] (MidiBuffer& m) { sb.clear(); sp.processBlock (sb, m); return sb.getMagnitude (0, 512); };
        MidiBuffer m;
        m.addEvent (MidiMessage::controllerEvent (1, 64, 127), 0);
        m.addEvent (MidiMessage::noteOn (1, 60, 0.9f), 0);
        m.addEvent (MidiMessage::noteOff (1, 60), 400);
        blockPeak (m); m.clear();
        float held = 0.0f;
        for (int b = 0; b < 20; ++b) held = blockPeak (m);            // ~0.2 s after the note-off, pedal down
        m.addEvent (MidiMessage::controllerEvent (1, 64, 0), 0);
        blockPeak (m); m.clear();
        float after = 0.0f;
        for (int b = 0; b < 20; ++b) after = blockPeak (m);           // ~0.2 s after pedal up (20 ms release)
        sustainOk = held > 0.01f && after < 1.0e-3f;
        std::printf ("Sustain pedal: level while held %.4f, after pedal up %.6f  %s\n", held, after, sustainOk ? "OK" : "FAILED");
    }

    // ---- regression: a block bigger than the announced maximum is processed in chunks with its MIDI
    bool bigOk = true;
    {
        VoidbringerProcessor bp;
        bp.setPlayConfigDetails (0, 2, 48000.0, 256);
        bp.prepareToPlay (48000.0, 256);
        AudioBuffer<float> bb (2, 2000);
        bb.clear();
        MidiBuffer m;
        m.addEvent (MidiMessage::noteOn (1, 60, 0.9f), 1500);   // lands in the 6th chunk
        bp.processBlock (bb, m);
        bigOk = bb.getMagnitude (0, 0, 1500) < 1.0e-6f && bb.getMagnitude (0, 1500, 500) > 0.001f;
        std::printf ("Oversized block (2000 > 256): note at 1500 sounds from 1500 on  %s\n", bigOk ? "OK" : "FAILED");
    }


    // ---- Polyphony = On: a C-E-G chord gives 3 distinct pitches (pure-sine settings); Off keeps the mono screech
    //      (last note only); the default screech with a 4-note chord stays finite and <= 0 dBFS
    bool polyOk = true;
    {
        auto chord = [] (int poly, bool pure, std::initializer_list<int> notes)
        {
            VoidbringerProcessor cp;
            cp.setPlayConfigDetails (0, 2, 48000.0, 512);
            cp.prepareToPlay (48000.0, 512);
            setParam (cp, PID::polyphony, (float) poly);
            if (pure)
            {
                setParam (cp, PID::fmAmount, 0.0f); setParam (cp, PID::fmEnvAmount, 0.0f); setParam (cp, PID::feedback, 0.0f);
                setParam (cp, PID::pitchEnvAmount, 0.0f); setParam (cp, PID::lfoDepth, 0.0f); setParam (cp, PID::stereoDetune, 0.0f);
                setParam (cp, PID::carRatio, 1.0f); setParam (cp, PID::filterType, 0.0f); setParam (cp, PID::cutoff, 18000.0f);
                setParam (cp, PID::resonance, 0.0f); setParam (cp, PID::filterEnvAmount, 0.0f);
                setParam (cp, PID::distMix, 0.0f); setParam (cp, PID::ott, 0.0f); setParam (cp, PID::softClip, 0.0f);
                setParam (cp, PID::ampAttack, 1.0f);
            }
            MidiBuffer chordSeq;
            for (int nn : notes) chordSeq.addEvent (MidiMessage::noteOn (1, nn, 0.9f), 0);
            AudioBuffer<float> chordOut (2, 48000);
            renderSeq (cp, chordSeq, chordOut, 512);
            return chordOut;
        };
        auto mag = [] (const AudioBuffer<float>& b, double hz)
        {
            const int a = 12000, len = 24000;
            const double w = MathConstants<double>::twoPi * hz / 48000.0;
            double re = 0, im = 0;
            for (int i = 0; i < len; ++i)
            {
                const double x = b.getSample (0, a + i) * (0.5 - 0.5 * std::cos (MathConstants<double>::twoPi * i / len));
                re += x * std::cos (w * i); im -= x * std::sin (w * i);
            }
            return std::sqrt (re * re + im * im) / len;
        };
        const double hz[3] = { 261.63, 329.63, 392.00 };
        auto p = chord (1, true, { 60, 64, 67 });
        auto m = chord (0, true, { 60, 64, 67 });
        int present = 0;
        for (double f : hz) { const double v = mag (p, f), off = mag (p, f * 1.06); present += (v > 0.01 && v > 5.0 * off) ? 1 : 0;
                              std::printf ("Poly chord %.1f Hz: %.4f (off %.4f)\n", f, v, off); }
        const bool monoLast = mag (m, hz[2]) > 0.01 && mag (m, hz[0]) < mag (m, hz[2]) * 0.05 && mag (m, hz[1]) < mag (m, hz[2]) * 0.05;
        int nanC = 0; double peakC = 0;
        auto d = chord (1, false, { 60, 63, 67, 72 });
        analyse (d, "Poly chord, default screech (4 notes)", nanC, peakC);
        polyOk = present == 3 && monoLast && nanC == 0 && peakC <= 1.0;
        std::printf ("Polyphony: %d/3 chord pitches, mono = last note only %s  %s\n", present, monoLast ? "yes" : "NO", polyOk ? "OK" : "FAILED");
    }

    const bool ok = nan == 0 && nan2 == 0 && peak <= 1.0 && peak2 <= 1.0 && stateOk && audible && sustainOk && bigOk && polyOk;
    const bool allOk = ok && presetsOk;
    std::printf ("RESULT: %s\n", allOk ? "PASS" : "FAIL");
    return allOk ? 0 : 1;
}
