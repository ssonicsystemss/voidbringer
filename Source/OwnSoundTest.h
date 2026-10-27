#pragma once
// Render-test check for OWN SOUND (used by every instrument's Tools/RenderDemo.cpp):
// a 0.25 s test tone is put in as the own sample, one note plays it in REPLACE mode -> clean, audible output;
// the state round trip keeps the sample; removing it brings the plugin's own sound back.
#include <cstdio>
#include "OwnSound.h"

namespace Sonic
{
template <class Proc>
bool ownSoundSelfTest (double sr, int block, int note = 60, float maxPeak = 1.2f)
{
    using namespace juce;
    auto render = [&] (Proc& p, float seconds)
    {
        const int len = (int) (seconds * sr);
        AudioBuffer<float> b (jmax (2, p.getTotalNumOutputChannels()), block);
        float peak = 0.0f;
        int bad = 0;
        for (int pos = 0; pos < len; pos += block)
        {
            const int n = jmin (block, len - pos);
            MidiBuffer m;
            if (pos == 0) m.addEvent (MidiMessage::noteOn (1, note, 0.8f), 0);
            if (pos + n >= len) m.addEvent (MidiMessage::noteOff (1, note), n - 1);
            b.setSize (b.getNumChannels(), n, false, false, true);
            b.clear();
            p.processBlock (b, m);
            for (int ch = 0; ch < b.getNumChannels(); ++ch)
                for (int i = 0; i < n; ++i)
                {
                    const float v = b.getSample (ch, i);
                    if (! std::isfinite (v)) ++bad; else peak = jmax (peak, std::abs (v));
                }
        }
        return bad > 0 ? -1.0f : peak;
    };

    Proc p;
    p.setPlayConfigDetails (p.getTotalNumInputChannels(), p.getTotalNumOutputChannels(), sr, block);
    p.prepareToPlay (sr, block);
    AudioBuffer<float> tone (1, (int) (0.25 * sr));
    for (int i = 0; i < tone.getNumSamples(); ++i)
        tone.setSample (0, i, 0.5f * (float) std::sin (MathConstants<double>::twoPi * 220.0 * i / sr));
    p.own.setSound (tone, sr, "selftest");
    if (auto* m = p.apvts.getParameter (OwnID::mode)) m->setValueNotifyingHost (1.0f);   // REPLACE
    const float replaced = render (p, 0.6f);

    MemoryBlock st;
    p.getStateInformation (st);
    Proc q;
    q.setStateInformation (st.getData(), (int) st.getSize());
    const bool kept = q.own.hasSound() && q.own.name() == "selftest" && std::abs (q.own.seconds() - 0.25) < 0.01;

    p.own.clear();
    if (auto* m = p.apvts.getParameter (OwnID::mode)) m->setValueNotifyingHost (0.0f);
    p.prepareToPlay (sr, block);
    const float own = render (p, 0.6f);

    const bool ok = replaced > 0.2f && replaced < maxPeak && kept && own >= 0.0f && ! p.own.hasSound();
    std::printf ("Own sound: replace peak %.3f, state %s (%d KB), removed -> plugin peak %.3f  %s\n", replaced,
                 kept ? "kept" : "LOST", (int) (st.getSize() / 1024), own, ok ? "OK" : "FAILED");
    return ok;
}
}
