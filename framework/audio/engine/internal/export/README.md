# Audio export: single-file vs. multi-stem

This directory has two writers with a shared purpose (render the audio engine
offline and encode the result to a file) but different scopes:

- `soundtrackwriter.{h,cpp}` — renders the whole engine graph and encodes the
  **summed master output** to one file. This is the original, unchanged path
  used by a normal single-instrument or full-score audio export.
- `multisoundtrackwriter.{h,cpp}` — renders the graph **once** and encodes
  several **per-track stems** to several files in the same pass, instead of
  doing one full render per file. Added for exporting audio for multiple
  parts/instruments of a score at once (see the companion `MuseScore` PR that
  consumes `IPlayback::saveSoundTracks`).

Both share encoder construction via `encoderfactory.{h,cpp}` (previously
duplicated inline in `soundtrackwriter.cpp`).

## How the stem capture works

`Mixer::process()` already computes a separate buffer per track on every
render block, before summing them into the master output (see
`mixer.cpp`, `processTrackChannels()` / `mixOutputFromChannel()`). Only the
summed result was ever kept. `Mixer::setTrackStemCallback()` adds an optional
hook, invoked with each track's own buffer at exactly that point — right
after that track's own volume/pan/fx chain has run, right before it's added
to the master mix. No callback set means no behavior change to normal
playback or the existing single-file export.

`MultiSoundTrackWriter` uses this to drive one offline render loop (mirroring
`SoundTrackWriter`'s phased leading-silence / data / trailing-silence
structure) while its callback encodes each requested track's buffer to that
track's own encoder as it's produced. The render loop's own output buffer
(the *summed* mix) is intentionally discarded — only the callback's per-track
buffers are used.

**Known tradeoff, not a bug:** the stem callback fires *before* aux/reverb
sends are computed (`writeTrackToAuxBuffers()` runs after it, per track, in
the same loop). A part that relies on a shared reverb aux bus will sound
drier in its exported stem than in the full mix, or than soloing that same
track via the mixer panel (soloing still routes through aux sends normally).
Moving the capture point to after aux mixing would need each track's *wet*
contribution isolated per-track, which aux buses don't currently track
per-source — a bigger change, and not attempted here.

## Threading

`Mixer::setTrackStemCallback()` asserts `ONLY_AUDIO_ENGINE_THREAD`, matching
its sibling setters (`setAuxSends`, `setTracksToProcessWhenIdle`). Both
`SoundTrackWriter` and `MultiSoundTrackWriter` are driven from
`AudioContext::doSaveSoundTrack(s)` inside `execOperation`, which is what
gives the offline render exclusive access to the graph while it runs (see
the existing comment in `audiocontext.cpp` above `doSaveSoundTrack`) — the
same guarantee extends to `doSaveSoundTracks`.

## Plumbing added alongside this

- `SoundTrackTarget` / `SoundTrackTargetList` (`audio/common/audiotypes.h`) —
  one `{trackId, dstDevice}` pair per requested stem.
- `IAudioContext`/`AudioContext::saveSoundTracks`,
  `IPlayback`/`Playback::saveSoundTracks`, and `MsgCode::SaveSoundTracks` on
  the RPC layer — plural counterparts to the existing singular
  `saveSoundTrack`, wired identically (same progress/abort channels, same
  `execOperation` synchronization).
