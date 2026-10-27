#include "PluginEditor.h"
#include "Presets.h"

using namespace juce;
using namespace Vb;

// ======================================================================= the gravity well
WellStage::WellStage (Session& s, VoidbringerProcessor& p)
    : Stage (s, "gravity well", {}),
      proc (p), stars (makeStars (180, 23)), disk (makeStars (260, 29))
{
    for (auto& d : disk) { d.x *= MathConstants<float>::twoPi; d.y = 0.17f + d.y * 0.3f; }   // angle, radius
}

void WellStage::mouseDown (const MouseEvent& e)
{
    begin ({ PID::fmAmount, PID::feedback });
    moveTo (e.position);
}

void WellStage::mouseDrag (const MouseEvent& e) { moveTo (e.position); }

void WellStage::moveTo (Point<float> p)
{
    const float S = (float) jmin (getWidth(), getHeight());
    const float dx = (p.x - (float) getWidth() * 0.5f) / S, dy = (p.y - (float) getHeight() * 0.58f) / S;
    set (PID::fmAmount, 1.0f - (std::hypot (dx, dy) - 0.08f) / 0.36f);
    set (PID::feedback, (std::atan2 (dy, dx) + MathConstants<float>::pi) / MathConstants<float>::twoPi);
}

StringArray WellStage::readout()
{
    const auto ratio = [this] (const char* id) { auto* q = p (id); return q != nullptr ? String (q->convertFrom0to1 (q->getValue()), 2) : String(); };
    return { row ("FM", text (PID::fmAmount)), row ("FEEDBACK", text (PID::feedback)),
             row ("RATIO", ratio (PID::carRatio) + " : " + ratio (PID::modRatio)),
             row ("FILTER", text (PID::cutoff)) };
}

void WellStage::drawStage (Graphics& g, Rectangle<float> a, double t)
{
    const float w = a.getWidth(), h = a.getHeight(), S = jmin (w, h), cx = w * 0.5f, cy = h * 0.58f;
    drawStars (g, a, stars, 0.7f);

    // the live waveform across the top (zero-crossing trigger)
    {
        constexpr int N = VoidbringerProcessor::scopeSize, show = 512;
        std::vector<float> d ((size_t) N);
        if (! staticWave.empty())
            for (int i = 0; i < N; ++i) d[(size_t) i] = staticWave[(size_t) (i % (int) staticWave.size())];
        else
        {
            const int wpos = proc.scopeWrite.load();
            for (int i = 0; i < N; ++i) d[(size_t) i] = proc.scope[(size_t) ((wpos + i) % N)].load (std::memory_order_relaxed);
        }
        int start = 0;
        for (int i = N - show - 2; i > 1; --i) if (d[(size_t) i - 1] < 0.0f && d[(size_t) i] >= 0.0f) { start = i; break; }
        if (! staticWave.empty()) start = 0;
        const float top = h * 0.13f, hgt = h * 0.09f;
        Path p;
        for (int i = 0; i < show; ++i)
        {
            const float x = w * 0.04f + w * 0.92f * (float) i / (float) (show - 1);
            const float y = top + hgt * 0.5f - jlimit (-1.0f, 1.0f, d[(size_t) (start + i)]) * hgt * 0.5f;
            if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
        }
        g.setColour (Colours::white.withAlpha (0.35f));
        g.strokePath (p, PathStrokeType (1.0f));
    }

    const float fm = get (PID::fmAmount), fb = get (PID::feedback), sp = 0.00025f * (0.5f + get (PID::modRatio) * 2.5f);
    // colour follows the sound: a clean patch glows white, more FM + feedback + drive turn the hole red
    const float heatAll = jlimit (0.0f, 1.0f, 0.45f * fm + 0.3f * fb + 0.25f * get (PID::drive) / 0.8f);
    const auto tint = [] (float k) { return Colours::white.interpolatedWith (Col::accent, jlimit (0.0f, 1.0f, k)); };
    auto drawDisk = [&] (bool front)
    {
        for (auto& q : disk)
        {
            const float an = q.x + (float) (t * sp / q.y), y = std::sin (an);
            if ((y > 0.0f) != front) continue;
            const float heat = 1.0f - (q.y - 0.17f) / 0.3f;
            const bool hot = q.b < 0.3f;
            g.setColour (tint (heatAll * (0.35f + 0.9f * heat) * (hot ? 1.15f : 0.85f)).withAlpha (hot ? 0.3f + 0.6f * heat : 0.15f + 0.6f * heat));
            g.fillRect (cx + std::cos (an) * q.y * S * 1.05f, cy + y * q.y * S * 0.26f, 1.0f + heat, 1.2f);
        }
    };
    drawDisk (false);
    g.setColour (Col::bg);
    g.fillEllipse (cx - 0.1f * S, cy - 0.1f * S, 0.2f * S, 0.2f * S);
    g.setColour (tint (heatAll * 0.8f));
    g.drawEllipse (cx - 0.115f * S, cy - 0.115f * S, 0.23f * S, 0.23f * S, 1.3f);
    {
        Path lens;
        lens.addCentredArc (cx, cy - 0.01f * S, 0.135f * S, 0.1f * S, 0.0f, -MathConstants<float>::halfPi * 0.9f, MathConstants<float>::halfPi * 0.9f, true);
        g.setColour (Colours::white.withAlpha (0.45f));
        g.strokePath (lens, PathStrokeType (1.0f));
    }
    drawDisk (true);
    {
        Path outer, d;
        outer.addEllipse (cx - 0.44f * S, cy - 0.44f * S, 0.88f * S, 0.88f * S);
        const float dash[] = { 1.0f, 4.0f };
        PathStrokeType (1.0f).createDashedStroke (d, outer, dash, 2);
        g.setColour (Col::line2);
        g.fillPath (d);
    }
    const float rr = (0.08f + 0.36f * (1.0f - fm)) * S, an = fb * MathConstants<float>::twoPi - MathConstants<float>::pi;
    const Point<float> pr (cx + std::cos (an) * rr, cy + std::sin (an) * rr);
    {
        Path tether, d;
        tether.startNewSubPath (cx, cy); tether.lineTo (pr);
        const float dash[] = { 3.0f, 5.0f };
        PathStrokeType (1.0f).createDashedStroke (d, tether, dash, 2);
        g.setColour (Col::dim);
        g.fillPath (d);
    }
    const int n = roundToInt (4.0f + fm * 30.0f);
    const float fl = (float) std::fmod (t / 700.0, 1.0);
    for (int i = 0; i < n; ++i)   // the stream pulled into the hole
    {
        const float k = (float) std::fmod ((float) i / (float) n + fl, 1.0f), r = rr * (1.0f - k) + 0.1f * S * k, a2 = an + k * (1.5f + fb * 4.0f);
        g.setColour (tint (heatAll * (0.6f + 0.4f * k)).withAlpha ((1.0f - k) * 0.9f));
        g.fillRect (Rectangle<float> (2.0f, 2.0f).withCentre ({ cx + std::cos (a2) * r, cy + std::sin (a2) * r }));
    }
    const float z = 5.0f * (1.0f + 0.2f * (float) std::sin (t / 180.0));
    g.setColour (tint (heatAll));
    g.fillRect (Rectangle<float> (2 * z, 2 * z).withCentre (pr));
    g.setColour (Col::ink);
    g.drawRect (Rectangle<float> (2 * z, 2 * z).withCentre (pr), 1.2f);
    g.setColour (Col::mute);
    g.setFont (mono (8.5f));
    g.drawText ("NO FM", Rectangle<float> (60.0f, 12.0f).withCentre ({ cx, cy - 0.44f * S - 8.0f }), Justification::centred, false);
}

// ======================================================================= editor
VoidbringerEditor::VoidbringerEditor (VoidbringerProcessor& p)
    : AudioProcessorEditor (p), proc (p), session (p, [&p] { return p.apvts.state; }),
      window (session, Branding::displayName, {}, 360), stage (session, p)
{
    setLookAndFeel (&laf);

    session.skip ({ PID::voiceMode, PID::bendRange, PID::octave, PID::lfoSync, PID::outGain, PID::softClip, PID::polyphony });
    session.rules[PID::glide]           = { 0.00f, 0.50f };
    session.rules[PID::carRatio].pick   = { 0.5f, 1.0f, 1.0f, 2.0f, 3.0f };
    session.rules[PID::modRatio].pick   = { 0.5f, 1.0f, 1.5f, 2.0f, 2.0f, 3.0f, 4.0f, 7.0f };
    session.rules[PID::modFine]         = { 0.40f, 0.60f };
    session.rules[PID::fmAmount]        = { 0.20f, 0.85f };
    session.rules[PID::feedback]        = { 0.10f, 0.80f };
    session.rules[PID::stereoDetune]    = { 0.00f, 0.60f };
    session.rules[PID::fmEnvAmount]     = { 0.30f, 0.90f };
    session.rules[PID::fmEnvDecay]      = { 0.20f, 0.70f };
    session.rules[PID::pitchEnvAmount]  = { 0.40f, 0.75f };
    session.rules[PID::pitchEnvTime]    = { 0.10f, 0.60f };
    session.rules[PID::lfoDepth]        = { 0.00f, 0.60f };
    session.rules[PID::cutoff]          = { 0.30f, 0.85f };
    session.rules[PID::resonance]       = { 0.00f, 0.75f };
    session.rules[PID::filterEnvAmount] = { 0.35f, 0.85f };
    session.rules[PID::filterEnvDecay]  = { 0.20f, 0.70f };
    session.rules[PID::drive]           = { 0.20f, 0.80f };
    session.rules[PID::distMix]         = { 0.50f, 1.00f };
    session.rules[PID::ott]             = { 0.00f, 0.70f };
    session.rules[PID::ampAttack]       = { 0.00f, 0.30f };
    session.rules[PID::ampDecay]        = { 0.20f, 0.80f };
    session.rules[PID::ampSustain]      = { 0.50f, 1.00f };
    session.rules[PID::ampRelease]      = { 0.10f, 0.60f };

    auto& s = session;
    auto knob = [&] (const char* id, const char* l, bool lk = false, bool big = false) { return keep (std::make_unique<Knob> (s, id, l, lk, big)); };
    auto planet = [&] (const char* id, const char* l) { return keep (std::make_unique<Planet> (s, id, l)); };
    auto moon = [&] (const char* id, const char* l) { return keep (std::make_unique<Moon> (s, id, l)); };
    auto fader = [&] (const char* id, const char* l) { return keep (std::make_unique<Fader> (s, id, l, 100.0f)); };
    auto stack = [&] (std::initializer_list<Component*> c) { return keep (std::make_unique<Stack> (c)); };
    auto buttons = [&] (const char* id, const char* l, StringArray names) { return keep (std::make_unique<Buttons> (s, id, l, names)); };
    auto star = [&] (const char* id, const char* l) { return keep (std::make_unique<Star> (s, id, l)); };

    window.panel.setPages ({ "FM", "LFO + FILTER", "AMP + OUT" });
    window.panel.addGroup (group ("FM OPERATORS", { planet (PID::carRatio, "Carrier"), planet (PID::modRatio, "Modulator"), knob (PID::modFine, "Mod Fine"),
                                                    knob (PID::fmAmount, "FM Amount", true, true), knob (PID::feedback, "Feedback", true),
                                                    planet (PID::stereoDetune, "Detune") }), 0);
    window.panel.addGroup (group ("FM ENVELOPE", { knob (PID::fmEnvAmount, "Amount"), knob (PID::fmEnvDecay, "Decay") }), 0);
    window.panel.addGroup (group ("PITCH ENVELOPE", { knob (PID::pitchEnvAmount, "Amount"), knob (PID::pitchEnvTime, "Time") }), 0);
    window.panel.addGroup (group ("VOICE", { stack ({ buttons (PID::voiceMode, "Voice", { "MONO", "LEGATO" }),
                                                      buttons (PID::octave, "Octave", { "-2", "-1", "0", "+1", "+2" }),
                                                      keep (std::make_unique<ListPicker> (s, PID::bendRange, "Bend", 130.0f)) }),
                                             knob (PID::glide, "Glide"), stack ({ star (PID::polyphony, "Chords") }) }), 0);
    window.panel.addGroup (group ("LFO", { stack ({ buttons (PID::lfoWave, "Wave", { "SIN", "TRI", "SQR", "SAW", "RND" }),
                                                    buttons (PID::lfoDest, "Target", { "PITCH", "FM", "CUTOFF" }) }),
                                           stack ({ star (PID::lfoSync, "Sync"), keep (std::make_unique<ListPicker> (s, PID::lfoDivision, "Division", 120.0f)) }),
                                           knob (PID::lfoRate, "Rate"), moon (PID::lfoDepth, "Depth") }), 1);
    window.panel.addGroup (group ("FILTER", { stack ({ buttons (PID::filterType, "Type", { "LOWPASS", "BANDPASS", "VOWEL" }) }),
                                              knob (PID::cutoff, "Cutoff"), knob (PID::resonance, "Reso"),
                                              knob (PID::filterEnvAmount, "Env Amt"), knob (PID::filterEnvDecay, "Env Decay") }), 1);
    window.panel.addGroup (group ("AMP ENV", { fader (PID::ampAttack, "A"), fader (PID::ampDecay, "D"), fader (PID::ampSustain, "S"), fader (PID::ampRelease, "R") }), 2);
    window.panel.addGroup (group ("DISTORTION", { knob (PID::drive, "Drive", false, true), moon (PID::distMix, "Dist Mix"), moon (PID::ott, "OTT") }), 2);
    window.panel.addGroup (group ("OUTPUT", { fader (PID::outGain, "Output"), stack ({ star (PID::softClip, "Soft Clip") }) }), 2);
    window.panel.addPage (keep (std::make_unique<Vb::OwnSoundGroup> (session, proc.own)), "OWN SOUND");
    window.setStage (&stage);

    for (auto& fp : factoryPresets()) window.top.factoryNames.add (fp.name);
    window.top.applyFactory = [this] (int i) { applyFactoryPreset (proc.apvts, i); };
    window.top.currentName = [this] { return proc.apvts.state.getProperty ("preset").toString(); };
    window.top.onUserPresetLoaded = [this] (const String& n) { proc.apvts.state.setProperty ("preset", n, nullptr); window.top.refresh(); };
    session.onRandomized = [this] { proc.apvts.state.setProperty ("preset", "Random", nullptr); window.top.refresh(); };
    window.footer.level = [this] { return jmax (proc.outMeter.take (0), proc.outMeter.take (1)); };
    window.footer.clipping = [this] { return proc.clipAmount.load() > 0.001f; };

    Vb::makeResizable (*this, window, proc.apvts.state);
    setOpaque (true);
    Vb::keepKeyboardForHost (*this);
}

VoidbringerEditor::~VoidbringerEditor() { setLookAndFeel (nullptr); }

void VoidbringerEditor::resized() { Vb::scaleWindow (*this, window, proc.apvts.state); }

void VoidbringerEditor::prepareScreenshot (const float* wave, int n)
{
    Stage::freezeAnimation (true);
    window.footer.setStatic (-2.5f);
    stage.staticWave.assign (wave, wave + n);
}
