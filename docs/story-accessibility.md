# Story accessibility and loading investigation

This change targets the unchanged Steam 18789511 executable. Native capture is
read only, runs after the outermost native UI event on the game window thread,
and copies values for the narration/audio workers. It never calls Story save,
progress, travel, playback, combat or input actions.

## Captured information

- Movie pause: the displayed caption and direct Resume/Skip or Resume/Back
  actions. There is no selected row to invent on this screen.
- Story pause: visible win conditions, character conditions/buffs and rendered
  special-rule text. Active child popups take precedence.
- Command list: the declared active parent's popup, settled character page,
  fully visible rows, rendered names, command glyphs, notice and navigation
  text. The native focusIndex is a scroll target, not a selected move. Partial
  rows and moving layouts wait for a settled capture. Unknown glyphs make the
  document unavailable instead of guessing a command.
- Location/date cards, one-point advice in drawn platform/slot order, loading
  text, Continue direct actions, Game Over and the base English Thanks texture.
  Canned image text requires a verified resource, current flow, owner and shown
  active widget. The Thanks phrase was independently viewed in the decoded
  shipped texture; its localized replacement is not mapped.
- F7 Story counters: the current-world, shown Story HUD's drawn enemy digits,
  finite timer digits or infinity symbol. Material parameters, attachment,
  lifecycle and owner guards reject hidden/logical counters. These additional
  reads run only while an F7 request awaits a native capture and speech delivery.
- F8: the currently rendered subtitle and other visible Story text. Subtitle
  audio is opt-in through F8; native actor dialogue is not automatically repeated.

Every reader rechecks relevant object serials, declared bindings, world,
visibility, values and ownership before publishing. Ambiguous, replaced or
uninitialized owners remain unavailable. Menu/modals retain speech priority.
Informational text becomes acknowledged only after Prism accepts it. A fresh
submission still counts if a slow backend returns after the capture expires;
a stale proposal is not submitted. Loading keeps its acknowledgment through
stale observations and internal hide, detach or owner changes. Only a readable
replacement menu/dialog, presented playing movie or playable fight rearms it,
with no current loading caption or known drawn loading overlay. Failed
delivery remains retryable, and F8 can explicitly repeat the current loading
text. Other captions retire when their native source disappears. Existing
combat cues and F5-F9 remain.
When loading is visibly drawn but its text is unreadable or two drawn owners
are present, that observation still keeps the episode active. Ambiguous or
changing text is never spoken.

Location/date captions, subtitles and loading text also require the researched
TextBlock SlateColor alpha to be above zero. Enum visibility alone admitted
invisible retained Bifrost/Doctor text that could interrupt recordings. Nested
reflected properties, type/offset bounds, color-use mode and late value/owner
checks gate these particular Story fields; unknown color rules stay unavailable.

## Recorded descriptions

The pack covers all 80 shipped Story movie files, including the gaps encountered
in `0-B_DEMO04`, `0-B_DEMO08` and `1-B_DEMO01`. It adds 439 lines and retains
the 30 previous recordings and windows exactly. Descriptions focus on visible
actions, expressions and scene changes, using recognizable character names.
Descript sampled the early batches in a private review project. When the
service returned only timeline information, local half-second sheets and
closer frame checks supplied the visual evidence.

The approved Echos of Gaia personal voice references and `s2.1-pro-free` renderer
were reused read only. Job inputs, accepted and rejected takes, and timing
evidence stay in private research. The public pack contains 469 canonical
24 kHz mono PCM16 WAVs, a manifest and the complete movie ledger. New clips
use the same gain as the prior update, with measured true peak at or below
-1 dBTP and unchanged sample counts. Timing or word-check failures are
reviewed and, where necessary, rerecorded. No time stretching, native mixer
change or runtime voice service is needed.
For five lines naming Jedah, native subtitles establish the name and dialogue
provides a pronunciation reference. Before and after mastering, the chosen
takes match the unguided recognizer's spelling of the native pronunciation and
pass exact recognition with the name in the lexicon. The original rejection
records remain available.

New recordings leave at least 200 ms of dispatch room inside the reviewed
windows. The actual compiled catalog is tested
across all one-millisecond phase offsets at 50/80/100 ms native capture cadences,
a 20 ms worker and a 10 ms driver delay. Every line must start once. Playback's
conservative dialogue-expiry fence remains intact; long stalls can still skip
a line. This fixture proves scheduling headroom under those conditions, not
live synchronization or immunity to every frame hitch.

Cues prefer gaps in the native subtitle and locally detected mixed/centre
channel speech. When essential action occurs during speech, a short line plays
over the original audio with the user's approval. ASR and subtitle absence do
not prove silence; unknown native mix parameters are treated conservatively.
In-game dialogue coexistence remains a listening check. Twelve real-time
pre-rolls are outside the movie files and have no recorded descriptions.
Original movies/audio, private references, credentials, rejected takes and
service IDs are excluded from the package.

Descriptions start enabled. F10 toggles these recordings. Playback requires a
fresh foreground capture, the current drawn movie and the native timestamp of
its presented frame. The sequence supplies scene ownership and pause state. The
frame timestamp schedules the recordings even when the sequence cursor drifts.
When the native soundtrack clock advances within the current movie, it can
expire lines that would play late while the picture catches up. A changed
sync origin or a backward seek requires the mod to observe that clock again.
Native pause or a stalled frame suspends playback. Skipping, seeking, a hidden
or replaced movie, stale state, focus loss, disabling descriptions or Prism
interruption cancels or suspends playback. The scheduler skips expired lines
and checks elapsed time before submitting audio to the driver.

One bounded recording bank loads and hashes on a dedicated background worker.
The UI hook and narration/combat worker perform no WAV file reads or hashing.
Only this mod's waveOut handle/buffers are controlled; uncertain driver-owned
buffers and their module stay alive after a failed reset/unprepare. There is no
voice service call during gameplay. Coverage/timing/load/output failures have
one-time spoken notices where a current movie can be identified.

## Loading evidence and limits

The earlier Candidate 10 session brackets first Collection menu loss to first
destination speech at 38.782 seconds, versus 1.157 seconds on a repeat visit.
Those are narration boundaries, not exact button/loading times.

The earlier Candidate 11 session was isolated from the append-only historical log.
Its 38 periodic samples have mean capture work of 0.250-3.555 ms and a 15.877 ms
maximum, with no capture exceptions or discovery failures. It submitted loading
speech 50 times, as little as 46 ms apart. The new loading-episode regressions
cover the observed stale/empty/reacquired-state pattern without consuming failed
speech or suppressing a positively new load.

The latest Candidate 12 session had 22 accepted loading submissions, four
before the first network initialization dialog. Its 84 periodic samples show
mean capture work of 0.570-7.947 ms and a 93.543 ms maximum. Static inspection
found that internal travel can hide and show the same loading widget, and that
a slow Prism call could accept a line while the mod discarded its
acknowledgment. Candidate 13 handles both cases. Its worker logs loading state
changes and separates speech submission age, backend time and acknowledgment.
The earlier log did not record acknowledgment, so it cannot attribute each
repeat to one specific cause.

After the first movie, the first known loading observation to combat-present
boundary is 37.671 seconds; to combat-playable it is 49.031 seconds. The second
pair is 7.922 and 21.453 seconds. These brackets include native transitions and
advice/readiness time, not only disk loading. The recording bank was already
idle during the long first fight load; it was not continuing WAV reads then.
This does not rule out every source of mod overhead.

The generic reader visits an ordered, deduplicated shortlist of menu families
instead of examining every retained UserWidget. It now reuses the discovery
catalog's coarse class masks before entering unrelated adapters. Every matching
adapter retains its live class, visibility, owner and lifetime checks; unknown
or unmanaged candidates take the original validation path. In a second fixture
with 100 retained offscreen windows and one title, first-capture native reads
fall from 19,803 to 12,531 with the same title result. The catalog adds no new
registry scan or pointer reads for this filter.

Incremental discovery, event hints, periodic refresh and completion-plus-33-ms
pacing remain. Story HUD reads are on demand; recording I/O stays off the game
thread. No native minimum-wait timer, loading budget or gameplay setting changes.

A copied-memory benchmark with 100,000 unrelated assets, 200 retained widgets
and one appended asset per later capture measured the frozen Candidate 10
pipeline at 355-361 microseconds per repeated capture, versus 25-54 microseconds
with the new pipeline including idle Story capture. Reads fell from 12,273 to
686. Initial capture was 3,749 versus 2,809 microseconds. This empty-menu fixture
proves reduced irrelevant work, not a real Collection or fight loading time.
Research retains raw samples and exact benchmark binary hashes.

Windows identifies D: as a TOSHIBA EXTERNAL_USB disk on USB. Its current healthy
status does not measure load latency or establish the drive as the cause.
There is no matched cold unmodded baseline from this PC. The cold Collection
and post-video delay remain unattributed; neither the mod nor the PC is
established as their full cause. A future user-driven comparison should
keep the same machine, saves, cold/repeat distinction and settings. New copied
STORY phase/clock diagnostics help bracket that test without driving the game.

## Verification boundary

Native/Blueprint/Ghidra research, frame review, copied-memory fixtures, corrupt
input/driver failure tests and package checks are separate from live proof.
During the user-driven pause/resume check, the native sequence cursor changed
from 13.3067417 to 21.7830029 seconds and held paused over five later reads.
The bounded trace had ended before movement: this is before/after evidence with
user confirmation, not continuous movement. It occurred in a real-time pre-roll,
so that particular trace did not verify the active Mana movie clock. The later
Candidate 11 play session logs two Xgard description playback intervals with
`timing_bad=0`; the user reported that descriptions seem to work. Neither the
log nor that general report proves every clip's audibility or dialogue spacing.

Still needed: in-process listening on the new Story surfaces, actual F7 HUD
materials/lifecycle transitions, command-list geometry/FText behavior, audible
recordings with native pause/skip/focus changes and dialogue, and matched loading
tests. Full ending credits, all Story gauges/special mechanics, all controller
glyphs, non-English Thanks art and narration of real-time Story sequences are
not claimed. Native timers, saves and original media remain unchanged.
