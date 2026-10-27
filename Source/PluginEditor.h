#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "VoidUI.h"

// Stage: the gravity well. Drag the probe: closer to the black hole = more FM, around it = more feedback; the stream
// pulled off the probe shows how hard the FM works, the disk spins with the modulator ratio. The live waveform runs
// across the top.
class WellStage : public Vb::Stage
{
public:
    WellStage (Vb::Session&, VoidbringerProcessor&);
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override { end(); }
    std::vector<float> staticWave;   // screenshot only
private:
    void drawStage (juce::Graphics&, juce::Rectangle<float>, double timeMs) override;
    juce::StringArray readout() override;
    void moveTo (juce::Point<float>);
    VoidbringerProcessor& proc;
    std::vector<StarDot> stars, disk;
};

class VoidbringerEditor : public juce::AudioProcessorEditor
{
public:
    explicit VoidbringerEditor (VoidbringerProcessor&);
    ~VoidbringerEditor() override;
    void paint (juce::Graphics& g) override { g.fillAll (Vb::Col::bg); }
    void resized() override;
    void prepareScreenshot (const float* wave, int n);   // render test: fixed moment + a given waveform

private:
    template <class T> T* keep (std::unique_ptr<T> c) { auto* raw = c.get(); parts.push_back (std::move (c)); return raw; }
    Vb::Group* group (const juce::String& title, std::initializer_list<juce::Component*> items)
    { return keep (std::make_unique<Vb::Group> (title, items)); }

    VoidbringerProcessor& proc;
    Vb::LookAndFeel laf;
    Vb::Session session;
    Vb::Window window;
    WellStage stage;
    std::vector<std::unique_ptr<juce::Component>> parts;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoidbringerEditor)
};
