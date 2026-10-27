# voidbringer by sonicsystems — read first

FM screech synth (VST3 + CLAP, JUCE 8.0.4). The user's own standalone remake (2026-10-27) of an earlier synth of
theirs: same DSP, same 39 parameters (IDs, ranges, defaults) and the same 16 factory presets; only the look changed.
Main host: FL Studio on Windows. Answer the owner in their language (Polish or English), simply.

## Identity (frozen once released — FL stores these in projects)
- `PRODUCT_NAME` = `PLUGIN_NAME` = "voidbringer", company "sonicsystems"
- Manufacturer `Sncs`, plugin code `Vdbr`, bundle / CLAP ID `com.sonicsystems.voidbringer`
- Parameter IDs: `Source/Params.h` (`PID::`). New parameters only at the END of the layout.

## Look
- Black, white type, **red `#FF2E3D` as the only colour** (`Vb::Col::accent`), JetBrains Mono (fonts/, OFL),
  square corners, 1 px lines. Header: VOIDBRINGER + "by sonicsystems" (no suite name anywhere).
- The black hole stage ("gravity well", `WellStage` in PluginEditor.cpp) keeps its shape and motion; its colour
  fades white -> red with the sound (`heatAll` = FM amount + feedback + drive).
- `Source/VoidUI.h/.cpp` = the window, controls, panel auto-layout, RANDOM/FREEZE, MY PRESETS
  (`Documents/sonicsystems Presets/voidbringer/*.vbpreset`), OWN SOUND tab (`OwnSound.h`).
- Mockup: `design/voidbringer.html`.

## Build
- `python build.py` (Visual Studio 2026, JOBS=2 — never more, a parallel build crashed the laptop) builds VST3 + CLAP
  + `VoidbringerRender`, runs the render test (must print `RESULT: PASS`) and puts the result in `out/`.
  `python build.py --install` also copies it into `C:\Program Files\Common Files\VST3` (admin, FL closed).
- `deps/` (not in git): JUCE 8.0.4 in `deps/JUCE`, clap-juce-extensions in `deps/cje` (on the laptop a junction to an existing deps folder).
- Install, commit, push and look changes only after the user says so.

## Rules
- Keep sound + presets identical to the original unless the user asks.
- Never let names of the user's other projects show up in this one (code, files, anything FL shows); preset names ("Rawstyle Terror Screech", "Terror Laser") are the user's and stay.
