# Echo Stone

Echo Stone adds screen-reader access to **Marvel vs. Capcom: Infinite** for
blind players. It uses Prism for menus and prompts and provides combat cues,
health and gauge announcements, and recorded Story descriptions.

This beta supports the Windows x64 Steam build 18789511.
[Published releases](https://github.com/buu420/echo-stone/releases).

## Game language

Echo Stone follows the language the game loads. Native menu text and subtitles
come from the game. Echo Stone translates its own labels, command icon
descriptions, and combat announcements. The language list includes English,
Arabic, French, German, Italian, Japanese, Korean, Polish, Brazilian Portuguese,
Russian, Spanish for Spain and Latin America, Thai, and Simplified and
Traditional Chinese.

Choose a suitable voice in your screen reader. The recorded Story descriptions
are in English, and F10 controls their playback. The translations are beta
wording and still need live listening and native-speaker review.

| Key | Action |
| --- | --- |
| F5 | Track Player 1 or Player 2; starts with Player 1 |
| F6 | Turn the opponent distance beep on or off; starts on |
| F7 | Read both active fighters' health, your tracked team's side and gauges, and visible Story enemy/time counters |
| F8 | Repeat the current menu, prompt, visible Mission commands or Story text |
| F9 | Turn automatic side announcements on or off; starts on |
| F10 | Turn recorded movie descriptions on or off; starts on |

The same 800 Hz, 80 ms tone used by the FighterZ mod speeds up as the visible
fighters get closer. Pulse timing follows FighterZ's near-range curve, from
150 ms near to 1500 ms far. Its scale is the opening separation for the current round;
it does not describe attack reach. Side means your tracked team is left or right
of the opponent. Tagging keeps the selected team.
Both cues start enabled each time the game starts. F9 controls automatic side
announcements; F7 still includes side when you request combat status.

Both active fighters' health is announced at the start of play, on tags, and
when health crosses a ten-point boundary. F5 chooses which fighter is called
"your" fighter. F7 reads both current percentages on demand.

Hyper Combo announcements report newly gained stocks and call out the one- and
three-bar readiness thresholds. Infinity announces Storm readiness, a full gauge,
and Storm activation and end. During a Storm, F7 reads the displayed draining bar
percentage. Menu, pause, focus loss and stale captures silence the distance beep.
Visible health and gauges remain readable during combat cinematics.

The save-data creation dialog includes its full overwrite warning and Yes/No
action guide. Make that choice yourself using the game's normal controls.
This mod does not press buttons, choose options, alter gameplay or create saves.

In single-controller Character Select, narration follows the side currently
receiving input, including the opponent team in Training. F5 continues to
choose the team tracked during combat. Collection reads focused entries,
visible New and unavailable indicators, displayed details, and visible control
guides in the character and artwork viewers.

Missions reads the CAPCOM, MARVEL, ALL and TUTORIAL pages, the focused
character/category, mission slot, progress, visible mission details and the
OK/Cancel confirmation. Tutorial text follows the displayed text and button
icons in order; unrecognized glyphs are described as a button icon. When details
are hidden, the mission slot comes from the visible cursor; hidden descriptions
are not read. After entering a Mission, Prism reads the tutorial window and the
drawn objective. Press F8 during the Mission for the objective and the currently
visible command rows, including move icons and the highlighted command.

Story reads visible location/date cards, loading and advice text, pause actions,
win and character conditions, Continue actions, Game Over, and the mapped English and Simplified Chinese
Thank You screens. The Command List in the pause menu reads the current fighter,
fully visible rows, move icons, and scroll cues. F7 requests
only the enemy/time digits or infinity symbol actually drawn on the Story HUD.
These extra HUD reads run on demand. F8 can repeat the currently displayed
subtitle; dialogue is not automatically spoken over the game's actors.
Loading is announced once per episode, including startup. It rearms when the
game presents the next screen, movie or playable fight. F8 repeats it on request.

Pause menus include the drawn settings and action guides, including Training
recording slots and durations. Arcade stage captions and visible offline results
use the game's displayed text. Online readers follow the highlighted lobby,
room choice, member, ranking, profile and replay rows. Live online sessions are
needed to check what the server populates and what Prism speaks in those menus.

The mod includes 469 descriptions across all 80 Story movie files. They cover
key action, visual reveals and scene changes. The 30 earlier
recordings keep their volume and timing. Descriptions use dialogue gaps where
possible, with brief overlaps for essential action. The original movies and
their audio remain intact. The real-time sequences before some movies do not
have recorded descriptions.
Descriptions use the game's movie clock, yield to Prism narration, and stop
or suspend on pause, skip, loss of focus or an unavailable clock. F10 changes
only these recordings.
No voice service, reference recording or account credential is needed at runtime.
Native dialogue and sound are left under the game's normal audio controls.

For screen coverage and the distinction between automated checks and live
verification, see `docs/coverage.md` in the source checkout. First local testing
still needs user-driven menu transitions, combat, tagging and controller checks.
Story scope, source evidence and remaining live checks are documented in
`docs/story-accessibility.md`. New description audibility and synchronization
still need an in-game listening check.

## Install and remove

### Windows ZIP

1. Close MVC Infinite normally and extract `EchoStone-0.1.2-Windows.zip`.
2. Run `Install-EchoStone.cmd` in the extracted folder. It finds the Steam
   game folder and installs the mod automatically.
3. Start your screen reader and launch MVC Infinite through Steam.

The package includes the ASI loader, Prism, and the Story recordings. Use the
same command for upgrades. It checks the game build and package files, keeps
an installation receipt for removal, and backs up replaced mod files. It
stops if it finds a conflicting file.

To remove the mod, close the game and run this command in the extracted folder.
Replace the example path with your game folder.

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\tools\Uninstall.ps1 -GamePath "C:\Path\To\MARVEL VS. CAPCOM INFINITE"
```

Removal preserves edited files, shared loaders, logs, backups, saves and settings.

### Amethyst Accessibility Mod Manager

With the game closed, find **Echo Stone** under **Marvel vs. Capcom: Infinite**
in the [buu420 catalog](https://github.com/buu420/buu-s-mods). Choose Install
and allow the read-only compatibility check. You can install over an intact
Echo Stone 0.1.0, 0.1.1, or 0.1.2 Windows ZIP installation. If you do, Amethyst
backs up that copy and restores it when you uninstall through Amethyst.

Use Amethyst for updates and removal while it manages Echo Stone. Close the
game before installing, updating, or removing the mod.

## Troubleshooting

Diagnostic log: `MVCI\Binaries\Win64\scripts\MVCIAccess\reader.log` beneath
the game folder. `LANG` records the game culture used for mod speech.
`SPEAK accepted=1` means Prism accepted a line. For Story speech,
`ACK` distinguishes backend acceptance from the mod remembering that line.
Hearing the line is a separate check. Unsupported game updates
disable native capture and are announced when Prism is available.
`CAPTURE` lines also report work time, per-stage timings and discovery counters
for investigating loading delays. Discovery is shared between readers; captures
leave a gap after completing so slow work cannot continuously occupy the game thread.

## Build and provenance

Build the x64 mod from the source checkout with MSVC and CMake.
The ZIP includes the matching source under `source`. To build the plugin from
that copy, configure with `cmake -S source -B build -A x64 -DBUILD_TESTING=OFF`,
then run `cmake --build build --config Release`.

The pinned loader uses its eager compatibility mode for this protected game.
An explicit empty extra-plugin setting also keeps discovery in the Win64 folder
when the root launcher uses a different working directory. Plugin initialization
starts a worker without waiting; the worker waits for the
real Unreal window and verified code before hooking. The default deferred loader
mode did not initialize the plugin in the first live startup test.

```
cmake -S . -B build -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
py -3.12 tools/package.py --amethyst
```

For the full test suite and packaging, copy the pinned `dinput8.dll` and
`prism.dll` from the Windows ZIP's payload to `dependencies/ual-x64/dinput8.dll`
and `dependencies/prism/prism.dll`, respectively. The packager checks their
hashes. The Windows ZIP includes the matching buildable plugin source.

GPL-3.0. Audio code/waveform is adapted from the user's Fighting Chance and
FighterZAccess projects. Third-party notices are in `LICENSES` and
`dependencies/versions.json`. The runtime uses the
[Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader),
[MinHook](https://github.com/TsudaKageyu/minhook),
[PicoSHA2](https://github.com/okdshin/PicoSHA2), and [Prism](https://github.com/ethindp/prism). Original game
executables, assets and decompiled code are not included.
