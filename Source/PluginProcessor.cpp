#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    constexpr double twoPi = juce::MathConstants<double>::twoPi;
    inline float fastSin (double ph) { return (float) std::sin (twoPi * ph); }
    inline float msCoef (float ms, double rate) { return (float) std::exp (-1.0 / juce::jmax (1.0e-6, (double) ms * 0.001 * rate)); }
}

VoidbringerProcessor::VoidbringerProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "VoidbringerState", createParameterLayout())
{
    own.attach (apvts);
    own.gate = true;   // note-off ends the own sample
    apvts.state.setProperty ("preset", "Rawstyle Terror Screech", nullptr);   // the parameter defaults = that factory preset
}

bool VoidbringerProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void VoidbringerProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    own.prepare (sampleRate);
    sr = sampleRate;
    srOS = sr * osFactor;
    maxBlock = juce::jmax (1, samplesPerBlock);
    os = std::make_unique<juce::dsp::Oversampling<float>> (2, (size_t) osOrder,
            juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false);
    os->initProcessing ((size_t) maxBlock);
    os->reset();
    monoTmp.setSize (2, maxBlock);
    chunkMidi.ensureSize (4096);
    sustain = false;
    sustainedOff.fill (false);
    heldNotes.clear();
    for (auto& v : voices) { v.amp.setSampleRate (srOS); v.amp.reset(); v.active = v.keyDown = false; v.key = -1; v.resetState(); }
    lastPoly = polyOn();
    polyComp = 1.0f;
    for (int c = 0; c < 2; ++c)
    {
        bandLp1[c] = bandLp2[c] = 0;
        for (auto& e : bandEnv[c]) e = 0;
    }
    dcR = (float) std::exp (-twoPi * 10.0 / srOS);
    outG.reset (sr, 0.02); outG.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (p (PID::outGain)));
    ottS.reset (sr, 0.05); ottS.setCurrentAndTargetValue (p (PID::ott) * 0.01f);
    setLatencySamples ((int) std::round (os->getLatencyInSamples()));   // the oversampler's (small) delay, for host PDC
    own.latency = getLatencySamples();   // the own sample starts in time with the engine
}

// ------------------------------------------------------------------ MIDI / voice
void VoidbringerProcessor::noteOn (int note)
{
    sustainedOff[(size_t) (note & 127)] = false;
    if (lastPoly) { heldNotes.push (note); polyNoteOn (note); return; }
    auto& v = voices[0];
    const bool wasHeld = ! heldNotes.empty();
    heldNotes.remove (note);
    heldNotes.push (note);
    v.targetNote = (float) note;
    const bool legato = (int) p (PID::voiceMode) == 1;

    if (! v.active || ! v.amp.isActive())
        v.curNote = v.targetNote;             // first note: no glide from nowhere
    if (! wasHeld || ! legato)
    {
        v.amp.noteOn();
        v.fmEnv = v.pitchEnv = v.filtEnv = 1.0f;
        if (! v.active || ! v.amp.isActive()) { lfoPh = 0.0; }
        if (! wasHeld && ! legato) lfoPh = 0.0;
    }
    v.active = true;
}

void VoidbringerProcessor::noteOff (int note)
{
    if (sustain) { sustainedOff[(size_t) (note & 127)] = true; return; }   // pedal down: the note keeps sounding
    heldNotes.remove (note);
    if (lastPoly) { polyNoteOff (note); return; }
    auto& v = voices[0];
    if (heldNotes.empty()) v.amp.noteOff();
    else v.targetNote = (float) heldNotes.back();   // fall back to previous held note (slides back)
}

// Polyphony = On: one voice per key (no glide between voices; LFO shared), oldest released voice stolen first
void VoidbringerProcessor::polyNoteOn (int note)
{
    Voice* v = nullptr;
    for (auto& x : voices) if (x.active && x.key == note) { v = &x; break; }          // same key again: retrigger it
    if (v == nullptr) for (auto& x : voices) if (! x.active) { v = &x; break; }
    if (v == nullptr) for (auto& x : voices) if (! x.keyDown && (v == nullptr || x.age < v->age)) v = &x;
    if (v == nullptr) for (auto& x : voices) if (v == nullptr || x.age < v->age) v = &x;
    if (! v->active) v->resetState();
    bool anyActive = false;
    for (auto& x : voices) anyActive = anyActive || x.active;
    if (! anyActive) lfoPh = 0.0;
    v->key = note;
    v->keyDown = true;
    v->age = ++ageCounter;
    v->targetNote = v->curNote = (float) note;
    v->amp.noteOn();
    v->fmEnv = v->pitchEnv = v->filtEnv = 1.0f;
    v->active = true;
}

void VoidbringerProcessor::polyNoteOff (int note)
{
    for (auto& x : voices)
        if (x.active && x.key == note && x.keyDown) { x.keyDown = false; x.amp.noteOff(); }
}

void VoidbringerProcessor::handleMidi (const juce::MidiMessage& m)
{
    if (m.isNoteOn()) noteOn (m.getNoteNumber());
    else if (m.isNoteOff()) noteOff (m.getNoteNumber());
    else if (m.isPitchWheel()) bend = (float) (m.getPitchWheelValue() - 8192) / 8192.0f;
    else if (m.isSustainPedalOn()) sustain = true;
    else if (m.isSustainPedalOff())
    {
        sustain = false;
        for (int n = 0; n < 128; ++n)
            if (sustainedOff[(size_t) n]) { sustainedOff[(size_t) n] = false; noteOff (n); }
    }
    else if (m.isAllNotesOff() || m.isAllSoundOff())
    {
        heldNotes.clear(); sustainedOff.fill (false);
        for (auto& v : voices)
        {
            v.amp.noteOff(); v.keyDown = false;
            if (m.isAllSoundOff()) { v.amp.reset(); v.active = false; }
        }
    }
}

float VoidbringerProcessor::lfoValue (int wave)
{
    const float ph = (float) lfoPh;
    switch (wave)
    {
        case 0:  return std::sin ((float) twoPi * ph);
        case 1:  return 1.0f - 4.0f * std::abs (ph - 0.5f);
        case 2:  return ph < 0.5f ? 1.0f : -1.0f;
        case 3:  return 1.0f - 2.0f * ph;
        default: return lfoHold;
    }
}

// ------------------------------------------------------------------ synthesis (oversampled)
void VoidbringerProcessor::render (int startBase, int numBase)
{
    const int start = startBase * osFactor, n = numBase * osFactor;
    for (int c = 0; c < 2; ++c) juce::FloatVectorOperations::clear (up[c] + start, n);
    const bool poly = lastPoly;
    const int nv = maxVoices;   // inactive voices are skipped; in mono only voices[0] gets new notes
    bool any = false;
    for (int vi = 0; vi < nv; ++vi) any = any || voices[(size_t) vi].active;
    if (! any) return;

    // read parameters once per segment
    const float glideMs   = p (PID::glide);
    const float bendRange = p (PID::bendRange);
    const float octave    = p (PID::octave);
    const float carRatio  = p (PID::carRatio);
    const float modRatio  = p (PID::modRatio) * std::pow (2.0f, p (PID::modFine) / 1200.0f);
    const float fmAmt     = p (PID::fmAmount) * 0.01f;
    const float fb        = p (PID::feedback) * 0.01f * 1.7f;
    const float detune    = p (PID::stereoDetune);
    const float fmEnvAmt  = p (PID::fmEnvAmount) * 0.01f;
    const float pEnvAmt   = p (PID::pitchEnvAmount);
    const float fEnvAmt   = p (PID::filterEnvAmount) * 0.01f;
    const int   lfoWave   = (int) p (PID::lfoWave);
    const bool  sync      = (int) p (PID::lfoSync) == 1;
    const double lfoHz    = sync ? (bpm / 60.0) / lfoDivisionBeats ((int) p (PID::lfoDivision)) : (double) p (PID::lfoRate);
    const float lfoDepth  = p (PID::lfoDepth) * 0.01f;
    const int   lfoDest   = (int) p (PID::lfoDest);
    const int   fType     = (int) p (PID::filterType);
    const float cutoff    = p (PID::cutoff);
    const float reso      = p (PID::resonance) * 0.01f;
    const float driveG    = juce::Decibels::decibelsToGain (p (PID::drive));
    const float dMix      = p (PID::distMix) * 0.01f;

    const juce::ADSR::Parameters ampP { p (PID::ampAttack) * 0.001f, p (PID::ampDecay) * 0.001f, p (PID::ampSustain) * 0.01f, p (PID::ampRelease) * 0.001f };
    for (int vi = 0; vi < nv; ++vi) voices[(size_t) vi].amp.setParameters (ampP);
    int numActive = 0;
    for (int vi = 0; vi < nv; ++vi) numActive += voices[(size_t) vi].active ? 1 : 0;
    // poly: a chord's sum is scaled by 1/sqrt(voices) (smoothed), so a 3-note stack is not 3x louder into the OTT/clipper
    const float compTarget = poly ? 1.0f / std::sqrt ((float) juce::jmax (1, numActive)) : 1.0f;
    const float compC = msCoef (5.0f, srOS);
    const float glideC = glideMs < 0.5f ? 0.0f : msCoef (glideMs * 0.25f, srOS);   // ~ reaches target in glideMs
    const float fmEnvC = msCoef (p (PID::fmEnvDecay) * 0.25f, srOS);
    const float pEnvC  = msCoef (p (PID::pitchEnvTime) * 0.25f, srOS);
    const float fEnvC  = msCoef (p (PID::filterEnvDecay) * 0.25f, srOS);
    const float detRat[2] = { std::pow (2.0f, -detune / 2400.0f), std::pow (2.0f, detune / 2400.0f) };
    const float k = juce::jmax (0.04f, 2.0f * (1.0f - 0.98f * reso));
    const float kF = juce::jmax (0.06f, 2.0f * (1.0f - 0.95f * reso) * 0.5f + 0.03f);
    const float nyq = (float) (0.45 * srOS);
    const double lfoInc = lfoHz / srOS;
    constexpr float formantRatio[3] = { 1.0f, 1.62f, 3.3f }, formantGain[3] = { 1.0f, 0.75f, 0.45f };
    const float asym = 0.25f, asymOff = std::tanh (asym);

    for (int i = 0; i < n; ++i)
    {
        // LFO (shared by all voices)
        lfoPh += lfoInc;
        if (lfoPh >= 1.0) { lfoPh -= 1.0; lfoHold = rng.nextFloat() * 2.0f - 1.0f; }
        const float lfo = lfoValue (lfoWave) * lfoDepth;
        polyComp = compTarget + compC * (polyComp - compTarget);

        for (int vi = 0; vi < nv; ++vi)
        {
            auto& v = voices[(size_t) vi];
            if (! v.active) continue;
            auto& curNote = v.curNote;
            auto& fmEnv = v.fmEnv; auto& pitchEnv = v.pitchEnv; auto& filtEnv = v.filtEnv;
            auto& carPh = v.carPh; auto& modPh = v.modPh; auto& modPrev = v.modPrev;
            auto& svf = v.svf; auto& dcX = v.dcX; auto& dcY = v.dcY;

            // pitch / glide (glide only in mono/legato)
            const float tgt = v.targetNote + octave * 12.0f + bend * bendRange;
            curNote = (glideC > 0.0f && ! poly && vi == 0) ? tgt + (curNote - tgt) * glideC : tgt;

            const float semis = curNote + pEnvAmt * pitchEnv + (lfoDest == 0 ? lfo * 12.0f : 0.0f);
            const float baseHz = 440.0f * std::exp2 ((semis - 69.0f) / 12.0f);
            const float index = juce::jlimit (0.0f, 12.0f, 8.0f * (fmAmt + fmEnvAmt * fmEnv + (lfoDest == 1 ? lfo * 0.5f : 0.0f)));
            const float fc = juce::jlimit (20.0f, juce::jmin (20000.0f, nyq),
                                           cutoff * std::exp2 (fEnvAmt * 5.0f * filtEnv + (lfoDest == 2 ? lfo * 3.0f : 0.0f)));
            const float g = std::tan (juce::MathConstants<float>::pi * fc / (float) srOS);
            const float a = v.amp.getNextSample() * (poly ? polyComp : 1.0f);
            for (int c = 0; c < 2; ++c)
            {
                const double f = (double) baseHz * detRat[c];
                // modulator with (averaged) self-feedback -> saw-like, gritty
                modPh[c] += f * modRatio / srOS; modPh[c] -= std::floor (modPh[c]);
                const float fbSig = 0.5f * (modPrev[c][0] + modPrev[c][1]);
                const float m = std::sin ((float) (twoPi * modPh[c]) + fb * fbSig);
                modPrev[c][1] = modPrev[c][0]; modPrev[c][0] = m;
                // carrier
                carPh[c] += f * carRatio / srOS; carPh[c] -= std::floor (carPh[c]);
                float x = std::sin ((float) (twoPi * carPh[c]) + index * m);

                // filter
                float lp, bp;
                if (fType == 0)      { svf[c][0].process (x, g, k, lp, bp); x = lp * juce::jmin (1.0f, 0.5f + 0.5f * k); }
                else if (fType == 1) { svf[c][0].process (x, g, k, lp, bp); x = bp * juce::jmax (k, 0.35f) * 1.6f; }
                else
                {
                    float y = 0.0f;
                    for (int b = 0; b < 3; ++b)
                    {
                        const float fb2 = juce::jmin (fc * formantRatio[b], nyq);
                        const float gb = b == 0 ? g : std::tan (juce::MathConstants<float>::pi * fb2 / (float) srOS);
                        svf[c][b].process (x, gb, kF, lp, bp);
                        y += bp * formantGain[b];
                    }
                    x = y * kF * 1.8f;
                }

                // asymmetric tanh distortion + DC blocker
                const float d = (std::tanh (x * driveG + asym) - asymOff) * 0.85f;
                float y = d - dcX[c] + dcR * dcY[c];
                dcX[c] = d; dcY[c] = y;
                x = x * (1.0f - dMix) + y * dMix;

                up[c][start + i] += x * a;
            }

            fmEnv *= fmEnvC; pitchEnv *= pEnvC; filtEnv *= fEnvC;
        }
    }
    for (int vi = 0; vi < nv; ++vi)
    {
        auto& v = voices[(size_t) vi];
        if (! v.amp.isActive() && ((vi == 0 && ! poly) ? heldNotes.empty() : ! v.keyDown)) v.active = false;
    }
}

void VoidbringerProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    const int nch = buffer.getNumChannels();
    if (n == 0 || nch == 0) return;

    if (n > maxBlock)   // host sent a bigger block than announced -> chunks
    {
        for (int pos = 0; pos < n; pos += maxBlock)
        {
            const int len = juce::jmin (maxBlock, n - pos);
            juce::AudioBuffer<float> view (buffer.getArrayOfWritePointers(), nch, pos, len);
            chunkMidi.clear();   // preallocated in prepareToPlay -> no allocation on the audio thread
            const bool last = pos + len >= n;
            for (const auto md : midi)
                if (md.samplePosition >= pos && (md.samplePosition < pos + len || last))
                    chunkMidi.addEvent (md.getMessage(), juce::jmin (md.samplePosition - pos, len - 1));
            processBlock (view, chunkMidi);   // len <= maxBlock -> the nested call never touches chunkMidi itself
        }
        return;
    }

    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto b = pos->getBpm()) if (*b > 20.0 && *b < 999.0) bpm = *b;

    // up-sampled work buffer (contents overwritten by the synth)
    juce::AudioBuffer<float> tmp (monoTmp.getArrayOfWritePointers(), 2, n);   // only used for a mono bus
    juce::dsp::AudioBlock<float> base;
    buffer.clear();   // clear first (resets the buffer's isClear flag BEFORE we grab write pointers)
    if (nch < 2) { juce::FloatVectorOperations::clear (tmp.getWritePointer (0), n); juce::FloatVectorOperations::clear (tmp.getWritePointer (1), n); }
    float* chans[2] = { buffer.getWritePointer (0), buffer.getWritePointer (nch > 1 ? 1 : 0) };
    juce::AudioBuffer<float>* work = &buffer;
    if (nch < 2) { work = &tmp; chans[0] = tmp.getWritePointer (0); chans[1] = tmp.getWritePointer (1); }
    juce::ignoreUnused (work);
    base = juce::dsp::AudioBlock<float> (chans, 2, (size_t) n);
    auto upBlock = os->processSamplesUp (base);
    up[0] = upBlock.getChannelPointer (0);
    up[1] = upBlock.getChannelPointer (1);

    if (polyOn() != lastPoly)   // Polyphony switched: release everything (short fade), the next keys use the new mode
    {
        lastPoly = ! lastPoly;
        for (auto& v : voices) { v.amp.noteOff(); v.keyDown = false; }
        heldNotes.clear();
    }

    // sample-accurate MIDI
    int pos = 0;
    for (const auto md : midi)
    {
        const int at = juce::jlimit (0, n, md.samplePosition);
        if (at > pos) { render (pos, at - pos); pos = at; }
        handleMidi (md.getMessage());
    }
    if (pos < n) render (pos, n - pos);

    os->processSamplesDown (base);

    // multiband upward/downward compression, output gain, soft clip
    outG.setTargetValue (juce::Decibels::decibelsToGain (p (PID::outGain)));
    ottS.setTargetValue (p (PID::ott) * 0.01f);
    const bool clipOn = (int) p (PID::softClip) == 1;
    const float c1 = (float) std::exp (-twoPi * 180.0 / sr), c2 = (float) std::exp (-twoPi * 2600.0 / sr);
    const float atk = msCoef (4.0f, sr), rel = msCoef (90.0f, sr);
    const float bandWeight[3] = { 1.0f, 1.0f, 1.1f };
    constexpr float thr = 0.7f, ceil = 0.98f;
    int clipped = 0;
    for (int i = 0; i < n; ++i)
    {
        const float amt = ottS.getNextValue(), og = outG.getNextValue();
        for (int c = 0; c < 2; ++c)
        {
            float x = chans[c][i];
            if (amt > 0.001f)
            {
                bandLp1[c] = x + c1 * (bandLp1[c] - x);
                const float low = bandLp1[c], rest = x - low;
                bandLp2[c] = rest + c2 * (bandLp2[c] - rest);
                const float bands[3] = { low, bandLp2[c], rest - bandLp2[c] };
                float sum = 0.0f;
                for (int b = 0; b < 3; ++b)
                {
                    const float lvl = std::abs (bands[b]);
                    auto& e = bandEnv[c][b];
                    e = lvl > e ? lvl + atk * (e - lvl) : lvl + rel * (e - lvl);
                    const float db = juce::Decibels::gainToDecibels (e + 1.0e-9f);
                    float gDb = 0.0f;
                    if (db > -14.0f) gDb = (-14.0f - db) * 0.66f;               // downward 3:1 above -14 dB
                    else if (db > -60.0f) gDb = juce::jmin (14.0f, (-14.0f - db) * 0.5f);  // upward 2:1, max +14 dB
                    sum += bands[b] * bandWeight[b] * juce::Decibels::decibelsToGain (gDb * amt);
                }
                x = x * (1.0f - amt) + sum * amt;
            }
            x *= og;
            if (! std::isfinite (x)) x = 0.0f;
            if (clipOn)
            {
                const float ax = std::abs (x);
                if (ax > thr) { x = std::copysign (thr + (ceil - thr) * std::tanh ((ax - thr) / (ceil - thr)), x); ++clipped; }
            }
            chans[c][i] = x;
            outMeter.update (c, std::abs (x));
        }
        const int w = scopeWrite.load (std::memory_order_relaxed);
        scope[w].store (0.5f * (chans[0][i] + chans[1][i]), std::memory_order_relaxed);
        scopeWrite.store ((w + 1) % scopeSize, std::memory_order_relaxed);
    }
    if (nch == 1)   // raw pointer maths: tmp's isClear flag is stale on purpose
    {
        float* o = buffer.getWritePointer (0);
        for (int i = 0; i < n; ++i) o[i] = 0.5f * (chans[0][i] + chans[1][i]);
    }
    for (int ch = 2; ch < nch; ++ch) buffer.clear (ch, 0, n);
    own.addMidi (midi);   // OWN SOUND (after the chunking above, so per chunk)
    own.render (buffer);
    clipAmount.store ((float) clipped / (float) (2 * n), std::memory_order_relaxed);
}

juce::AudioProcessorEditor* VoidbringerProcessor::createEditor() { return new VoidbringerEditor (*this); }

void VoidbringerProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

// a corrupted / hand-edited state must not inject NaN/inf parameter values: such PARAM entries are dropped,
// so those parameters load with their default instead
static juce::XmlElement& dropNonFiniteValues (juce::XmlElement& xml)
{
    for (int i = xml.getNumChildElements(); --i >= 0;)
        if (auto* c = xml.getChildElement (i))
            if (c->hasAttribute ("value") && ! std::isfinite (c->getDoubleAttribute ("value")))
                xml.removeChildElement (c, true);
    return xml;
}

void VoidbringerProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            apvts.replaceState (juce::ValueTree::fromXml (dropNonFiniteValues (*xml)));
            own.restore();
        }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new VoidbringerProcessor(); }
