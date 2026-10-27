#pragma once
// voidbringer by sonicsystems: the plugin's look. Black background, white type, red (#FF2E3D) as the only colour,
// JetBrains Mono, square corners, 1 px lines.
//
// A plugin window = top bar (name, presets, MY PRESETS, undo, RANDOM) + control panel on the left (groups of
// controls, auto-spaced) + a big animated "stage" on the right + a footer (status line, output meter).
// Every control talks to its parameter through juce::ParameterAttachment, so a control, the stage, automation and
// RANDOM all move the same value. Design reference: design/voidbringer.html.

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "OwnSound.h"
#include <functional>
#include <map>
#include <vector>

namespace Vb
{
// ================================================================================================ palette + type
namespace Col
{
    const juce::Colour bg     { 0xff000000 };
    const juce::Colour ink    { 0xffffffff };   // text, lit ticks
    const juce::Colour label  { 0xffbdbdbd };
    const juce::Colour dim    { 0xff3a3a3a };   // unlit
    const juce::Colour line   { 0xff2b2b2b };
    const juce::Colour line2  { 0xff333333 };
    const juce::Colour mute   { 0xff8a8a8a };
    const juce::Colour accent { 0xffff2e3d };   // the only colour
    const juce::Colour strobe { 0xffb3121f };   // RANDOM flash
}

enum class Weight { regular, semibold, extrabold };
// `px` is the em size (like CSS font-size); `tracking` is extra letter spacing in em
juce::Font mono (float px, Weight = Weight::regular, float tracking = 0.0f);
float textWidth (const juce::Font&, const juce::String&);
// the voidbringer mark: a black hole with a white photon ring and a red accretion disk
void drawMark (juce::Graphics&, juce::Rectangle<float>);

namespace Brand
{
    inline constexpr const char* maker = "sonicsystems";
}

class LookAndFeel : public juce::LookAndFeel_V4   // popup menus, the save-preset dialog, tooltips
{
public:
    LookAndFeel();
    juce::Typeface::Ptr getTypefaceForFont (const juce::Font&) override;
    juce::Font getPopupMenuFont() override;
    void drawPopupMenuBackground (juce::Graphics&, int w, int h) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
    juce::Font getTextButtonFont (juce::TextButton&, int) override;
    juce::Font getAlertWindowTitleFont() override;
    juce::Font getAlertWindowMessageFont() override;
    juce::Font getAlertWindowFont() override;
    void drawAlertBox (juce::Graphics&, juce::AlertWindow&, const juce::Rectangle<int>& textArea, juce::TextLayout&) override;
    void fillTextEditorBackground (juce::Graphics&, int w, int h, juce::TextEditor&) override;
    void drawTextEditorOutline (juce::Graphics&, int w, int h, juce::TextEditor&) override;
};

// ================================================================================================ session
// One per editor: status line, RANDOM (+ undo, locks), the strobe flash.
class Session : private juce::Timer
{
public:
    Session (juce::AudioProcessor&, std::function<juce::ValueTree()> stateGetter);
    ~Session() override;

    juce::AudioProcessor& proc;
    std::function<juce::ValueTree()> state;
    juce::RangedAudioParameter* param (const juce::String& id) const;

    // footer status line (bold part, rest); controls report hover, RANDOM reports what it did
    std::function<void (const juce::String&, const juce::String&)> showStatus;
    void hover (juce::RangedAudioParameter*);   // nullptr = back to the default hint
    void status (const juce::String& bold, const juce::String& rest);
    juce::String hintBold, hintRest;

    // FREEZE (right-click menu): a frozen control is never touched by RANDOM (kept in the state as "randomLocks")
    bool isLocked (const juce::String& id) const;
    void toggleLock (const juce::String& id);
    // right-click menu of a control: the host's own items (in FL / Cubase / Reaper / Bitwig: create an automation
    // clip / lane, link to a controller...) + FREEZE + reset. `editorChild` = any component inside the editor.
    void showParameterMenu (juce::Component& editorChild, juce::RangedAudioParameter&, std::function<void()> changed = {});
    // marks the parameter as "last tweaked" in the host (FL: Tools > Last tweaked > Create automation clip)
    void touchForHost (juce::RangedAudioParameter&);

    // RANDOM: musical ranges per parameter. Unlisted parameters use the full range (switches 50/50).
    struct Rule
    {
        float lo = 0.0f, hi = 1.0f;     // normalised range
        std::vector<float> pick;        // or: pick one of these real values
        float onChance = 0.5f;          // switches / two-state choices
        bool skip = false;              // never randomised (output, ceiling, MIDI channel, oversampling...)
    };
    std::map<juce::String, Rule> rules;
    void skip (std::initializer_list<const char*> ids) { for (auto* id : ids) rules[id].skip = true; }
    void randomize();
    void undo();
    bool canUndo() const { return ! undoStack.empty(); }
    std::function<void()> onUndoChanged, onFlash, onRandomized;
    float flash() const;   // 1 while the panel shows the red strobe

private:
    void timerCallback() override;
    void startMoves (const std::vector<std::pair<juce::RangedAudioParameter*, float>>& targets, bool stagger);
    void finishMoves();
    struct Move { juce::RangedAudioParameter* p; float from, to; double delay; };
    std::vector<Move> moves;
    double moveStart = 0.0, flashStart = -1.0e9;
    std::vector<std::vector<std::pair<juce::RangedAudioParameter*, float>>> undoStack;
    juce::Random rnd;
    std::map<juce::String, juce::RangedAudioParameter*> byId;
};

// ================================================================================================ layout
// Anything the auto-layout can place: its natural size at control scale z (1 = design size).
struct Sized
{
    virtual ~Sized() = default;
    virtual juce::Point<float> sizeAt (float z) const = 0;
    virtual void setScale (float) {}
};

// ================================================================================================ controls
// Base of every control: label + real-unit value under it, drag up/down (Shift/Ctrl = fine), double-click = default,
// wheel = step, right-click = menu (create automation clip, FREEZE from RANDOM, reset). `linked` = the control is also on the stage (red label).
class Control : public juce::Component, public Sized
{
public:
    Control (Session&, const juce::String& paramId, const juce::String& label, bool linked = false);
    ~Control() override;

    juce::RangedAudioParameter& param;
    float norm() const { return value; }
    juce::String valueText() const;
    juce::String fullName() const { return param.getName (64); }

    void paint (juce::Graphics&) override;
    void mouseEnter (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void setScale (float s) override { z = s; repaint(); }

protected:
    virtual void drawGraphic (juce::Graphics&, juce::Rectangle<float>) = 0;
    virtual float graphicHeight() const = 0;
    virtual bool dragsVertically() const { return true; }
    virtual float dragRange() const { return 150.0f; }   // pixels for the full range
    virtual void clickAt (juce::Point<float>) {}          // click controls (buttons, list, star)
    virtual bool showsValue() const { return true; }
    virtual bool showsLabel() const { return true; }
    virtual bool alignLeft() const { return false; }
    void setNorm (float n);                               // one complete gesture
    bool discrete() const;
    int steps() const;                                    // discrete: number of values - 1
    float snap (float n) const;
    float zeroNorm() const;                               // where 0 sits for a range across 0 (-1 = none)
    float labelWidth() const;
    float widestValue() const;

    Session& session;
    juce::String label;
    bool linked;
    float value = 0.0f, z = 1.0f;

private:
    juce::ParameterAttachment attachment;
    float dragValue = 0.0f, lastY = 0.0f;
    bool dragging = false, hovered = false;
};

// pulsar dial: 36 ticks, white needle like a rotating beam. Main sound shaping.
class Knob : public Control
{
public:
    Knob (Session& s, const juce::String& id, const juce::String& l, bool linked = false, bool big = false)
        : Control (s, id, l, linked), big (big) {}
    juce::Point<float> sizeAt (float z) const override;
protected:
    void drawGraphic (juce::Graphics&, juce::Rectangle<float>) override;
    float graphicHeight() const override { return (big ? 76.0f : 52.0f) * z; }
    bool big;
};

// orbit knob: a red planet on a tilted orbit around a small sun. Spread / character / sends.
class Planet : public Control
{
public:
    using Control::Control;
    juce::Point<float> sizeAt (float z) const override;
protected:
    void drawGraphic (juce::Graphics&, juce::Rectangle<float>) override;
    float graphicHeight() const override { return 44.0f * z + 8.0f; }
};

// thin track with ticks, red fill (from 0 for ranges across 0), white cap. Levels, envelopes, output.
class Fader : public Control
{
public:
    Fader (Session& s, const juce::String& id, const juce::String& l, float trackHeight = 110.0f, bool linked = false)
        : Control (s, id, l, linked), track (trackHeight) {}
    juce::Point<float> sizeAt (float z) const override;
protected:
    void drawGraphic (juce::Graphics&, juce::Rectangle<float>) override;
    float graphicHeight() const override { return track * z; }
    float dragRange() const override { return (track - 10.0f) * z; }
    float track;
};

// a moon from new to full as the value goes 0 -> 100 %, with a red arc for exact reading. Amounts.
class Moon : public Control
{
public:
    using Control::Control;
    juce::Point<float> sizeAt (float z) const override;
protected:
    void drawGraphic (juce::Graphics&, juce::Rectangle<float>) override;
    float graphicHeight() const override { return 48.0f * z + 4.0f; }
};

// square segmented buttons for short choices (selected = white with black text)
class Buttons : public Control
{
public:
    Buttons (Session&, const juce::String& id, const juce::String& l, juce::StringArray shortNames = {}, int columns = 0,
             bool linked = false);
    juce::Point<float> sizeAt (float z) const override;
protected:
    void drawGraphic (juce::Graphics&, juce::Rectangle<float>) override;
    float graphicHeight() const override;
    bool dragsVertically() const override { return false; }
    bool alignLeft() const override { return true; }
    void clickAt (juce::Point<float>) override;
private:
    juce::Rectangle<float> cell (int i, juce::Rectangle<float> area) const;
    std::vector<float> columnWidths() const;
    juce::StringArray names;
    int cols;
};

// ◂ value ▸ for long choice lists (click the arrows, scroll, or click the name for the whole list)
class ListPicker : public Control
{
public:
    ListPicker (Session& s, const juce::String& id, const juce::String& l, float width = 150.0f, bool linked = false)
        : Control (s, id, l, linked), boxWidth (width) {}
    juce::Point<float> sizeAt (float z) const override;
protected:
    void drawGraphic (juce::Graphics&, juce::Rectangle<float>) override;
    float graphicHeight() const override;
    bool dragsVertically() const override { return false; }
    bool showsValue() const override { return false; }
    bool alignLeft() const override { return true; }
    void clickAt (juce::Point<float>) override;
private:
    float boxWidth;
};

// a star that lights up: on / off switches
class Star : public Control
{
public:
    using Control::Control;
    juce::Point<float> sizeAt (float z) const override;
protected:
    void drawGraphic (juce::Graphics&, juce::Rectangle<float>) override;
    float graphicHeight() const override;
    bool dragsVertically() const override { return false; }
    bool showsValue() const override { return false; }
    bool showsLabel() const override { return false; }
    void clickAt (juce::Point<float>) override { setNorm (value > 0.5f ? 0.0f : 1.0f); }
};

// a plain push button in the same style (e.g. DRAG MIDI)
class PushButton : public juce::Component, public Sized
{
public:
    explicit PushButton (const juce::String& text);
    std::function<void()> onClick;
    juce::Point<float> sizeAt (float z) const override;
    void setScale (float s) override { z = s; }
    void paint (juce::Graphics&) override;
    void mouseUp (const juce::MouseEvent&) override;
private:
    juce::String text;
    float z = 1.0f;
};

// a fixed-size display (scope, curve, meter) that sits in the panel like a control
class Display : public juce::Component, public Sized
{
public:
    Display (float w, float h) : size (w, h) {}
    juce::Point<float> sizeAt (float) const override { return size; }
protected:
    juce::Point<float> size;
};

// gain reduction now (ducking, gating): 12 segments from 0 to -24 dB + the value
class ReductionMeter : public Display, private juce::Timer
{
public:
    explicit ReductionMeter (std::function<float()> dbNow, float w = 132.0f, float h = 62.0f);
    void paint (juce::Graphics&) override;
    void setStatic (float db) { frozen = true; shown = db; repaint(); }
private:
    void timerCallback() override;
    std::function<float()> db;
    float shown = 0.0f;
    bool frozen = false;
};

// controls stacked vertically (gap 7), left-aligned (or centred)
class Stack : public juce::Component, public Sized
{
public:
    Stack (std::initializer_list<juce::Component*> items, bool centred = false);
    juce::Point<float> sizeAt (float z) const override;
    void setScale (float s) override;
    void resized() override;
private:
    std::vector<juce::Component*> items;
    bool centred;
    float z = 1.0f;
};

// a titled group of controls in one row (bottom-aligned, spread evenly when the group is wider than needed)
class Group : public juce::Component, public Sized
{
public:
    Group (const juce::String& title, std::initializer_list<juce::Component*> items);
    Group (const juce::String& title, std::vector<juce::Component*> items);
    std::function<bool()> active;   // e.g. sidechain controls only in SIDECHAIN mode; dimmed when false
    juce::Point<float> sizeAt (float z) const override;
    void setScale (float s) override;
    void resized() override;
    void paint (juce::Graphics&) override;
    void refreshActive();
private:
    juce::String title;
    std::vector<juce::Component*> items;
    float z = 1.0f;
};

// OWN SOUND: the box that holds the user's own sample (click = load, drop a file on it, right-click = menu) with a
// small waveform of it, and the group around it: box + Level + Tune + Pitch (fixed / follows notes) + Layer / Replace.
class OwnSoundBox : public juce::Component, public Sized, public juce::FileDragAndDropTarget, private juce::Timer
{
public:
    OwnSoundBox (Session&, Sonic::OwnSound&);
    juce::Point<float> sizeAt (float z) const override;
    void setScale (float s) override { z = s; repaint(); }
    void paint (juce::Graphics&) override;
    void mouseEnter (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    bool isInterestedInFileDrag (const juce::StringArray&) override;
    void fileDragEnter (const juce::StringArray&, int, int) override { dragOver = true; repaint(); }
    void fileDragExit (const juce::StringArray&) override { dragOver = false; repaint(); }
    void filesDropped (const juce::StringArray&, int, int) override;
private:
    void timerCallback() override;
    void choose();
    void load (const juce::File&);
    void refresh();
    void hint();
    Session& session;
    Sonic::OwnSound& own;
    std::unique_ptr<juce::FileChooser> chooser;
    std::vector<float> peaks;
    juce::String shownName;
    int lastHits = 0;
    double hitAt = -1.0e9;
    float z = 1.0f;
    bool hovered = false, dragOver = false;
};

struct OwnSoundParts
{
    OwnSoundParts (Session&, Sonic::OwnSound&);
    OwnSoundBox box;
    Knob level, tune;
    Buttons key, mode;
    Stack modes;
};

class OwnSoundGroup : private OwnSoundParts, public Group
{
public:
    // `extra` = plugin-specific controls after the standard ones (e.g. which drum it replaces)
    OwnSoundGroup (Session&, Sonic::OwnSound&, std::vector<juce::Component*> extra = {});
};

// The left side of the window: picks the biggest control scale (up to 1.5x) at which the groups fit, splits them
// into lines of similar width (order kept), spreads the lines evenly from top to bottom and lets every group grow
// in proportion to its size, so the panel is evenly filled. Optional pages (tabs) for big plugins.
class Panel : public juce::Component, private juce::Timer
{
public:
    Panel() { startTimerHz (8); }               // re-checks the groups' `active` conditions
    void addGroup (Group*, int page = 0);      // not owned
    void setPages (const juce::StringArray& names);
    // a group on a page of its own (e.g. OWN SOUND); a plugin without pages gets "SOUND" + this one
    void addPage (Group*, const juce::String& name);
    void showLastPage() { showPage (juce::jmax (0, pages.size() - 1)); }   // render tests: the OWN SOUND page
    void resized() override;
    void paint (juce::Graphics&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void refreshActive();
    float scaleOfPage (int p) const { return p < (int) pageScale.size() ? pageScale[(size_t) p] : 1.0f; }
private:
    void timerCallback() override { refreshActive(); }
    void showPage (int);
    void fit (int page, juce::Rectangle<int> area);
    juce::Rectangle<float> tabRect (int i) const;
    std::vector<std::pair<Group*, int>> groups;
    juce::StringArray pages;
    std::vector<float> pageScale;
    int current = 0;
};

// ================================================================================================ stage
// The big starfield display with the plugin's signature control. Animates at ~30 fps, only while visible.
// Subclasses draw in drawStage() and move parameters through the Gesture helper.
class Stage : public juce::Component, private juce::Timer
{
public:
    Stage (Session&, const juce::String& hint, const juce::String& how);
    ~Stage() override;
    void paint (juce::Graphics&) final;
    void visibilityChanged() override { updateTimer(); }
    void parentHierarchyChanged() override { updateTimer(); }
    static void freezeAnimation (bool shouldFreeze);   // screenshots: every stage draws one fixed moment

protected:
    virtual void drawStage (juce::Graphics&, juce::Rectangle<float> area, double timeMs) = 0;
    virtual juce::StringArray readout() = 0;          // lines "LABEL     value"
    static juce::String row (const juce::String& name, const juce::String& value) { return name.paddedRight (' ', 9) + value; }

    float get (const juce::String& id) const;          // normalised value
    juce::String text (const juce::String& id) const;  // real-unit text
    juce::RangedAudioParameter* p (const juce::String& id) const { return session.param (id); }

    // drag gestures from the stage (begin on mouse down, end on mouse up)
    void begin (std::initializer_list<const char*> ids);
    void set (const juce::String& id, float norm);
    void end();
    void nudge (const juce::String& id, float wheelDelta, bool fine);   // one complete wheel step

    struct StarDot { float x, y, b; };
    static std::vector<StarDot> makeStars (int count, int seed);
    static void drawStars (juce::Graphics&, juce::Rectangle<float>, const std::vector<StarDot>&, float alpha = 1.0f);

    Session& session;
    juce::String hint, how;

private:
    void timerCallback() override { repaint(); }
    void updateTimer();
    std::vector<juce::RangedAudioParameter*> held;
};

// A planetary system: tilted rings around a small sun, one planet per parameter, dragged round its ring
// (solarsynth, starlightharmony, kickolossusgigantus). Subclasses add decorations and the readout.
class OrbitStage : public Stage
{
public:
    struct Ring { juce::String id; float rx, ry; juce::String tag; };   // radii as a fraction of the stage width
    OrbitStage (Session&, const juce::String& hint, const juce::String& how, std::vector<Ring>);
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

protected:
    void drawStage (juce::Graphics&, juce::Rectangle<float>, double timeMs) override;
    virtual bool hollow (int) const { return false; }                 // e.g. a distortion stage set to OFF
    virtual float pulse (double) const { return 0.0f; }               // 0..1, makes the sun beat
    virtual void drawBehind (juce::Graphics&, juce::Rectangle<float>, double) {}
    virtual void drawSunExtras (juce::Graphics&, juce::Point<float>, double) {}
    // in the tilted ring plane (origin = the sun): `a` = the planet's angle (0 = right, clockwise)
    virtual void drawRingExtras (juce::Graphics&, int ring, float a, float rx, float ry, double) {}
    static float ringAngle (float n) { return -juce::MathConstants<float>::halfPi + n * juce::MathConstants<float>::twoPi * 0.97f; }
    std::vector<Ring> rings;
    std::vector<StarDot> stars;
    float tilt = -0.24f;

private:
    juce::Point<float> toPlane (juce::Point<float>) const;
    void dragTo (juce::Point<float>);
    int active = -1;
};

// ================================================================================================ chrome
// MY PRESETS: the plugin's own saved state as an XML file in Documents/sonicsystems Presets/<plugin>/<name>.vbpreset.
namespace UserPresets
{
    juce::File folder (const juce::String& plugin);
    juce::Array<juce::File> list (const juce::String& plugin, const juce::String& legacyPlugin = {});
    bool saveTo (juce::AudioProcessor&, const juce::File&);
    bool loadFrom (juce::AudioProcessor&, const juce::File&, const std::function<juce::ValueTree()>& state = {});
}

class TopBar : public juce::Component
{
public:
    // factory presets: names + apply(index); `current` returns the name to show
    TopBar (Session&, const juce::String& pluginName, const juce::String& legacyPresetFolder);
    juce::StringArray factoryNames;
    std::function<void (int)> applyFactory;
    std::function<juce::String()> currentName;
    std::function<void (const juce::String&)> onUserPresetLoaded;
    void refresh() { repaint(); }

    void paint (juce::Graphics&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
private:
    enum class Zone { none, prev, name, next, mine, undo, random };
    Zone zoneAt (juce::Point<float>) const;
    juce::Rectangle<float> zoneRect (Zone) const;
    void stepPreset (int delta);
    void showFactoryMenu();
    void showMyPresets();
    void askNameAndSave();
    Session& session;
    juce::String pluginName, legacyFolder;
    juce::Array<juce::File> userFiles;
    Zone over = Zone::none;
};

class Footer : public juce::Component, private juce::Timer
{
public:
    Footer();
    std::function<float()> level;   // output peak 0..1 (linear), called 30x per second
    std::function<bool()> clipping;
    void setStatus (const juce::String& bold, const juce::String& rest) { boldText = bold; restText = rest; repaint(); }
    void paint (juce::Graphics&) override;
    void setStatic (float levelDb) { staticDb = levelDb; }
private:
    void timerCallback() override;
    juce::String boldText, restText;
    float shown = -60.0f, staticDb = 1.0f;
    bool clip = false;
};

// ================================================================================================ window
// 1000 x 580 design size; the editor scales it as a whole (60 - 150 %, remembered as "uiScale" in the state).
class Window : public juce::Component
{
public:
    static constexpr int W = 1000, H = 580;
    Window (Session&, const juce::String& pluginName, const juce::String& legacyPresetFolder, int stageWidth);
    void setStage (Stage*);   // not owned
    void paint (juce::Graphics&) override;
    void resized() override;

    Session& session;
    TopBar top;
    Panel panel;
    Footer footer;
private:
    Stage* stage = nullptr;
    int stageWidth;
};

// End of the editor constructor: puts `window` into the editor, makes it resizable and sizes it (the stored scale,
// or on the first open the largest that fits the screen, max 100 %). Call scaleWindow() from the editor's resized().
void makeResizable (juce::AudioProcessorEditor&, Window&, juce::ValueTree state);
float scaleWindow (juce::AudioProcessorEditor&, Window&, juce::ValueTree state);
// Clicking a control must not steal the keyboard from the host (FL's space bar). Call at the end of the constructor.
void keepKeyboardForHost (juce::Component&);
}
