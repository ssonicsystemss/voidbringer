#include "VoidUI.h"
#include "VoidFonts.h"

using namespace juce;

namespace Vb
{
namespace
{
    constexpr float pi = MathConstants<float>::pi, twoPi = MathConstants<float>::twoPi;

    Typeface::Ptr typefaceFor (Weight w)
    {
        static Typeface::Ptr reg  = Typeface::createSystemTypefaceFor (VoidFonts::JetBrainsMonoRegular_ttf,
                                                                       VoidFonts::JetBrainsMonoRegular_ttfSize);
        static Typeface::Ptr semi = Typeface::createSystemTypefaceFor (VoidFonts::JetBrainsMonoSemiBold_ttf,
                                                                       VoidFonts::JetBrainsMonoSemiBold_ttfSize);
        static Typeface::Ptr xb   = Typeface::createSystemTypefaceFor (VoidFonts::JetBrainsMonoExtraBold_ttf,
                                                                       VoidFonts::JetBrainsMonoExtraBold_ttfSize);
        return w == Weight::semibold ? semi : w == Weight::extrabold ? xb : reg;
    }

    // height of buttons / lists / stars: grows half as fast as the dials
    float rowH (float z) { return 22.0f * (1.0f + (z - 1.0f) * 0.5f); }

    const Font& labelFont() { static const Font f = mono (8.5f, Weight::regular, 0.16f); return f; }
    const Font& valueFont() { static const Font f = mono (10.0f); return f; }
    constexpr float labelH = 12.0f, valueH = 14.0f;

    void dashed (Graphics& g, const Path& p, std::initializer_list<float> dashes, float thickness)
    {
        Path out;
        std::vector<float> d (dashes);
        PathStrokeType (thickness).createDashedStroke (out, p, d.data(), (int) d.size());
        g.fillPath (out);
    }
}

// ================================================================================================ type
Font mono (float px, Weight w, float tracking)
{
    Font f (FontOptions (typefaceFor (w)).withPointHeight (px));
    if (tracking != 0.0f)
        f = f.withExtraKerningFactor (tracking * px / jmax (1.0f, f.getHeight()));
    return f;
}

float textWidth (const Font& f, const String& s) { return GlyphArrangement::getStringWidth (f, s); }

void drawMark (Graphics& g, Rectangle<float> r)
{
    const float s = jmin (r.getWidth(), r.getHeight()) / 40.0f;
    const auto c = r.getCentre();
    g.setColour (Col::accent);
    g.drawEllipse (c.x - 18.0f * s, c.y - 5.0f * s, 36.0f * s, 10.0f * s, 1.0f * s);   // disk, back half
    g.setColour (Col::ink);
    g.drawEllipse (c.x - 9.5f * s, c.y - 9.5f * s, 19.0f * s, 19.0f * s, 1.2f * s);   // photon ring
    g.setColour (Col::bg);
    g.fillEllipse (c.x - 8.0f * s, c.y - 8.0f * s, 16.0f * s, 16.0f * s);              // the hole
    Path front;
    front.addCentredArc (c.x, c.y, 18.0f * s, 5.0f * s, 0.0f, pi * 0.5f, pi * 1.5f, true);
    g.setColour (Col::accent);
    g.strokePath (front, PathStrokeType (1.0f * s));                                    // disk, front half
}

// ================================================================================================ look and feel
LookAndFeel::LookAndFeel()
{
    setColour (PopupMenu::backgroundColourId, Col::bg);
    setColour (PopupMenu::textColourId, Col::ink);
    setColour (PopupMenu::headerTextColourId, Col::accent);
    setColour (PopupMenu::highlightedBackgroundColourId, Col::ink);
    setColour (PopupMenu::highlightedTextColourId, Col::bg);
    setColour (AlertWindow::backgroundColourId, Col::bg);
    setColour (AlertWindow::textColourId, Col::ink);
    setColour (AlertWindow::outlineColourId, Col::ink);
    setColour (TextEditor::backgroundColourId, Col::bg);
    setColour (TextEditor::textColourId, Col::ink);
    setColour (TextEditor::outlineColourId, Col::line2);
    setColour (TextEditor::focusedOutlineColourId, Col::accent);
    setColour (TextEditor::highlightColourId, Col::accent.withAlpha (0.4f));
    setColour (TextEditor::highlightedTextColourId, Col::ink);
    setColour (CaretComponent::caretColourId, Col::accent);
    setColour (TextButton::buttonColourId, Col::bg);
    setColour (TextButton::buttonOnColourId, Col::ink);
    setColour (TextButton::textColourOffId, Col::ink);
    setColour (TextButton::textColourOnId, Col::bg);
    setColour (TooltipWindow::backgroundColourId, Col::bg);
    setColour (TooltipWindow::textColourId, Col::ink);
    setColour (TooltipWindow::outlineColourId, Col::line2);
    setColour (ResizableWindow::backgroundColourId, Col::bg);
}

Typeface::Ptr LookAndFeel::getTypefaceForFont (const Font& f)
{
    return typefaceFor (f.isBold() ? Weight::semibold : Weight::regular);
}

Font LookAndFeel::getPopupMenuFont() { return mono (12.0f); }

void LookAndFeel::drawPopupMenuBackground (Graphics& g, int w, int h)
{
    g.fillAll (Col::bg);
    g.setColour (Col::line2);
    g.drawRect (0, 0, w, h, 1);
}

void LookAndFeel::drawButtonBackground (Graphics& g, Button& b, const Colour&, bool over, bool down)
{
    const auto r = b.getLocalBounds().toFloat();
    g.setColour (down ? Col::ink : over ? Col::line : Col::bg);
    g.fillRect (r);
    g.setColour (Col::ink);
    g.drawRect (r, 1.0f);
}

Font LookAndFeel::getTextButtonFont (TextButton&, int) { return mono (11.0f, Weight::semibold, 0.2f); }
Font LookAndFeel::getAlertWindowTitleFont()            { return mono (13.0f, Weight::semibold, 0.2f); }
Font LookAndFeel::getAlertWindowMessageFont()          { return mono (12.0f); }
Font LookAndFeel::getAlertWindowFont()                 { return mono (12.0f); }

void LookAndFeel::drawAlertBox (Graphics& g, AlertWindow& w, const Rectangle<int>& textArea, TextLayout& layout)
{
    g.fillAll (Col::bg);
    g.setColour (Col::ink);
    g.drawRect (w.getLocalBounds(), 1);
    layout.draw (g, textArea.toFloat());
}

void LookAndFeel::fillTextEditorBackground (Graphics& g, int, int, TextEditor&) { g.fillAll (Col::bg); }

void LookAndFeel::drawTextEditorOutline (Graphics& g, int w, int h, TextEditor& e)
{
    g.setColour (e.hasKeyboardFocus (true) ? Col::accent : Col::line2);
    g.drawRect (0, 0, w, h, 1);
}

// ================================================================================================ session
Session::Session (AudioProcessor& p, std::function<ValueTree()> s) : proc (p), state (std::move (s))
{
    for (auto* q : proc.getParameters())
        if (auto* r = dynamic_cast<RangedAudioParameter*> (q))
            byId[r->paramID] = r;
}

Session::~Session()
{
    finishMoves();
    stopTimer();
}

RangedAudioParameter* Session::param (const String& id) const
{
    const auto it = byId.find (id);
    return it != byId.end() ? it->second : nullptr;
}

void Session::status (const String& bold, const String& rest)
{
    if (showStatus) showStatus (bold, rest);
}

void Session::hover (RangedAudioParameter* p)
{
    if (p == nullptr) { status (hintBold, hintRest); return; }
    const bool locked = isLocked (p->paramID);
    status (p->getName (64).toUpperCase(), p->getCurrentValueAsText() + (locked ? "  ·  FROZEN" : "")
                                               + "  ·  right-click = automation clip / " + (locked ? "unfreeze" : "freeze from RANDOM"));
}

void Session::touchForHost (RangedAudioParameter& p)
{
    p.beginChangeGesture();
    p.setValueNotifyingHost (p.getValue());
    p.endChangeGesture();
}

void Session::showParameterMenu (Component& c, RangedAudioParameter& p, std::function<void()> changed)
{
    // the host's own menu for this parameter (VST3 IContextMenu; FL, Cubase, Reaper, Bitwig...)
    PopupMenu hostItems;
    auto* editor = dynamic_cast<AudioProcessorEditor*> (&c);
    if (editor == nullptr) editor = c.findParentComponentOfClass<AudioProcessorEditor>();
    if (editor != nullptr)
        if (auto* ctx = editor->getHostContext())
            if (auto menu = ctx->getContextMenuForParameter (&p))
                hostItems = menu->getEquivalentPopupMenu();
    bool hostAutomation = false;
    for (PopupMenu::MenuItemIterator it (hostItems, true); it.next();)
        hostAutomation = hostAutomation || it.getItem().text.containsIgnoreCase ("automat");

    Component::SafePointer<Component> safe (&c);
    PopupMenu m;
    m.addSectionHeader (p.getName (64).toUpperCase());
    if (! hostAutomation)   // FL does not offer one for plugins: mark it as last tweaked and say where FL's command is
        m.addItem ("Create automation clip", [this, safe, &p]
        {
            if (safe == nullptr) return;
            touchForHost (p);
            status ("AUTOMATION", p.getName (64) + " is now FL's last tweaked control  ·  in FL: Tools > Last tweaked > "
                                  "Create automation clip");
        });
    if (hostItems.getNumItems() > 0)
    {
        m.addSeparator();
        for (PopupMenu::MenuItemIterator it (hostItems); it.next();) m.addItem (it.getItem());
    }
    m.addSeparator();
    const bool frozen = isLocked (p.paramID);
    m.addItem (frozen ? "Unfreeze (RANDOM may change it again)" : "Freeze (RANDOM leaves it alone)", true, frozen,
               [this, safe, &p, changed]
               {
                   if (safe == nullptr) return;
                   toggleLock (p.paramID);
                   if (changed) changed();
                   status (p.getName (64).toUpperCase(), isLocked (p.paramID) ? "FROZEN  ·  RANDOM leaves it alone"
                                                                             : "unfrozen  ·  RANDOM may change it again");
               });
    m.addItem ("Reset to default", [safe, &p]
    {
        if (safe == nullptr) return;
        p.beginChangeGesture();
        p.setValueNotifyingHost (p.getDefaultValue());
        p.endChangeGesture();
    });
    m.showMenuAsync (PopupMenu::Options().withMousePosition());
}

bool Session::isLocked (const String& id) const
{
    return StringArray::fromTokens (state().getProperty ("randomLocks").toString(), " ", "").contains (id);
}

void Session::toggleLock (const String& id)
{
    auto locks = StringArray::fromTokens (state().getProperty ("randomLocks").toString(), " ", "");
    if (locks.contains (id)) locks.removeString (id); else locks.add (id);
    locks.removeEmptyStrings();
    state().setProperty ("randomLocks", locks.joinIntoString (" "), nullptr);
}

void Session::randomize()
{
    finishMoves();
    std::vector<std::pair<RangedAudioParameter*, float>> snap, targets;
    for (auto* q : proc.getParameters())
    {
        auto* r = dynamic_cast<RangedAudioParameter*> (q);
        if (r == nullptr) continue;
        snap.push_back ({ r, r->getValue() });
        const auto it = rules.find (r->paramID);
        const Rule rule = it != rules.end() ? it->second : Rule();
        if (rule.skip || isLocked (r->paramID) || r->paramID.startsWith ("own")) continue;   // OWN SOUND stays put
        float to;
        if (! rule.pick.empty())
            to = r->convertTo0to1 (rule.pick[(size_t) rnd.nextInt ((int) rule.pick.size())]);
        else if (r->isDiscrete() && r->getNumSteps() == 2)
            to = rnd.nextFloat() < rule.onChance ? 1.0f : 0.0f;
        else
            to = rule.lo + rnd.nextFloat() * (rule.hi - rule.lo);
        targets.push_back ({ r, to });
    }
    undoStack.push_back (std::move (snap));
    if (undoStack.size() > 20) undoStack.erase (undoStack.begin());
    flashStart = Time::getMillisecondCounterHiRes();
    startMoves (targets, true);
    if (onRandomized) onRandomized();
    if (onUndoChanged) onUndoChanged();
    status ("RANDOM", "moved " + String ((int) targets.size()) + " settings  ·  output, ceiling and frozen controls stay put"
                          "  ·  the undo button brings the old sound back");
}

void Session::undo()
{
    if (undoStack.empty()) return;
    finishMoves();
    auto snap = std::move (undoStack.back());
    undoStack.pop_back();
    std::vector<std::pair<RangedAudioParameter*, float>> changed;
    for (auto& [p, v] : snap)
        if (! approximatelyEqual (p->getValue(), v)) changed.push_back ({ p, v });
    startMoves (changed, false);
    if (onUndoChanged) onUndoChanged();
    status ("UNDO", "back to the sound before RANDOM");
}

void Session::startMoves (const std::vector<std::pair<RangedAudioParameter*, float>>& targets, bool stagger)
{
    moves.clear();
    int i = 0;
    for (auto& [p, to] : targets)
    {
        p->beginChangeGesture();   // so FL records the whole sweep as automation
        moves.push_back ({ p, p->getValue(), to, stagger ? (double) ((i++ % 24) * 16) : 0.0 });
    }
    moveStart = Time::getMillisecondCounterHiRes();
    startTimerHz (60);
}

void Session::finishMoves()
{
    for (auto& m : moves)
    {
        if (! approximatelyEqual (m.p->getValue(), m.to)) m.p->setValueNotifyingHost (m.to);
        m.p->endChangeGesture();
    }
    moves.clear();
}

void Session::timerCallback()
{
    const double now = Time::getMillisecondCounterHiRes();
    bool running = false;
    for (auto& m : moves)
    {
        const double k = jlimit (0.0, 1.0, (now - moveStart - m.delay) / 500.0);
        const float e = 1.0f - (float) std::pow (1.0 - k, 3.0);
        const float v = m.from + (m.to - m.from) * e;
        if (! approximatelyEqual (m.p->getValue(), v)) m.p->setValueNotifyingHost (v);
        running = running || k < 1.0;
    }
    if (! running) finishMoves();
    if (onFlash && now - flashStart < 520.0) onFlash();
    if (! running && now - flashStart >= 520.0) stopTimer();
}

float Session::flash() const
{
    const double t = Time::getMillisecondCounterHiRes() - flashStart;
    return t >= 0.0 && t < 440.0 && ((int) (t / 110.0)) % 2 == 0 ? 1.0f : 0.0f;   // two red blinks
}

// ================================================================================================ controls
Control::Control (Session& s, const String& id, const String& l, bool lk)
    : param (*s.param (id)), session (s), label (l), linked (lk),
      attachment (param, [this] (float v)
                  {
                      value = param.convertTo0to1 (v);
                      repaint();
                      if (hovered) session.hover (&param);
                  }, nullptr)
{
    attachment.sendInitialUpdate();
}

Control::~Control()
{
    if (dragging) attachment.endGesture();
}

String Control::valueText() const { return param.getText (value, 64); }
bool Control::discrete() const { return param.isDiscrete(); }
int Control::steps() const { return jmax (1, param.getNumSteps() - 1); }
float Control::snap (float n) const { return discrete() ? std::round (n * (float) steps()) / (float) steps() : n; }

float Control::zeroNorm() const
{
    const auto& r = param.getNormalisableRange();
    return (r.start < 0.0f && r.end > 0.0f) ? r.convertTo0to1 (0.0f) : -1.0f;
}

float Control::labelWidth() const { return showsLabel() ? textWidth (labelFont(), label.toUpperCase()) : 0.0f; }

float Control::widestValue() const
{
    if (! showsValue()) return 0.0f;
    float w = 0.0f;
    if (discrete() && steps() <= 32)
        for (int i = 0; i <= steps(); ++i) w = jmax (w, textWidth (valueFont(), param.getText ((float) i / (float) steps(), 64)));
    else
        for (float n : { 0.0f, 0.5f, 1.0f, param.getDefaultValue() })
            w = jmax (w, textWidth (valueFont(), param.getText (n, 64)));
    return w;
}

void Control::setNorm (float n)
{
    attachment.setValueAsCompleteGesture (param.convertFrom0to1 (snap (jlimit (0.0f, 1.0f, n))));
}

void Control::paint (Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    drawGraphic (g, b.removeFromTop (graphicHeight()));
    const auto just = alignLeft() ? Justification::centredLeft : Justification::centred;
    if (showsLabel())
    {
        b.removeFromTop (2.0f);
        g.setColour (linked ? Col::accent : Col::label);
        g.setFont (labelFont());
        g.drawText (label.toUpperCase(), b.removeFromTop (labelH), just, false);
    }
    if (showsValue())
    {
        g.setColour (Col::ink);
        g.setFont (valueFont());
        g.drawText (valueText(), b.removeFromTop (valueH), just, false);
    }
    if (session.isLocked (param.paramID))
    {
        g.setColour (Col::accent);
        g.fillRect ((float) getWidth() - 7.0f, 0.0f, 5.0f, 5.0f);
    }
}

void Control::mouseEnter (const MouseEvent&)
{
    hovered = true;
    setMouseCursor (dragsVertically() ? MouseCursor::UpDownResizeCursor : MouseCursor::PointingHandCursor);
    session.hover (&param);
}

void Control::mouseExit (const MouseEvent&)
{
    hovered = false;
    session.hover (nullptr);
}

void Control::mouseDown (const MouseEvent& e)
{
    if (e.mods.isPopupMenu())
    {
        SafePointer<Control> safe (this);
        session.showParameterMenu (*this, param, [safe] { if (safe != nullptr) safe->repaint(); });
        return;
    }
    if (dragsVertically())
    {
        dragging = true;
        dragValue = value;
        lastY = e.position.y;
        attachment.beginGesture();
    }
}

void Control::mouseDrag (const MouseEvent& e)
{
    if (! dragging) return;
    const bool fine = e.mods.isShiftDown() || e.mods.isCtrlDown() || e.mods.isCommandDown();
    dragValue = jlimit (0.0f, 1.0f, dragValue + (lastY - e.position.y) / (dragRange() * (fine ? 5.0f : 1.0f)));
    lastY = e.position.y;
    attachment.setValueAsPartOfGesture (param.convertFrom0to1 (snap (dragValue)));
}

void Control::mouseUp (const MouseEvent& e)
{
    if (dragging)
    {
        dragging = false;
        attachment.endGesture();
        return;
    }
    if (! e.mods.isPopupMenu() && e.mouseWasClicked() && ! dragsVertically())
        clickAt (e.position);
}

void Control::mouseDoubleClick (const MouseEvent&)
{
    if (dragsVertically()) setNorm (param.getDefaultValue());
}

void Control::mouseWheelMove (const MouseEvent& e, const MouseWheelDetails& w)
{
    const float dir = w.deltaY > 0.0f ? 1.0f : w.deltaY < 0.0f ? -1.0f : 0.0f;
    if (dir == 0.0f) return;
    const float step = discrete() ? 1.0f / (float) steps() : (e.mods.isShiftDown() ? 0.005f : 0.02f);
    setNorm (value + dir * step);
}

// ---------------------------------------------------------------- knob
Point<float> Knob::sizeAt (float s) const
{
    const float w = jmax ((big ? 86.0f : 62.0f) * s, labelWidth() + 4.0f, widestValue() + 4.0f);
    return { w, (big ? 76.0f : 52.0f) * s + 2.0f + labelH + valueH };
}

void Knob::drawGraphic (Graphics& g, Rectangle<float> a)
{
    const float d = (big ? 76.0f : 52.0f) * z, s = d / 100.0f;
    const Point<float> c (a.getCentreX(), a.getY() + d * 0.5f);
    const float A0 = pi * 0.75f, SW = pi * 1.5f;
    auto at = [&] (float r, float ang) { return Point<float> (c.x + r * s * std::cos (ang), c.y + r * s * std::sin (ang)); };

    constexpr int N = 36;
    const int lit = roundToInt (value * (float) (N - 1));
    const float zn = zeroNorm();
    const int zi = zn >= 0.0f ? roundToInt (zn * (float) (N - 1)) : 0;
    for (int i = 0; i < N; ++i)
    {
        const float ang = A0 + SW * (float) i / (float) (N - 1);
        g.setColour (i >= jmin (zi, lit) && i <= jmax (zi, lit) ? Col::ink : Col::dim);
        g.drawLine ({ at (37.0f, ang), at (i % 5 ? 43.0f : 47.0f, ang) }, 1.6f * s);
    }
    g.setColour (Col::bg);
    g.fillEllipse (c.x - 29.0f * s, c.y - 29.0f * s, 58.0f * s, 58.0f * s);
    g.setColour (Col::ink);
    g.drawEllipse (c.x - 29.0f * s, c.y - 29.0f * s, 58.0f * s, 58.0f * s, 1.2f * s);

    const float ang = A0 + SW * value, w = 0.24f;
    Path beam;
    beam.addTriangle (c, at (29.0f, ang - w), at (29.0f, ang + w));
    g.setColour (Colours::white.withAlpha (0.14f));
    g.fillPath (beam);
    g.setColour (Col::ink);
    g.drawLine ({ c, at (27.0f, ang) }, 2.2f * s);
    g.setColour (Col::accent);
    g.fillRect (Rectangle<float> (6.0f * s, 6.0f * s).withCentre (c));
}

// ---------------------------------------------------------------- planet
Point<float> Planet::sizeAt (float s) const
{
    return { jmax (66.0f * s, labelWidth() + 4.0f, widestValue() + 4.0f), 44.0f * s + 8.0f + 2.0f + labelH + valueH };
}

void Planet::drawGraphic (Graphics& g, Rectangle<float> a)
{
    const float s = 62.0f * z / 100.0f;
    const Point<float> c (a.getCentreX(), a.getY() + 4.0f + 35.0f * s);
    Path ring;
    ring.addEllipse (c.x - 10.0f * s, c.y - 10.0f * s, 20.0f * s, 20.0f * s);
    g.setColour (Col::accent);
    dashed (g, ring, { 1.5f * s, 2.5f * s }, 1.0f * s);
    g.setColour (Col::ink);
    g.fillEllipse (c.x - 5.0f * s, c.y - 5.0f * s, 10.0f * s, 10.0f * s);

    const auto tilt = AffineTransform::rotation (-16.0f * pi / 180.0f, c.x, c.y);
    auto pt = [&] (float ang) { return Point<float> (c.x + 44.0f * s * std::cos (ang), c.y + 17.0f * s * std::sin (ang)).transformedBy (tilt); };
    Path orbit;
    orbit.addEllipse (c.x - 44.0f * s, c.y - 17.0f * s, 88.0f * s, 34.0f * s);
    orbit.applyTransform (tilt);
    g.setColour (Col::dim);
    g.strokePath (orbit, PathStrokeType (1.2f * s));

    auto ang = [] (float n) { return pi * 0.5f + n * twoPi * 0.97f; };
    const float zn = zeroNorm(), a0 = ang (zn >= 0.0f ? zn : 0.0f), a1 = ang (value);
    Path trail;
    for (int i = 0; i <= 40; ++i)
    {
        const auto p = pt (a0 + (a1 - a0) * (float) i / 40.0f);
        if (i == 0) trail.startNewSubPath (p); else trail.lineTo (p);
    }
    g.setColour (Col::ink);
    g.strokePath (trail, PathStrokeType (2.0f * s, PathStrokeType::curved, PathStrokeType::rounded));
    const auto pl = pt (a1);
    g.setColour (Col::accent);
    g.fillEllipse (pl.x - 5.5f * s, pl.y - 5.5f * s, 11.0f * s, 11.0f * s);
    g.setColour (Col::ink);
    g.drawEllipse (pl.x - 5.5f * s, pl.y - 5.5f * s, 11.0f * s, 11.0f * s, 1.2f * s);
}

// ---------------------------------------------------------------- fader
Point<float> Fader::sizeAt (float s) const
{
    return { jmax (40.0f, 30.0f * s, labelWidth(), widestValue()) + 6.0f, track * s + 2.0f + labelH + valueH };
}

void Fader::drawGraphic (Graphics& g, Rectangle<float> a)
{
    const float left = a.getCentreX() - 15.0f * z, top = a.getY() + 5.0f * z, bot = a.getBottom() - 5.0f * z, len = bot - top;
    const float X = left + 19.0f * z;
    for (int i = 0; i <= 10; ++i)
    {
        const float y = bot - len * (float) i / 10.0f;
        g.setColour (i % 5 ? Col::dim : Col::mute);
        g.drawLine (left + (i % 5 ? 6.0f : 2.0f) * z, y, left + 11.0f * z, y, 1.0f);
    }
    g.setColour (Col::line2);
    g.drawLine (X, top, X, bot, 1.0f);
    const float zn = zeroNorm(), y0 = bot - len * (zn >= 0.0f ? zn : 0.0f), y = bot - len * value;
    g.setColour (Col::accent);
    g.drawLine (X, y0, X, y, 3.0f * z);
    g.setColour (Col::ink);
    g.fillRect (X - 8.0f * z, y - 2.0f * z, 16.0f * z, 4.0f * z);
}

// ---------------------------------------------------------------- moon
Point<float> Moon::sizeAt (float s) const
{
    return { jmax (56.0f * s, labelWidth() + 4.0f, widestValue() + 4.0f), 48.0f * s + 4.0f + 2.0f + labelH + valueH };
}

void Moon::drawGraphic (Graphics& g, Rectangle<float> a)
{
    const float d = 48.0f * z, s = d / 100.0f, R = 30.0f * s;
    const Point<float> c (a.getCentreX(), a.getY() + 2.0f + d * 0.5f);
    g.setColour (Col::bg);
    g.fillEllipse (c.x - R, c.y - R, 2 * R, 2 * R);
    g.setColour (Col::dim);
    g.drawEllipse (c.x - R, c.y - R, 2 * R, 2 * R, 1.4f * s);
    const float n = value;
    if (n > 0.997f)
    {
        g.setColour (Col::ink);
        g.fillEllipse (c.x - R, c.y - R, 2 * R, 2 * R);
    }
    else if (n > 0.003f)
    {
        Path lit;   // lit right half, closed by the terminator (an ellipse bulging right = crescent, left = gibbous)
        lit.addCentredArc (c.x, c.y, R, R, 0.0f, 0.0f, pi, true);
        const float rx = R * std::abs (1.0f - 2.0f * n);
        lit.addCentredArc (c.x, c.y, rx, R, 0.0f, pi, n > 0.5f ? twoPi : 0.0f, false);
        lit.closeSubPath();
        g.setColour (Col::ink);
        g.fillPath (lit);
    }
    const float ro = 43.0f * s;
    g.setColour (Col::line2);
    g.drawEllipse (c.x - ro, c.y - ro, 2 * ro, 2 * ro, 1.0f);
    if (n > 0.003f)
    {
        Path arc;
        arc.addCentredArc (c.x, c.y, ro, ro, 0.0f, 0.0f, jmin (n, 0.9995f) * twoPi, true);
        g.setColour (Col::accent);
        g.strokePath (arc, PathStrokeType (3.0f * s));
    }
}

// ---------------------------------------------------------------- segmented buttons
Buttons::Buttons (Session& s, const String& id, const String& l, StringArray shortNames, int columns, bool lk)
    : Control (s, id, l, lk), names (std::move (shortNames)), cols (columns)
{
    if (names.isEmpty())
    {
        if (auto* c = dynamic_cast<AudioParameterChoice*> (&param)) names = c->choices;
        else for (int i = 0; i <= steps(); ++i) names.add (param.getText ((float) i / (float) steps(), 16));
    }
    if (cols <= 0) cols = names.size();
}

std::vector<float> Buttons::columnWidths() const
{
    const auto f = mono (9.5f, Weight::regular, 0.06f);
    std::vector<float> w ((size_t) cols, 26.0f);
    for (int i = 0; i < names.size(); ++i)
        w[(size_t) (i % cols)] = jmax (w[(size_t) (i % cols)], textWidth (f, names[i]) + 12.0f);
    return w;
}

float Buttons::graphicHeight() const
{
    const int rows = (names.size() + cols - 1) / cols;
    return (float) rows * rowH (z) + 1.0f;
}

Point<float> Buttons::sizeAt (float s) const
{
    float gw = 1.0f;
    for (auto w : columnWidths()) gw += w - 1.0f;
    const int rows = (names.size() + cols - 1) / cols;
    return { jmax (gw, labelWidth(), widestValue()), (float) rows * rowH (s) + 1.0f + 2.0f + labelH + valueH };
}

Rectangle<float> Buttons::cell (int i, Rectangle<float> area) const
{
    const auto w = columnWidths();
    float x = area.getX();
    for (int c = 0; c < i % cols; ++c) x += w[(size_t) c] - 1.0f;
    const float h = rowH (z);
    return { x, area.getY() + (float) (i / cols) * (h - 1.0f), w[(size_t) (i % cols)], h };
}

void Buttons::drawGraphic (Graphics& g, Rectangle<float> a)
{
    const int sel = roundToInt (value * (float) steps());
    g.setFont (mono (9.5f, Weight::regular, 0.06f));
    for (int pass = 0; pass < 2; ++pass)   // the selected cell last, so its white border sits on top
        for (int i = 0; i < names.size(); ++i)
        {
            if ((i == sel) != (pass == 1)) continue;
            const auto r = cell (i, a);
            g.setColour (i == sel ? Col::ink : Col::bg);
            g.fillRect (r);
            g.setColour (i == sel ? Col::ink : Col::line2);
            g.drawRect (r, 1.0f);
            g.setColour (i == sel ? Col::bg : Col::label);
            g.drawText (names[i], r, Justification::centred, false);
        }
}

void Buttons::clickAt (Point<float> p)
{
    const auto area = getLocalBounds().toFloat().withHeight (graphicHeight());
    for (int i = 0; i < names.size(); ++i)
        if (cell (i, area).contains (p)) { setNorm ((float) i / (float) steps()); return; }
}

// ---------------------------------------------------------------- list
float ListPicker::graphicHeight() const { return rowH (z); }

Point<float> ListPicker::sizeAt (float s) const { return { jmax (boxWidth, labelWidth()), rowH (s) + 2.0f + labelH }; }

void ListPicker::drawGraphic (Graphics& g, Rectangle<float> a)
{
    auto r = a.withWidth (boxWidth).reduced (0.5f);
    g.setColour (Col::line2);
    g.drawRect (r, 1.0f);
    g.setFont (mono (9.0f));
    g.setColour (Col::label);
    g.drawText (String::fromUTF8 ("\xe2\x97\x82"), r.removeFromLeft (20.0f), Justification::centred, false);
    g.drawText (String::fromUTF8 ("\xe2\x96\xb8"), r.removeFromRight (20.0f), Justification::centred, false);
    g.setColour (Col::ink);
    g.setFont (valueFont());
    g.drawText (valueText().toUpperCase(), r, Justification::centred, true);
}

void ListPicker::clickAt (Point<float> p)
{
    const int n = steps() + 1, cur = roundToInt (value * (float) steps());
    if (p.x < 20.0f)            { setNorm ((float) ((cur - 1 + n) % n) / (float) steps()); return; }
    if (p.x > boxWidth - 20.0f) { setNorm ((float) ((cur + 1) % n) / (float) steps()); return; }
    PopupMenu m;   // click the name: the whole list
    for (int i = 0; i < n; ++i) m.addItem (i + 1, param.getText ((float) i / (float) steps(), 64), true, i == cur);
    SafePointer<ListPicker> safe (this);
    m.showMenuAsync (PopupMenu::Options().withTargetComponent (this), [safe, n] (int r)
    {
        if (safe != nullptr && r > 0 && r <= n) safe->setNorm ((float) (r - 1) / (float) safe->steps());
    });
}

// ---------------------------------------------------------------- star switch
float Star::graphicHeight() const { return rowH (z); }

Point<float> Star::sizeAt (float s) const
{
    return { 8.0f + 12.0f + 6.0f + textWidth (mono (9.5f, Weight::regular, 0.14f), label.toUpperCase()) + 10.0f, rowH (s) };
}

void Star::drawGraphic (Graphics& g, Rectangle<float> a)
{
    const bool on = value > 0.5f;
    const auto r = a.reduced (0.5f);
    g.setColour (on ? Col::ink : Col::line2);
    g.drawRect (r, 1.0f);
    const auto c = Point<float> (r.getX() + 14.0f, r.getCentreY());
    Path star;
    const float k = 6.0f, q = 1.8f;
    star.startNewSubPath (c.x, c.y - k);
    star.lineTo (c.x + q, c.y - q); star.lineTo (c.x + k, c.y); star.lineTo (c.x + q, c.y + q);
    star.lineTo (c.x, c.y + k);     star.lineTo (c.x - q, c.y + q); star.lineTo (c.x - k, c.y); star.lineTo (c.x - q, c.y - q);
    star.closeSubPath();
    if (on) { g.setColour (Col::ink); g.fillPath (star); }
    else    { g.setColour (Col::mute); g.strokePath (star, PathStrokeType (1.0f)); }
    g.setColour (on ? Col::ink : Col::label);
    g.setFont (mono (9.5f, Weight::regular, 0.14f));
    g.drawText (label.toUpperCase(), r.withTrimmedLeft (26.0f), Justification::centredLeft, false);
}

// ---------------------------------------------------------------- push button
PushButton::PushButton (const String& t) : text (t) { setMouseCursor (MouseCursor::PointingHandCursor); }

Point<float> PushButton::sizeAt (float s) const
{
    return { textWidth (mono (9.5f, Weight::regular, 0.16f), text) + 22.0f, rowH (s) };
}

void PushButton::paint (Graphics& g)
{
    const auto r = getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (isMouseOver() ? Col::line : Col::bg);
    g.fillRect (r);
    g.setColour (Col::ink);
    g.drawRect (r, 1.0f);
    g.setFont (mono (9.5f, Weight::regular, 0.16f));
    g.drawText (text, r, Justification::centred, false);
}

void PushButton::mouseUp (const MouseEvent& e)
{
    if (e.mouseWasClicked() && onClick) onClick();
}

// ---------------------------------------------------------------- reduction meter
ReductionMeter::ReductionMeter (std::function<float()> f, float w, float h) : Display (w, h), db (std::move (f))
{
    setInterceptsMouseClicks (false, false);
    startTimerHz (30);
}

void ReductionMeter::timerCallback()
{
    if (frozen || ! isShowing()) return;
    const float v = jlimit (-24.0f, 0.0f, db ? db() : 0.0f);
    shown = v < shown ? v : shown + (v - shown) * 0.25f;   // fast down, slow back
    repaint();
}

void ReductionMeter::paint (Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    g.fillAll (Col::bg);
    g.setColour (Col::line);
    g.drawRect (b, 1.0f);
    const int n = 12, lit = roundToInt (-shown / 24.0f * (float) n);
    const float sw = (b.getWidth() - 16.0f) / (float) n;
    for (int i = 0; i < n; ++i)
    {
        g.setColour (i < lit ? Col::accent : Col::dim);
        g.fillRect (8.0f + (float) i * sw, 10.0f, sw - 2.0f, 10.0f);
    }
    g.setFont (mono (8.5f));
    g.setColour (Col::mute);
    g.drawText ("0", Rectangle<float> (8.0f, 22.0f, 30.0f, 11.0f), Justification::centredLeft, false);
    g.drawText ("-24 dB", Rectangle<float> (b.getWidth() - 58.0f, 22.0f, 50.0f, 11.0f), Justification::centredRight, false);
    g.setColour (Col::ink);
    g.setFont (mono (12.0f, Weight::semibold));
    g.drawText (String (shown, 1) + " dB", Rectangle<float> (8.0f, b.getHeight() - 24.0f, b.getWidth() - 16.0f, 18.0f),
                Justification::centredLeft, false);
}

// ---------------------------------------------------------------- stack
Stack::Stack (std::initializer_list<Component*> list, bool c) : items (list), centred (c)
{
    for (auto* i : items) addAndMakeVisible (i);
}

Point<float> Stack::sizeAt (float s) const
{
    float w = 0.0f, h = 0.0f;
    for (auto* i : items)
        if (auto* sz = dynamic_cast<Sized*> (i))
        {
            const auto p = sz->sizeAt (s);
            w = jmax (w, p.x);
            h += p.y;
        }
    return { w, h + 7.0f * (float) jmax (0, (int) items.size() - 1) };
}

void Stack::setScale (float s)
{
    z = s;
    for (auto* i : items) if (auto* sz = dynamic_cast<Sized*> (i)) sz->setScale (s);
}

void Stack::resized()
{
    float y = 0.0f;
    for (auto* i : items)
        if (auto* sz = dynamic_cast<Sized*> (i))
        {
            const auto p = sz->sizeAt (z);
            const float x = centred ? ((float) getWidth() - p.x) * 0.5f : 0.0f;
            i->setBounds (Rectangle<float> (x, y, p.x, p.y).getSmallestIntegerContainer());
            y += p.y + 7.0f;
        }
}

// ---------------------------------------------------------------- group
namespace { constexpr float titleH = 17.0f, titleGap = 7.0f; const Font& titleFont() { static const Font f = mono (9.0f, Weight::regular, 0.26f); return f; } }

Group::Group (const String& t, std::initializer_list<Component*> list) : Group (t, std::vector<Component*> (list)) {}

Group::Group (const String& t, std::vector<Component*> list) : title (t), items (std::move (list))
{
    for (auto* i : items) addAndMakeVisible (i);
    setInterceptsMouseClicks (false, true);
}

Point<float> Group::sizeAt (float s) const
{
    float w = 0.0f, h = 0.0f;
    for (auto* i : items)
        if (auto* sz = dynamic_cast<Sized*> (i))
        {
            const auto p = sz->sizeAt (s);
            w += p.x;
            h = jmax (h, p.y);
        }
    w += 4.0f * (float) jmax (0, (int) items.size() - 1);
    return { jmax (w, textWidth (titleFont(), title) + 4.0f), titleH + titleGap + h };
}

void Group::setScale (float s)
{
    z = s;
    for (auto* i : items) if (auto* sz = dynamic_cast<Sized*> (i)) sz->setScale (s);
}

void Group::resized()
{
    // one row, bottom-aligned, spread evenly (space-evenly) when the group is wider than its controls
    float total = 0.0f, rowHgt = 0.0f;
    std::vector<Point<float>> sizes;
    for (auto* i : items)
    {
        const auto p = dynamic_cast<Sized*> (i) != nullptr ? dynamic_cast<Sized*> (i)->sizeAt (z) : Point<float>();
        sizes.push_back (p);
        total += p.x;
        rowHgt = jmax (rowHgt, p.y);
    }
    const float n = (float) items.size();
    const float gap = jmax (0.0f, ((float) getWidth() - total - 4.0f * (n - 1.0f)) / (n + 1.0f));
    float x = gap;
    const float bottom = titleH + titleGap + rowHgt;
    for (size_t k = 0; k < items.size(); ++k)
    {
        items[k]->setBounds (Rectangle<float> (x, bottom - sizes[k].y, sizes[k].x, sizes[k].y).getSmallestIntegerContainer());
        x += sizes[k].x + 4.0f + gap;
    }
}

void Group::paint (Graphics& g)
{
    g.setColour (Col::mute);
    g.setFont (titleFont());
    g.drawText (title, Rectangle<float> (0.0f, 0.0f, (float) getWidth(), 12.0f), Justification::centredLeft, false);
    g.setColour (Col::line);
    g.fillRect (0.0f, titleH - 1.0f, (float) getWidth(), 1.0f);
}

void Group::refreshActive()
{
    setAlpha (active && ! active() ? 0.3f : 1.0f);
}

// ---------------------------------------------------------------- panel
namespace
{
    // every way of cutting the groups (in order) into lines; keep the ones that fit, prefer evenly filled lines
    std::vector<std::vector<int>> bestLines (const std::vector<float>& ws, const std::vector<float>& hs, float W, float H, float G)
    {
        const int n = (int) ws.size();
        std::vector<std::vector<int>> best;
        double bestScore = 1.0e300;
        for (int m = 0; m < (1 << jmax (0, n - 1)); ++m)
        {
            std::vector<std::vector<int>> lines { { 0 } };
            for (int i = 1; i < n; ++i)
            {
                if (m & (1 << (i - 1))) lines.push_back ({ i });
                else lines.back().push_back (i);
            }
            bool ok = true;
            float tot = G * (float) (lines.size() - 1);
            double score = 0.0;
            for (auto& L : lines)
            {
                float w = G * (float) (L.size() - 1), h = 0.0f;
                for (int i : L) { w += ws[(size_t) i]; h = jmax (h, hs[(size_t) i]); }
                if (w > W) { ok = false; break; }
                tot += h;
                score += (double) (W - w) * (W - w);
            }
            if (! ok || tot > H) continue;
            score /= (double) lines.size();
            if (score < bestScore) { bestScore = score; best = lines; }
        }
        return best;
    }
    constexpr float tabH = 24.0f, tabGap = 12.0f;
}

void Panel::addGroup (Group* g, int page)
{
    groups.push_back ({ g, page });
    addAndMakeVisible (g);
}

void Panel::setPages (const StringArray& names)
{
    pages = names;
    showPage (0);
}

void Panel::addPage (Group* g, const String& name)
{
    if (pages.isEmpty()) pages.add ("SOUND");
    pages.add (name);
    addGroup (g, pages.size() - 1);
    showPage (current);
}

Rectangle<float> Panel::tabRect (int i) const
{
    const auto f = mono (9.5f, Weight::regular, 0.22f);
    float x = 0.0f;
    for (int k = 0; k < i; ++k) x += textWidth (f, pages[k]) + 28.0f - 1.0f;
    return { x, 0.0f, textWidth (f, pages[i]) + 28.0f, tabH };
}

void Panel::resized()
{
    auto area = getLocalBounds();
    if (pages.size() > 1) area.removeFromTop ((int) (tabH + tabGap));
    const int count = jmax (1, pages.size());
    pageScale.assign ((size_t) count, 1.0f);
    for (int p = 0; p < count; ++p) fit (p, area);
    showPage (current);
}

void Panel::fit (int page, Rectangle<int> area)
{
    std::vector<Group*> gs;
    for (auto& [g, p] : groups) if (p == page) gs.push_back (g);
    if (gs.empty()) return;
    const float W = (float) area.getWidth(), H = (float) area.getHeight(), G = 16.0f;

    std::vector<std::vector<int>> plan;
    std::vector<float> ws, hs;
    float z = 1.0f;
    for (int k = 0; k <= 14 && plan.empty(); ++k)
    {
        z = 1.5f - 0.05f * (float) k;   // the biggest controls that still leave some air
        ws.clear(); hs.clear();
        for (auto* g : gs) { const auto s = g->sizeAt (z); ws.push_back (s.x); hs.push_back (s.y); }
        plan = bestLines (ws, hs, W, H * (z > 1.001f ? 0.9f : 0.98f), G);
    }
    if (plan.empty())   // never expected: one group per line at the smallest size
        for (int i = 0; i < (int) gs.size(); ++i) plan.push_back ({ i });
    pageScale[(size_t) page] = z;
    for (auto* g : gs) g->setScale (z);

    float total = 0.0f;
    for (auto& L : plan) { float h = 0.0f; for (int i : L) h = jmax (h, hs[(size_t) i]); total += h; }
    const float gapV = jmax (0.0f, (H - total) / (float) (plan.size() + 1));
    float y = (float) area.getY() + gapV;
    for (auto& L : plan)
    {
        float sumW = 0.0f, lineH = 0.0f;
        for (int i : L) { sumW += ws[(size_t) i]; lineH = jmax (lineH, hs[(size_t) i]); }
        const float free = jmax (0.0f, W - sumW - G * (float) (L.size() - 1));
        float x = (float) area.getX();
        for (int i : L)
        {
            const float w = ws[(size_t) i] + free * ws[(size_t) i] / jmax (1.0f, sumW);   // grow in proportion to size
            gs[(size_t) i]->setBounds (Rectangle<float> (x, y, w, hs[(size_t) i]).getSmallestIntegerContainer());
            x += w + G;
        }
        y += lineH + gapV;
    }
}

void Panel::showPage (int p)
{
    current = jlimit (0, jmax (0, pages.size() - 1), p);
    for (auto& [g, page] : groups) g->setVisible (pages.size() <= 1 || page == current);
    repaint();
}

void Panel::paint (Graphics& g)
{
    if (pages.size() <= 1) return;
    g.setFont (mono (9.5f, Weight::regular, 0.22f));
    for (int pass = 0; pass < 2; ++pass)
        for (int i = 0; i < pages.size(); ++i)
        {
            if ((i == current) != (pass == 1)) continue;
            const auto r = tabRect (i);
            g.setColour (i == current ? Col::ink : Col::bg);
            g.fillRect (r);
            g.setColour (i == current ? Col::ink : Col::line2);
            g.drawRect (r, 1.0f);
            g.setColour (i == current ? Col::bg : Col::label);
            g.drawText (pages[i], r, Justification::centred, false);
        }
}

void Panel::mouseUp (const MouseEvent& e)
{
    for (int i = 0; i < pages.size(); ++i)
        if (tabRect (i).contains (e.position)) { showPage (i); return; }
}

void Panel::refreshActive()
{
    for (auto& [g, p] : groups) g->refreshActive();
}

// ================================================================================================ stage
static bool frozenAnimation = false;
void Stage::freezeAnimation (bool f) { frozenAnimation = f; }

Stage::Stage (Session& s, const String& h, const String& w) : session (s), hint (h), how (w)
{
    setOpaque (true);
    setMouseCursor (MouseCursor::CrosshairCursor);
    updateTimer();
}

Stage::~Stage() { end(); }

void Stage::updateTimer()
{
    if (frozenAnimation) stopTimer();
    else if (! isTimerRunning()) startTimerHz (30);   // ~30 fps; repaint() does nothing while the window is hidden
}

void Stage::paint (Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    g.fillAll (Col::bg);
    {
        Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (getLocalBounds());
        drawStage (g, b, frozenAnimation ? 2600.0 : Time::getMillisecondCounterHiRes());
    }
    g.setFont (mono (9.5f));
    g.setColour (Col::mute);
    g.drawFittedText (how, Rectangle<int> (10, 7, roundToInt (b.getWidth() * 0.58f), 45), Justification::topLeft, 3, 1.0f);
    g.setFont (mono (9.5f, Weight::regular, 0.18f));
    g.setColour (Col::accent);
    g.drawText (hint.toUpperCase(), Rectangle<float> (10.0f, 7.0f, b.getWidth() - 20.0f, 13.0f), Justification::topRight, false);

    const auto lines = readout();
    if (! lines.isEmpty())
    {
        const auto f = mono (10.0f);
        float w = 0.0f;
        for (auto& l : lines) w = jmax (w, textWidth (f, l));
        const float lh = 16.0f, bh = lh * (float) lines.size() + 12.0f;
        auto box = Rectangle<float> (1.0f, b.getHeight() - 1.0f - bh, w + 18.0f, bh);
        g.setColour (Col::bg);
        g.fillRect (box);
        g.setColour (Col::ink);
        g.setFont (f);
        for (int i = 0; i < lines.size(); ++i)
            g.drawText (lines[i], Rectangle<float> (box.getX() + 9.0f, box.getY() + 6.0f + lh * (float) i, w + 4.0f, lh),
                        Justification::centredLeft, false);
    }
    g.setColour (Col::line2);
    g.drawRect (getLocalBounds(), 1);
}

float Stage::get (const String& id) const
{
    auto* q = p (id);
    return q != nullptr ? q->getValue() : 0.0f;
}

String Stage::text (const String& id) const
{
    auto* q = p (id);
    return q != nullptr ? q->getCurrentValueAsText() : String();
}

void Stage::begin (std::initializer_list<const char*> ids)
{
    end();
    for (auto* id : ids)
        if (auto* q = p (id)) { q->beginChangeGesture(); held.push_back (q); }
}

void Stage::set (const String& id, float n)
{
    auto* q = p (id);
    if (q == nullptr) return;
    n = jlimit (0.0f, 1.0f, n);
    if (q->isDiscrete())
    {
        const float st = (float) jmax (1, q->getNumSteps() - 1);
        n = std::round (n * st) / st;
    }
    if (! approximatelyEqual (q->getValue(), n)) q->setValueNotifyingHost (n);
}

void Stage::end()
{
    for (auto* q : held) q->endChangeGesture();
    held.clear();
}

void Stage::nudge (const String& id, float delta, bool fine)
{
    auto* q = p (id);
    if (q == nullptr || delta == 0.0f) return;
    const float step = q->isDiscrete() ? 1.0f / (float) jmax (1, q->getNumSteps() - 1) : (fine ? 0.005f : 0.02f);
    q->beginChangeGesture();
    q->setValueNotifyingHost (jlimit (0.0f, 1.0f, q->getValue() + (delta > 0.0f ? step : -step)));
    q->endChangeGesture();
}

std::vector<Stage::StarDot> Stage::makeStars (int count, int seed)
{
    Random r (seed);
    std::vector<StarDot> v;
    for (int i = 0; i < count; ++i) v.push_back ({ r.nextFloat(), r.nextFloat(), r.nextFloat() });
    return v;
}

void Stage::drawStars (Graphics& g, Rectangle<float> a, const std::vector<StarDot>& stars, float alpha)
{
    for (auto& s : stars)
    {
        g.setColour (Colours::white.withAlpha ((0.1f + s.b * 0.55f) * alpha));
        const float z = s.b > 0.94f ? 2.0f : 1.0f;
        g.fillRect (a.getX() + s.x * a.getWidth(), a.getY() + s.y * a.getHeight(), z, z);
    }
}

// ================================================================================================ orbit stage
OrbitStage::OrbitStage (Session& s, const String& h, const String& w, std::vector<Ring> r)
    : Stage (s, h, w), rings (std::move (r)), stars (makeStars (220, 11))
{
}

Point<float> OrbitStage::toPlane (Point<float> p) const
{
    const Point<float> c ((float) getWidth() * 0.5f, (float) getHeight() * 0.52f);
    return (p - c).transformedBy (AffineTransform::rotation (-tilt));
}

void OrbitStage::mouseDown (const MouseEvent& e)
{
    const auto q = toPlane (e.position);
    const float W = (float) getWidth();
    float best = 1.0e9f;
    for (int i = 0; i < (int) rings.size(); ++i)
    {
        const auto& r = rings[(size_t) i];
        const float d = std::abs (std::hypot (q.x / (r.rx * W), q.y / (r.ry * W)) - 1.0f);
        if (d < best) { best = d; active = i; }
    }
    if (active < 0) return;
    begin ({});
    if (auto* prm = p (rings[(size_t) active].id)) prm->beginChangeGesture();
    dragTo (e.position);
}

void OrbitStage::mouseUp (const MouseEvent&)
{
    if (active >= 0)
        if (auto* prm = p (rings[(size_t) active].id)) prm->endChangeGesture();
    active = -1;
}

void OrbitStage::mouseDrag (const MouseEvent& e) { dragTo (e.position); }

void OrbitStage::dragTo (Point<float> pos)
{
    if (active < 0) return;
    const auto q = toPlane (pos);
    const auto& r = rings[(size_t) active];
    const float W = (float) getWidth();
    float a = std::atan2 (q.y / (r.ry * W), q.x / (r.rx * W)) + MathConstants<float>::halfPi;
    if (a < 0.0f) a += MathConstants<float>::twoPi;
    set (r.id, a / (MathConstants<float>::twoPi * 0.97f));
}

void OrbitStage::drawStage (Graphics& g, Rectangle<float> area, double t)
{
    const float w = area.getWidth(), h = area.getHeight();
    const Point<float> c (w * 0.5f, h * 0.52f);
    tilt = -0.24f + (float) std::sin (t / 2600.0) * 0.025f;
    drawStars (g, area, stars);
    drawBehind (g, area, t);

    const float k = pulse (t);
    g.setColour (Col::ink);
    g.fillEllipse (Rectangle<float> (12.0f + 10.0f * k, 12.0f + 10.0f * k).withCentre (c));
    {
        Path halo, d;
        halo.addEllipse (Rectangle<float> (24.0f + 20.0f * k, 24.0f + 20.0f * k).withCentre (c));
        const float dash[] = { 1.0f, 2.5f };
        PathStrokeType (1.0f).createDashedStroke (d, halo, dash, 2);
        g.setColour (Col::accent);
        g.fillPath (d);
    }
    drawSunExtras (g, c, t);

    Graphics::ScopedSaveState ss (g);
    g.addTransform (AffineTransform::rotation (tilt).translated (c));
    for (int i = 0; i < (int) rings.size(); ++i)
    {
        const auto& r = rings[(size_t) i];
        const float rx = r.rx * w, ry = r.ry * w, n = get (r.id), a = ringAngle (n);
        const bool hol = hollow (i);
        Path orbit, d;
        orbit.addEllipse (-rx, -ry, 2 * rx, 2 * ry);
        const float dash[] = { 2.0f, 4.0f };
        PathStrokeType (1.0f).createDashedStroke (d, orbit, dash, 2);
        g.setColour (Col::dim);
        g.fillPath (d);
        if (! hol && n > 0.001f)
        {
            Path trail;
            trail.addCentredArc (0.0f, 0.0f, rx, ry, 0.0f, 0.0f, n * MathConstants<float>::twoPi * 0.97f, true);
            g.setColour (Col::ink);
            g.strokePath (trail, PathStrokeType (2.0f));
        }
        drawRingExtras (g, i, a, rx, ry, t);
        const Point<float> pl (rx * std::cos (a), ry * std::sin (a));
        const auto body = Rectangle<float> (14.0f, 14.0f).withCentre (pl);
        g.setColour (hol ? Col::bg : Col::accent);
        g.fillEllipse (body);
        g.setColour (Col::ink);
        g.drawEllipse (body, 1.3f);
        if (r.tag.isNotEmpty())
        {
            Graphics::ScopedSaveState s2 (g);
            g.addTransform (AffineTransform::rotation (-tilt, pl.x, pl.y));
            g.setColour (hol ? Col::ink : Col::bg);
            g.setFont (mono (8.0f, Weight::semibold));
            g.drawText (r.tag, body.translated (0.0f, 0.5f), Justification::centred, false);
        }
    }
}

// ================================================================================================ user presets
namespace UserPresets
{
    File folder (const String& plugin)
    {
        return File::getSpecialLocation (File::userDocumentsDirectory)
                   .getChildFile ("sonicsystems Presets").getChildFile (File::createLegalFileName (plugin));
    }

    Array<File> list (const String& plugin, const String& legacy)
    {
        auto files = folder (plugin).findChildFiles (File::findFiles, false, "*.vbpreset");
        ignoreUnused (legacy);
        struct ByName { static int compareElements (const File& a, const File& b)
        { return a.getFileNameWithoutExtension().compareNatural (b.getFileNameWithoutExtension()); } } byName;
        files.sort (byName);
        return files;
    }

    bool saveTo (AudioProcessor& proc, const File& file)
    {
        MemoryBlock mb;
        proc.getStateInformation (mb);
        auto xml = AudioProcessor::getXmlFromBinary (mb.getData(), (int) mb.getSize());
        if (xml == nullptr) return false;
        xml->removeAttribute ("uiScale");      // the window size is never part of a sound
        xml->removeAttribute ("randomLocks");
        file.getParentDirectory().createDirectory();
        return xml->writeTo (file);
    }

    bool loadFrom (AudioProcessor& proc, const File& file, const std::function<ValueTree()>& state)
    {
        auto xml = parseXML (file);
        if (xml == nullptr) return false;
        const auto scale = state ? state().getProperty ("uiScale") : var();
        const auto locks = state ? state().getProperty ("randomLocks") : var();
        MemoryBlock mb;
        AudioProcessor::copyXmlToBinary (*xml, mb);
        proc.setStateInformation (mb.getData(), (int) mb.getSize());
        if (state && ! scale.isVoid()) state().setProperty ("uiScale", scale, nullptr);
        if (state && ! locks.isVoid()) state().setProperty ("randomLocks", locks, nullptr);
        return true;
    }
}

// ================================================================================================ top bar
TopBar::TopBar (Session& s, const String& name, const String& legacy) : session (s), pluginName (name), legacyFolder (legacy) {}

Rectangle<float> TopBar::zoneRect (Zone z) const
{
    const float W = (float) getWidth(), H = (float) getHeight();
    const float rndW = textWidth (mono (11.0f, Weight::semibold, 0.3f), "RANDOM") + 32.0f;
    const auto random = Rectangle<float> (W - 16.0f - rndW, (H - 34.0f) * 0.5f, rndW, 34.0f);
    const auto undo   = Rectangle<float> (random.getX() - 6.0f - 34.0f, random.getY(), 34.0f, 34.0f);
    const float mineW = textWidth (mono (9.5f, Weight::regular, 0.2f), "MY PRESETS") + 20.0f;
    const auto mine   = Rectangle<float> (undo.getX() - 12.0f - mineW, (H - 32.0f) * 0.5f, mineW, 32.0f);
    const auto box    = Rectangle<float> (mine.getX() - 12.0f - 288.0f, (H - 32.0f) * 0.5f, 288.0f, 32.0f);
    switch (z)
    {
        case Zone::prev:   return box.withWidth (26.0f);
        case Zone::next:   return box.withTrimmedLeft (box.getWidth() - 26.0f);
        case Zone::name:   return box.reduced (26.0f, 0.0f);
        case Zone::mine:   return mine;
        case Zone::undo:   return undo;
        case Zone::random: return random;
        case Zone::none:   break;
    }
    return box;
}

TopBar::Zone TopBar::zoneAt (Point<float> p) const
{
    for (auto z : { Zone::prev, Zone::name, Zone::next, Zone::mine, Zone::undo, Zone::random })
        if (zoneRect (z).contains (p)) return z;
    return Zone::none;
}

void TopBar::paint (Graphics& g)
{
    drawMark (g, { 16.0f, 12.0f, 34.0f, 34.0f });
    g.setColour (Col::ink);
    g.setFont (mono (19.0f, Weight::semibold, 0.42f));
    g.drawText (pluginName.toUpperCase(), Rectangle<float> (62.0f, 10.0f, 400.0f, 24.0f), Justification::centredLeft, false);
    g.setColour (Col::label);
    g.setFont (mono (9.0f, Weight::regular, 0.3f));
    g.drawText (String ("by ") + Brand::maker, Rectangle<float> (62.0f, 35.0f, 330.0f, 12.0f),
                Justification::centredLeft, false);

    // preset box
    const auto box = zoneRect (Zone::none);
    g.setColour (Col::ink);
    g.drawRect (box, 1.0f);
    g.setFont (mono (11.0f));
    for (auto z : { Zone::prev, Zone::next })
    {
        g.setColour (over == z ? Col::accent : Col::ink);
        g.drawText (String::fromUTF8 (z == Zone::prev ? "\xe2\x97\x82" : "\xe2\x96\xb8"), zoneRect (z), Justification::centred, false);
    }
    g.setColour (over == Zone::name ? Col::accent : Col::ink);
    g.setFont (mono (11.0f, Weight::regular, 0.08f));
    g.drawText ((currentName ? currentName() : String()).toUpperCase(), zoneRect (Zone::name), Justification::centred, true);

    // MY PRESETS
    const auto mine = zoneRect (Zone::mine);
    g.setColour (over == Zone::mine ? Col::accent : Col::line2);
    g.drawRect (mine, 1.0f);
    g.setColour (over == Zone::mine ? Col::ink : Col::label);
    g.setFont (mono (9.5f, Weight::regular, 0.2f));
    g.drawText ("MY PRESETS", mine, Justification::centred, false);

    // undo + RANDOM
    const auto undo = zoneRect (Zone::undo);
    const float ua = session.canUndo() ? 1.0f : 0.3f;
    g.setColour (Col::ink.withAlpha (ua));
    g.drawRect (undo, 1.0f);
    if (session.canUndo() && over == Zone::undo) { g.setColour (Col::line); g.fillRect (undo.reduced (1.0f)); g.setColour (Col::ink); }
    g.setFont (mono (14.0f));
    g.drawText (String::fromUTF8 ("\xe2\x86\xb6"), undo, Justification::centred, false);
    const auto rnd = zoneRect (Zone::random);
    g.setColour (over == Zone::random ? Col::accent : Col::ink);
    g.fillRect (rnd);
    g.setColour (Col::bg);
    g.setFont (mono (11.0f, Weight::semibold, 0.3f));
    g.drawText ("RANDOM", rnd, Justification::centred, false);
}

void TopBar::mouseMove (const MouseEvent& e)
{
    const auto z = zoneAt (e.position);
    if (z != over) { over = z; setMouseCursor (z == Zone::none ? MouseCursor::NormalCursor : MouseCursor::PointingHandCursor); repaint(); }
    if (z == Zone::random)    session.status ("RANDOM", "a new sound in musical ranges  ·  right-click a control to keep it");
    else if (z == Zone::undo) session.status ("UNDO", session.canUndo() ? "back to the sound before RANDOM" : "nothing to undo yet");
    else if (z == Zone::name) session.status ("PRESETS", "click for the list  ·  arrows = previous / next");
    else if (z == Zone::mine) session.status ("MY PRESETS", "save your own sounds and load them in any project");
    else session.hover (nullptr);
}

void TopBar::mouseExit (const MouseEvent&)
{
    over = Zone::none;
    repaint();
    session.hover (nullptr);
}

void TopBar::mouseUp (const MouseEvent& e)
{
    if (! e.mouseWasClicked()) return;
    switch (zoneAt (e.position))
    {
        case Zone::prev:   stepPreset (-1); break;
        case Zone::next:   stepPreset (1); break;
        case Zone::name:   showFactoryMenu(); break;
        case Zone::mine:   showMyPresets(); break;
        case Zone::undo:   session.undo(); break;
        case Zone::random: session.randomize(); break;
        case Zone::none:   break;
    }
    repaint();
}

void TopBar::stepPreset (int delta)
{
    if (factoryNames.isEmpty() || ! applyFactory) return;
    const int cur = factoryNames.indexOf (currentName ? currentName() : String());
    const int n = factoryNames.size();
    applyFactory (cur < 0 ? (delta > 0 ? 0 : n - 1) : (cur + delta + n) % n);
    repaint();
}

void TopBar::showFactoryMenu()
{
    if (factoryNames.isEmpty() || ! applyFactory) return;
    PopupMenu m;
    m.addSectionHeader ("FACTORY PRESETS");
    const auto cur = currentName ? currentName() : String();
    for (int i = 0; i < factoryNames.size(); ++i) m.addItem (i + 1, factoryNames[i], true, factoryNames[i] == cur);
    SafePointer<TopBar> safe (this);
    m.showMenuAsync (PopupMenu::Options().withTargetComponent (this).withTargetScreenArea (localAreaToGlobal (zoneRect (Zone::none).toNearestInt())),
                     [safe] (int r)
                     {
                         if (safe == nullptr || r <= 0) return;
                         safe->applyFactory (r - 1);
                         safe->repaint();
                     });
}

void TopBar::showMyPresets()
{
    userFiles = UserPresets::list (pluginName, legacyFolder);
    PopupMenu m;
    m.addSectionHeader ("MY PRESETS");
    m.addItem (1, "Save current sound...");
    m.addSeparator();
    if (userFiles.isEmpty()) m.addItem (2, "(nothing saved yet)", false);
    for (int i = 0; i < userFiles.size(); ++i) m.addItem (100 + i, userFiles[i].getFileNameWithoutExtension());
    m.addSeparator();
    m.addItem (3, "Open presets folder");
    SafePointer<TopBar> safe (this);
    m.showMenuAsync (PopupMenu::Options().withTargetComponent (this).withTargetScreenArea (localAreaToGlobal (zoneRect (Zone::mine).toNearestInt())),
                     [safe] (int r)
                     {
                         if (safe == nullptr || r == 0) return;
                         if (r == 1) safe->askNameAndSave();
                         else if (r == 3) { auto f = UserPresets::folder (safe->pluginName); f.createDirectory(); f.startAsProcess(); }
                         else if (r >= 100 && r - 100 < safe->userFiles.size())
                         {
                             const auto file = safe->userFiles[r - 100];
                             if (UserPresets::loadFrom (safe->session.proc, file, safe->session.state) && safe->onUserPresetLoaded)
                                 safe->onUserPresetLoaded (file.getFileNameWithoutExtension());
                             safe->repaint();
                         }
                     });
}

void TopBar::askNameAndSave()
{
    String name;   // a free name, so OK alone works even when the host keeps the keyboard
    for (int n = 1; n < 1000; ++n)
        if (! UserPresets::folder (pluginName).getChildFile ("My Sound " + String (n) + ".vbpreset").exists())
        { name = "My Sound " + String (n); break; }

    auto* aw = new AlertWindow ("SAVE PRESET", "Name for this sound:", MessageBoxIconType::NoIcon, this);
    aw->addTextEditor ("name", name);
    aw->addButton ("SAVE", 1, KeyPress (KeyPress::returnKey));
    aw->addButton ("CANCEL", 0, KeyPress (KeyPress::escapeKey));
    SafePointer<TopBar> safe (this);
    aw->enterModalState (true, ModalCallbackFunction::create ([safe, aw] (int r)
    {
        if (safe == nullptr || r != 1) return;
        const auto n = File::createLegalFileName (aw->getTextEditorContents ("name").trim());
        if (n.isEmpty()) return;
        const auto file = UserPresets::folder (safe->pluginName).getChildFile (n + ".vbpreset");
        if (UserPresets::saveTo (safe->session.proc, file) && safe->onUserPresetLoaded) safe->onUserPresetLoaded (n);
    }), true);
}

// ================================================================================================ footer
Footer::Footer() { startTimerHz (30); }

void Footer::timerCallback()
{
    if (! isShowing()) return;
    const float lv = staticDb <= 0.0f ? Decibels::decibelsToGain (staticDb) : (level ? level() : 0.0f);
    const float db = Decibels::gainToDecibels (lv, -60.0f);
    shown = db > shown ? db : jmax (-60.0f, shown - 1.5f);
    clip = clipping && clipping();
    repaint (getWidth() - 200, 0, 200, getHeight());
}

void Footer::paint (Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    const float meterW = 16.0f * 7.0f, right = b.getWidth() - 16.0f;
    // status line: bold part white, the rest grey
    const auto f = mono (9.5f, Weight::regular, 0.08f);
    const float bw = boldText.isEmpty() ? 0.0f : textWidth (f, boldText) + 8.0f;
    const float maxW = right - meterW - 90.0f - 16.0f;
    g.setFont (f);
    g.setColour (Col::ink);
    g.drawText (boldText, Rectangle<float> (16.0f, 0.0f, jmin (bw, maxW), b.getHeight()), Justification::centredLeft, true);
    g.setColour (Col::mute);
    g.drawText (restText, Rectangle<float> (16.0f + bw, 0.0f, jmax (0.0f, maxW - bw), b.getHeight()), Justification::centredLeft, true);

    if (! level && staticDb > 0.0f) return;   // this plugin has no output meter
    // CLIP + OUT meter: 16 squares from -48 to 0 dBFS, the top two red
    const int lit = roundToInt (jlimit (0.0f, 1.0f, (shown + 48.0f) / 48.0f) * 16.0f);
    float x = right - meterW;
    for (int i = 0; i < 16; ++i, x += 7.0f)
    {
        g.setColour (i < lit ? (i >= 14 ? Col::accent : Col::ink) : Col::dim);
        g.fillRect (x, b.getCentreY() - 4.0f, 5.0f, 8.0f);
    }
    g.setColour (Col::mute);
    g.drawText ("OUT", Rectangle<float> (right - meterW - 34.0f, 0.0f, 30.0f, b.getHeight()), Justification::centredRight, false);
    if (clipping)
    {
        g.setColour (clip ? Col::accent : Col::dim);
        g.drawText ("CLIP", Rectangle<float> (right - meterW - 84.0f, 0.0f, 40.0f, b.getHeight()), Justification::centredRight, false);
    }
}

// ================================================================================================ window
Window::Window (Session& s, const String& pluginName, const String& legacyPresetFolder, int sw)
    : session (s), top (s, pluginName, legacyPresetFolder), stageWidth (sw)
{
    setOpaque (true);
    addAndMakeVisible (top);
    addAndMakeVisible (panel);
    addAndMakeVisible (footer);
    s.hintBold = pluginName;
    s.hintRest = String::fromUTF8 ("drag \xc2\xb7 shift = fine \xc2\xb7 double-click = reset \xc2\xb7 right-click = automation clip / freeze");
    s.showStatus = [this] (const String& b, const String& r) { footer.setStatus (b, r); };
    s.onFlash = [this] { repaint(); };
    s.onUndoChanged = [this] { top.repaint(); };
    s.hover (nullptr);
    setSize (W, H);
}

void Window::setStage (Stage* s)
{
    stage = s;
    addAndMakeVisible (s);
    resized();
}

void Window::paint (Graphics& g)
{
    g.fillAll (session.flash() > 0.5f ? Col::strobe : Col::bg);
    g.setColour (Col::line);
    g.fillRect (0, 57, W, 1);
    g.fillRect (0, H - 26, W, 1);
    g.drawRect (getLocalBounds(), 1);
}

void Window::resized()
{
    top.setBounds (0, 0, W, 57);
    footer.setBounds (0, H - 25, W, 25);
    auto body = Rectangle<int> (0, 58, W, H - 58 - 26).reduced (16, 14);
    if (stage != nullptr) stage->setBounds (body.removeFromRight (stageWidth));
    body.removeFromRight (16);
    panel.setBounds (body);
}

void makeResizable (AudioProcessorEditor& ed, Window& win, ValueTree state)
{
    ed.addAndMakeVisible (win);
    double start = (double) state.getProperty ("uiScale", 0.0);
    if (! (start > 0.0))
    {
        start = 1.0;
        if (auto* d = Desktop::getInstance().getDisplays().getPrimaryDisplay())
            start = jmin (1.0, (d->userArea.getHeight() - 90) / (double) Window::H, (d->userArea.getWidth() - 40) / (double) Window::W);
    }
    start = jlimit (0.6, 1.5, start);
    ed.setResizable (true, true);
    ed.setResizeLimits (roundToInt (Window::W * 0.6), roundToInt (Window::H * 0.6), roundToInt (Window::W * 1.5), roundToInt (Window::H * 1.5));
    if (auto* c = ed.getConstrainer()) c->setFixedAspectRatio ((double) Window::W / (double) Window::H);
    ed.setSize (roundToInt (Window::W * start), roundToInt (Window::H * start));
}

float scaleWindow (AudioProcessorEditor& ed, Window& win, ValueTree state)
{
    const float scale = jlimit (0.3f, 3.0f, (float) ed.getWidth() / (float) Window::W);
    win.setTransform (AffineTransform::scale (scale));
    win.setBounds (0, 0, Window::W, Window::H);
    if (win.getParentComponent() == &ed)   // not for the first layout, before the window is in the editor
        state.setProperty ("uiScale", scale, nullptr);
    return scale;
}

// ================================================================================================ own sound
OwnSoundBox::OwnSoundBox (Session& s, Sonic::OwnSound& o) : session (s), own (o)
{
    setMouseCursor (MouseCursor::PointingHandCursor);
    refresh();
    startTimerHz (15);
}

Point<float> OwnSoundBox::sizeAt (float s) const { return { 124.0f * s, 44.0f * s + 2.0f + labelH + valueH }; }

void OwnSoundBox::refresh()
{
    shownName = own.hasSound() ? own.name() : String();
    peaks = own.peaks (60);
    repaint();
}

void OwnSoundBox::timerCallback()
{
    if ((own.hasSound() ? own.name() : String()) != shownName || own.hasSound() == peaks.empty()) refresh();
    const int h = own.hits.load();
    if (h != lastHits) { lastHits = h; hitAt = Time::getMillisecondCounterHiRes(); }
    if (Time::getMillisecondCounterHiRes() - hitAt < 450.0) repaint();
}

void OwnSoundBox::paint (Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    const auto box = b.removeFromTop (44.0f * z).reduced (0.5f);
    const float flash = jlimit (0.0f, 1.0f, 1.0f - (float) ((Time::getMillisecondCounterHiRes() - hitAt) / 450.0));
    g.setColour (Col::bg);
    g.fillRect (box);
    g.setColour (dragOver ? Col::accent : hovered ? Col::mute : Col::line2);
    g.drawRect (box, 1.0f);
    if (! peaks.empty())
    {
        const float x0 = box.getX() + 5.0f, w = box.getWidth() - 10.0f, cy = box.getCentreY(), hh = box.getHeight() * 0.5f - 5.0f;
        g.setColour (Col::accent.withAlpha (0.5f + 0.5f * flash));
        for (size_t i = 0; i < peaks.size(); ++i)
        {
            const float x = x0 + w * (float) i / (float) (peaks.size() - 1), h = jmax (0.5f, peaks[i] * hh);
            g.fillRect (x, cy - h, 1.0f, 2.0f * h);
        }
    }
    else
    {
        g.setColour (dragOver ? Col::accent : Col::mute);
        g.setFont (mono (8.5f, Weight::regular, 0.16f));
        g.drawText ("DROP A SAMPLE", box.withTrimmedBottom (box.getHeight() * 0.42f), Justification::centredBottom, false);
        g.setColour (Col::mute.withAlpha (0.6f));
        g.drawText ("or click", box.withTrimmedTop (box.getHeight() * 0.56f), Justification::centredTop, false);
    }
    b.removeFromTop (2.0f);
    g.setColour (Col::label);
    g.setFont (labelFont());
    g.drawText ("SAMPLE", b.removeFromTop (labelH), Justification::centredLeft, false);
    g.setColour (shownName.isEmpty() ? Col::mute : Col::ink);
    g.setFont (valueFont());
    g.drawText (shownName.isEmpty() ? String ("none") : shownName, b.removeFromTop (valueH), Justification::centredLeft, true);
}

void OwnSoundBox::hint()
{
    if (own.hasSound())
        session.status ("OWN SOUND", own.name() + "  ·  " + String (own.seconds(), 2) + " s  ·  click = another sample  ·  "
                                     "right-click = remove  ·  LAYER = on top of the plugin, REPLACE = only your sample");
    else
        session.status ("OWN SOUND", "click = load your own sample (WAV, AIFF, FLAC, MP3, OGG)  ·  or drop a file here, "
                                     "e.g. from FL's browser");
}

void OwnSoundBox::mouseEnter (const MouseEvent&) { hovered = true; hint(); repaint(); }
void OwnSoundBox::mouseExit (const MouseEvent&) { hovered = false; session.hover (nullptr); repaint(); }

void OwnSoundBox::mouseUp (const MouseEvent& e)
{
    if (! e.mouseWasClicked()) return;
    if (! e.mods.isPopupMenu()) { choose(); return; }
    SafePointer<OwnSoundBox> safe (this);
    PopupMenu m;
    m.addSectionHeader ("OWN SOUND");
    m.addItem ("Load a sample...", [safe] { if (safe != nullptr) safe->choose(); });
    m.addItem ("Remove the sample", own.hasSound(), false, [safe]
    {
        if (safe == nullptr) return;
        safe->own.clear();
        safe->refresh();
        safe->session.status ("OWN SOUND", "removed  ·  the plugin plays its own sound again");
    });
    m.showMenuAsync (PopupMenu::Options().withMousePosition());
}

void OwnSoundBox::choose()
{
    static File lastFolder = File::getSpecialLocation (File::userMusicDirectory);
    chooser = std::make_unique<FileChooser> ("Load your own sound", lastFolder, Sonic::OwnSound::fileTypes());
    SafePointer<OwnSoundBox> safe (this);
    chooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles, [safe] (const FileChooser& fc)
    {
        const auto f = fc.getResult();
        if (safe == nullptr || ! f.existsAsFile()) return;
        lastFolder = f.getParentDirectory();
        safe->load (f);
    });
}

void OwnSoundBox::load (const File& f)
{
    const auto note = own.loadFile (f);
    refresh();
    if (own.hasSound() && own.name() == f.getFileNameWithoutExtension())
        session.status ("OWN SOUND", own.name() + " loaded  ·  " + String (own.seconds(), 2) + " s" + (note.isEmpty() ? String() : "  ·  " + note)
                                     + "  ·  LAYER = on top of the plugin, REPLACE = only your sample");
    else
        session.status ("OWN SOUND", note);
}

bool OwnSoundBox::isInterestedInFileDrag (const StringArray& files)
{
    for (auto& f : files)
        if (File (f).hasFileExtension ("wav;aif;aiff;flac;mp3;ogg")) return true;
    return false;
}

void OwnSoundBox::filesDropped (const StringArray& files, int, int)
{
    dragOver = false;
    for (auto& f : files)
        if (File (f).hasFileExtension ("wav;aif;aiff;flac;mp3;ogg")) { load (File (f)); return; }
    repaint();
}

OwnSoundParts::OwnSoundParts (Session& s, Sonic::OwnSound& o)
    : box (s, o), level (s, Sonic::OwnID::level, "Level"), tune (s, Sonic::OwnID::tune, "Tune"),
      key (s, Sonic::OwnID::key, "Pitch", StringArray { "FIXED", "NOTES" }),
      mode (s, Sonic::OwnID::mode, "Mode", StringArray { "LAYER", "REPLACE" }),
      modes ({ &key, &mode })
{
}

static std::vector<Component*> ownItems (OwnSoundParts& p, std::vector<Component*> extra)
{
    std::vector<Component*> v { &p.box, &p.level, &p.tune, &p.modes };
    v.insert (v.end(), extra.begin(), extra.end());
    return v;
}

OwnSoundGroup::OwnSoundGroup (Session& s, Sonic::OwnSound& o, std::vector<Component*> extra)
    : OwnSoundParts (s, o), Group ("OWN SOUND", ownItems (*this, std::move (extra)))
{
}

void keepKeyboardForHost (Component& root)
{
    root.setWantsKeyboardFocus (false);
    root.setMouseClickGrabsKeyboardFocus (false);
    for (auto* child : root.getChildren()) keepKeyboardForHost (*child);
}
}
