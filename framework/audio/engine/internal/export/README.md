# Audio export: single file vs. several files in parallel

This directory has two writers. Both render the audio engine offline and encode
the result, but with a different scope:

- `soundtrackwriter.{h,cpp}` — renders the mixer once and encodes its output to
  one file. The original path, used for a normal single-file export (full score
  or one part).
- `parallelsoundtrackwriter.{h,cpp}` — renders several files at the same time
  (typically one per part) on a pool of worker threads. Used by
  `IPlayback::saveSoundTracks` for "export all parts" style exports, instead of
  one full render per file.

Both share encoder construction via `encoderfactory.{h,cpp}`.

## What each file of a parallel export contains

Exactly what the single-file path produces for that file when every other
track is muted: the file's own tracks, plus their aux sends run through the aux
channels (e.g. reverb), mixed the way `Mixer::process()` mixes them.

Like the single-file path, which renders `m_mixer` rather than the master track
chain, the master fx chain and master volume are **not** applied. The parallel
path deliberately matches that so both paths produce the same audio.

## How it works

`SoundTrackTarget` is one output file and the list of tracks that go into it.
`AudioContext::saveSoundTracks()`:

1. waits for online sounds to finish processing (same as `saveSoundTrack`),
2. validates the targets: a track may only belong to one target, since each
   target is rendered by one worker and a track can't be processed by two
   threads at once,
3. creates, per worker, a copy of every aux channel that at least one exported
   track sends to (active send above 0%) — see below — and waits until the
   copies are fully loaded,
4. runs `ParallelSoundTrackWriter` inside `execOperation`, like the
   single-file export, so the real-time driver doesn't touch the graph
   meanwhile,
5. releases the copies.

The writer starts one thread per worker (`std::thread::hardware_concurrency()`,
at most one per file). Each worker takes the next file from the queue until
the queue is empty, and renders it block by block:

- the file's tracks are **borrowed** from the live mixer (their synths, fx and
  volume/pan are used as is; nothing is reloaded),
- each track's output is added to the file's mix and, per its aux sends, to
  the worker's copy of the aux channels,
- the aux copies are processed and added to the mix,
- the mix is encoded.

The engine thread meanwhile only reports progress and handles incoming
messages (e.g. abort).

## Aux channel copies

The aux channels are shared by every track, so workers can't share the live
ones. `IAudioFactory::makeFxChainCopy()` / `IFxResolver::createFxListCopy()`
create new effect instances with the same settings (for VST: the same
component state) under a unique copy id, bypassing the per-track instance
cache of `resolveFxList()`, which would otherwise return the live instances.
`releaseFxChainCopy()` unregisters them afterwards.

VST instances load asynchronously on the main thread, and a `VstFxProcessor`
passes audio through untouched until it's loaded, so the export waits until
`FxChain::isReady()` for every copy (with a timeout, after which the export
fails rather than silently exporting without the effect).

A worker reuses its copies for every file it renders, calling
`FxChain::resetState()` (`IFxProcessor::resetState()`) before each file so no
reverb tail or compressor state carries over from the previous file.

## Threading notes

- Worker threads must not send async channel messages to the engine thread:
  the first send from a thread registers it with the engine thread's message
  queue, which blocks while the engine thread is itself blocked waiting for
  the workers (deadlock). Nodes that send UI updates while processing skip them
  in `ProcessMode::PlayingOffline`: level meters (`SignalNode`), automation
  values (`AutomationControlNode`) and VST transport events
  (`VstAudioClient::setIsOffline()`). The copies' own channels have no
  receivers, so they never send.
- During export the context player's position stays at 0 (see
  `ContextPlayer::seek`), for the single-file path as well, so automation and
  plugin transport see the same position in both paths.
