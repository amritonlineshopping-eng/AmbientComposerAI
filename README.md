# Ambient Composer AI

A dedicated **ambient / cinematic MIDI-generator plugin** (VST3 · AU · Standalone) built with JUCE 8 and C++20, for **Windows and macOS**. It composes original, royalty-free, emotionally-evocative **chord progressions + melodies** in the style of *Oneheart, Antent, .diedlonely* and similar atmospheric producers — then you **drag the MIDI straight into FL Studio** (or any DAW) and play it with your own sounds.

> **On Windows?** Jump to [🪟 Windows: build a shareable installer](#-windows-build-a-shareable-installer-for-your-friend) — your friend just downloads one `.exe` and runs it.

The plugin outputs **MIDI only**. It ships a tiny built-in preview synth used *solely* for auditioning inside the plugin window — it never renders your final sound.

---

## What it does

- Generates **exactly 4 voice-led chords** (whole-note ambient pads) + a **fitting, non-random melody** over an 8-bar loop (4 chords × 2 bars).
- **15 moods** (Sad, Lonely, Emotional, Happy, Hopeful, Bright, Dark, Cold, Dreamy, Floating, Ethereal, Nostalgic, Cinematic, Peaceful, Warm) that *audibly* change scale, chord colour, contour, density, rhythm, dynamics and note length.
- **12 scales** (+ "Auto" — picks one that fits the mood) and all 12 keys (+ "Random").
- Lush ambient extensions (maj7 / min7 / maj9 / min9 / add9 / sus2 / sus4 / 6 / 6-9), nearest-note voice leading, borrowed chords (♭VII, ♭VI), open/spread voicings, octave doubling and sustain/overlap for legato pads.
- Constraint-based melody: motif → repetition with variation → chord-tone targeting on strong beats, passing/neighbour tones on weak beats, expressive leaps with recovery, and a proper cadence.
- **Complexity**, **Velocity/Timing/Gate humanize**, voicing width, sustain, tempo, time-signature, melody/chord register — all shaping the output live.
- **Seed system**: every generation is unique and reproducible. Lock a seed to tweak one setting; copy/paste seeds to reproduce a vibe.
- **Piano-roll preview** (chords vs melody colour-coded) with a moving playhead and Play / Stop / Loop.
- **Drag-and-drop** three ways (Chords / Melody / Combined) plus an **Export…** file dialog. Combined exports as two named tracks with the correct tempo & time-signature.
- **Full session state** (all parameters + the exact last-generated music + seed) saves and restores inside your DAW project, plus **undo/redo history** and **favourites**.

---

---

## 🪟 Windows: build a shareable installer for your friend

Your friend just needs **one file**: `AmbientComposerAI-Setup.exe`. He downloads it, double-clicks, clicks through the wizard, and it installs the VST3 (for FL Studio) and the standalone app — no Visual C++ redistributable, no other dependencies (the Windows runtime is linked statically).

You don't need a Windows machine to produce it. This repo ships a **GitHub Actions** pipeline that builds and tests the plugin on a real Windows machine and packages the installer for you:

1. Put this project on GitHub (one-time):
   ```bash
   # from the project folder (a git repo is already initialised for you)
   gh repo create ambient-composer-ai --private --source=. --push
   # …or create an empty repo on github.com and:
   #   git remote add origin https://github.com/<you>/ambient-composer-ai.git
   #   git push -u origin main
   ```
2. GitHub automatically runs the **Build** workflow (watch it under the repo's **Actions** tab). It compiles with MSVC, runs the full test suite + pluginval, and builds the installer.
3. When it finishes (green check), open that workflow run → **Artifacts** → download **`AmbientComposerAI-Windows-Installer`**. Inside is `AmbientComposerAI-Setup.exe`.
4. Send that `.exe` to your friend.

**Want a shareable download link instead of an artifact?** Push a version tag and GitHub publishes a **Release** with the installer attached at a public URL:
```bash
git tag v1.0.0
git push origin v1.0.0
```
The installer then appears at `https://github.com/<you>/ambient-composer-ai/releases` — send your friend that link.

> There's also a **portable** artifact (`AmbientComposerAI-Windows-Portable`) — a plain zip of the VST3 + standalone with no installer, if you'd rather copy files by hand.

### What your friend does (Windows)

1. Download `AmbientComposerAI-Setup.exe` and run it (Windows may show a SmartScreen prompt for an unsigned app → **More info ▸ Run anyway**).
2. Click through the installer (it needs admin to write to the plugin folder).
3. In **FL Studio**: **Options ▸ Manage plugins ▸ Find installed plugins**, then add **Ambient Composer AI** to a channel. Or launch the **standalone** app from the Start Menu to try it with no DAW.

The installer places the VST3 in `C:\Program Files\Common Files\VST3` (FL scans this automatically) and the standalone app in `C:\Program Files\Ambient Composer AI` with Start-Menu/desktop shortcuts.

### Building on Windows directly (optional)

If you *do* have a Windows machine with **Visual Studio 2022** (Desktop C++) and **CMake**:
```bat
cmake -S . -B build
cmake --build build --config Release
```
Then build the installer with [Inno Setup 6](https://jrsoftware.org/isdl.php):
```bat
"C:\Program Files (x86)\Inno Setup 6\ISCC.exe" installer\AmbientComposerAI.iss
```
The setup `.exe` lands in `installer\Output\`.

---

## Prerequisites (macOS)

```bash
xcode-select --install     # Xcode Command Line Tools (compiler)
brew install cmake         # CMake 3.24+
```

JUCE is pulled automatically by CMake (`FetchContent`, pinned to 8.0.14) — **no manual download needed**.

---

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

The first configure clones JUCE (a few minutes). Subsequent builds are fast.

Build a single target if you prefer:

```bash
cmake --build build --target AmbientComposerAI_Standalone   # the standalone app
cmake --build build --target AmbientComposerAI_VST3         # VST3 (FL Studio)
cmake --build build --target AmbientComposerAI_AU           # AU (Logic/GarageBand)
cmake --build build --target AmbientComposerTests           # headless engine tests
```

### Where things install

`COPY_PLUGIN_AFTER_BUILD` copies the plugins on every build:

| Format | Location |
|---|---|
| VST3 | `~/Library/Audio/Plug-Ins/VST3/Ambient Composer AI.vst3` |
| AU   | `~/Library/Audio/Plug-Ins/Components/Ambient Composer AI.component` |
| Standalone | `build/AmbientComposerAI_artefacts/Release/Standalone/Ambient Composer AI.app` |

If macOS Gatekeeper blocks the (unsigned) plugin, clear its quarantine flag:

```bash
xattr -dr com.apple.quarantine "~/Library/Audio/Plug-Ins/VST3/Ambient Composer AI.vst3"
xattr -dr com.apple.quarantine "~/Library/Audio/Plug-Ins/Components/Ambient Composer AI.component"
```

---

## Using it in FL Studio

1. Build (the VST3 auto-installs). In FL Studio: **Options ▸ Manage plugins ▸ Find installed plugins** so it rescans `~/Library/Audio/Plug-Ins/VST3`.
2. Add **Ambient Composer AI** to a channel/mixer track (it's an *instrument*).
3. Pick a **Mood**, optionally a Key/Scale, and press **Generate Both**. Hit **Play** to audition with the preview synth.
4. Tweak **Complexity**, **Sustain**, the **Humanize** knobs, voicing, etc. Press **Generate Melody** for a new lead over the same chords, or **Generate Chords** to keep the melody.
5. **Drag** the **Chords**, **Melody**, or **Combined** tile from the bottom bar straight onto an FL playlist track or into the piano roll. The MIDI lands at the correct tempo; *Combined* arrives as two named tracks.
   - Prefer a file? Use **Export…**.
6. Assign your own instrument to the imported MIDI and play. Lock the seed if you found a vibe you like and want to tweak one control without losing it.

> **MIDI delivery is by drag-and-drop / file export**, not live MIDI-out — this is the reliable path into FL Studio. The plugin intentionally renders silence to the host except for the in-window preview synth.

---

## Verifying the build

```bash
cmake --build build --target AmbientComposerTests -j
./build/AmbientComposerTests_artefacts/Release/AmbientComposerTests
```

This headless suite checks determinism, per-part reproducibility, session uniqueness, scale/chord conformance (zero out-of-scale notes, zero harsh clashes, zero impossible leaps across every mood × scale), mood differentiation, the complexity mapping, generation speed (< 100 ms) and MIDI-file validity.

On macOS you can also validate the AU with Apple's tool:

```bash
auval -v aumu Acai Amra
```

---

## Architecture

Pure, JUCE-free, headlessly-testable **generation core**, cleanly separated from JUCE UI/audio:

```
Source/
  PluginProcessor.*      AudioProcessor, APVTS, state save/restore, hosts preview
  PluginEditor.*         custom dark UI, layout, wiring
  Theory/                scales, keys, chords, roman-numeral resolver
  Engines/
    RandomEngine.h       master seed -> {chord, melody, humanize} sub-seeds
    ScaleEngine.*        resolves Key/Scale (Random/Auto) -> MusicalContext
    MoodEngine.*         the 15-mood data table every engine reads from
    ChordEngine.*        templates -> subs/borrowed -> extensions -> voice leading
    MelodyEngine.*       motif + variation + chord-tone targeting + cadence
    Humanizer.*          velocity / timing / gate variation (bounded, safe)
    CompositionValidator.*  final guarantee pass (in-scale/consonant/no wild leaps)
    PatternGenerator.*   the single deterministic pipeline entry point
    IGenerator.h         extension seam for future tracks (bass, arp, counter-melody)
  IO/
    MidiExporter.*       Type-1 SMF + drag-and-drop temp files
    PatternState.*       pattern <-> ValueTree for session persistence
  Playback/
    PreviewSynth.*       tiny soft audition synth (not your sound)
    PreviewPlayer.*      RT-safe transport, lock-free sequence swap, all-notes-off
    PlaybackSequence.h   flattened, sorted event list (the only thing the audio thread reads)
  GUI/
    LookAndFeelDark.*    dark theme + custom knobs/buttons
    PianoRollComponent.* the hero view + moving playhead
    Controls.h           APVTS-attached widgets + MIDI drag tiles
  Model/
    Note.h               the atomic event (960 PPQ) + overlap sanitiser
    GeneratedPattern.h   chords + melody + metadata + fingerprints
Tests/
  EngineTests.cpp        headless verification of the whole core
```

**Data flow:** parameters (APVTS) → RandomEngine derives sub-seeds → MoodEngine profile → Key/Scale resolved → ChordEngine → MelodyEngine → Humanizer → CompositionValidator → `GeneratedPattern` → piano roll + preview player + MIDI export.

Generation runs on the message thread and completes in well under a millisecond. The audio thread only reads the current `PlaybackSequence` via a lock-free hand-off — no allocations, no blocking, click-free preview, and swapping the pattern mid-playback never hangs a note.

---

## Notes

- Company/manufacturer are placeholders (`AmritAudio` / code `Amra`, plugin code `Acai`).
- Plugin registers as an **instrument** with an audio output bus (needed for the preview synth); it is *not* a pure MIDI-effect, so FL always gives it an audio channel.
- The generation core has no JUCE dependency, so it can be unit-tested and reused independently of the plugin shell.
