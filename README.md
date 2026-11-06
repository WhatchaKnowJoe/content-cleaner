# Content Cleaner

One-plugin voice repair + polish for content creators. VST3 / AU / Standalone, built with JUCE 8.
Designed to live in Adobe Premiere Pro and Audition (also works in any VST3/AU host).

## How creators use it (Simple view)

1. Play the clip, press **Analyze**. Five seconds later the seven problems are set for you
   and "What I found" explains why in plain English.
2. Turn **Clean** up or down. Slide **Natural ↔ Aggressive** if it sounds processed.
3. Optional: **Learn Room** on 2 s of silence for a sharper noise fingerprint.
4. **Hear what's removed** solos the difference — if you hear voice in there, back off.

The seven problems (Off / Low / Mid / High) drive the engineering stages below:

| Problem | Drives |
|---|---|
| Echoey room | De-reverb amount, gate range |
| Background noise | Denoise amount |
| Hum / buzz | Hum notch bank depth (frequency auto-detected) |
| Pops & clicks | Plosive tamer, de-click (Mid+) |
| Harsh S sounds | De-esser amount |
| Uneven / quiet | Leveler + multiband amounts |
| Distorted | Clip repair + threshold |

`Natural ↔ Aggressive` is not cosmetic: it changes the spectral floor, over-subtraction factor and
gain smoothing in the denoiser/de-reverb, and the gate's depth and release.

## Testing

`cmake --build build --target AnalyzerTest && ./build/AnalyzerTest_artefacts/Release/AnalyzerTest`
synthesises a bad recording (reverb, noise, 60 Hz hum, sibilance, clipping, level drift), checks every
detector fires (and doesn't on a clean take), and verifies the denoiser lowers the floor ≥ 8 dB while
holding the voice within 2 dB.

## Signal chain (fixed order — repair first, polish last)

| # | Stage | Status | Notes |
|---|-------|--------|-------|
| 1 | High-pass | ✅ | 2nd-order Butterworth, 20–300 Hz |
| 2 | Hum remover | ✅ | 50/60 Hz + 5 harmonics, peak-notch bank |
| 3 | Denoise | ✅ baseline | STFT spectral subtraction, **Learn Room** profile or adaptive min-statistics |
| 4 | De-reverb | ✅ baseline | Late-reverb suppression on the same STFT (no extra latency) |
| 5 | Gate | ✅ | Linked downward expander w/ soft knee, hold, range |
| 6 | De-click / Plosive | ✅ plosive, 🟡 click | Plosive tamer = low-band fast comp @150 Hz. De-click is a basic slope detector |
| 7 | Clip repair | 🟡 basic | Cubic Hermite reconstruction of clipped runs ≤ 64 samples |
| 8 | Voice EQ | ✅ | Mud (250 Hz) / Presence (3.5 k) / Air (10 k shelf) |
| 9 | De-esser | ✅ | Linkwitz-Riley split-band compressor |
| 10 | Multiband comp | ✅ | 3 bands, one Amount control, auto make-up |
| 11 | Leveler | ✅ | Slow RMS gain rider toward target, speech-floor gated |
| 12 | Loudness + limiter | ✅ approx | K-weighted short-term LUFS steer to platform target + limiter |

Global: **Clean** macro (scales all amounts; 50 % = params as set), **Mic presets**, **Loudness target**,
**Delta Listen** (hear exactly what was removed, latency-matched), live "removed spectrum" display.

Reported latency = 1024 + 68 samples (STFT + clip repair). Premiere compensates automatically.

## Build

Requires CMake ≥ 3.22 and a C++17 compiler. JUCE is fetched automatically, or pass `-DJUCE_DIR=/path/to/JUCE`.

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --target ContentCleaner_VST3        # or ContentCleaner_AU / ContentCleaner_Standalone
```

Artefacts land in `build/ContentCleaner_artefacts/Release/`.

**Linux deps:** `libxrandr-dev libxinerama-dev libxcursor-dev libasound2-dev libfreetype-dev libfontconfig1-dev libgl1-mesa-dev`
**macOS:** Xcode CLT. For Premiere on Apple Silicon, build a universal binary: `-DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"`.
**Windows:** VS 2022; use `-G "Visual Studio 17 2022"`.

Install: copy `Content Cleaner.vst3` to
- Windows: `C:\Program Files\Common Files\VST3\`
- macOS: `/Library/Audio/Plug-Ins/VST3/` (and `.component` → `/Library/Audio/Plug-Ins/Components/`)

Then in Premiere: *Preferences → Audio → Audio Plug-In Manager → Scan*.

## Adobe-specific notes

- Premiere applies **clip effects** per clip (state saved with the clip) and **track effects** on the mixer.
  Learn Room profiles are stored in plugin state, so they travel with the clip.
- Premiere's Essential Sound / Enhance Speech is the incumbent. Our angle: *transparent + controllable* —
  users see what's removed, can bypass any stage, and never get the "robot voice" artefact.
- Test both 48 kHz (video standard) and 44.1 kHz. Everything here is sample-rate agnostic.
- Audition supports real-time VST3 and offline "Apply" — the chain is stateless enough that offline rendering is fine.

## Roadmap (in priority order)

1. **ML speech isolation** — swap `SpectralDenoiser::computeDenoiseGains()` for a DeepFilterNet-class model
   via ONNX Runtime or RTNeural. The STFT scaffolding stays; the model just emits per-bin gains.
2. **ML de-reverb** — same slot in `DereverbEstimator`.
3. **Proper de-click / mouth-noise** — AR-model interpolation (as in classic declickers) or ML.
4. **Wind noise** module (low-band modulated noise detector + spectral gain).
5. **True integrated LUFS** with BS.1770 gating + true-peak oversampled limiter.
6. **Offline "Analyze & Render" mode** — scan the whole clip, pick settings, then render.
7. **A/B snapshot slots** and preset save/load.
8. UI: before/after spectrogram strip, per-stage gain-reduction meters, resizable editor.

## Code map

```
Source/
  Parameters.h/.cpp       every parameter ID, ranges, presets
  PluginProcessor.*       APVTS, Learn Room handling, mic presets, latency
  PluginEditor.*          Simple/Advanced UI
  UI/ModuleStrip.*        one Advanced-panel stage (toggle + knobs)
  UI/Theme.h              palette
  DSP/ModuleBase.h        stage interface + helpers
  DSP/ModuleChain.*       the 12-stage chain, Clean macro, Delta Listen
  DSP/<Stage>.*           one file pair per stage
```
