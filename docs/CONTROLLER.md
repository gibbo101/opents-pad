# Controller support

This fork adds a controller control scheme to OpenTS. The guiding reference is
Command & Conquer: Red Alert Retaliation on the PlayStation: list-style menus,
one focused row, left and right to change a value, one button to accept and
one to go back. The Steam Deck is the primary test device, but nothing is
Deck-specific; every screen size from a 7-inch panel to a 32:9 ultrawide is a
target.

## Rules

- `ControlScheme` in `SUN.INI` selects `KeyboardMouse` or `Controller`.
  Under `KeyboardMouse` the game is unchanged. Under `Controller` some
  screens are replaced or extended. Everything added is reversible by
  flipping the setting; no original screen is edited away.
- Controller controls are a scheme of their own, not a mapping of pad buttons
  onto keyboard hotkeys. In game they will be a separate binding table.
- Console-style screens are focus based. Moving a mouse pointer with a stick
  is not acceptable anywhere in menus. A pointer stays right for the tactical
  map, with snapping to units.
- Button prompts show the pad's glyphs: Xbox, PlayStation, or Steam Deck
  styles, with plain text as the fallback. Under Proton every pad arrives as
  an Xbox pad, so beyond the Deck itself, which Steam marks with an
  environment variable, the style is a setting rather than a detection.

## What exists

| Piece | Where | State |
| --- | --- | --- |
| `ControlScheme` option | `code/options.cpp`, manual key page | Done. Defaults to Auto: Controller when a pad is connected by the time the first shell screen shows. Steam hands a game its virtual pad a moment after the window exists, so Auto keeps checking through the startup movies and waits up to two seconds before game select. After that a pad button pressed on a keyboard-scheme shell page switches to Controller and the page shows again as its console version. Switched from the console options screen or the options page's Switch To Controller button. Choosing Keyboard & Mouse from the pad asks for a real key or click within ten seconds, else reverts. Menu and B held for a second switch to Controller from anywhere. |
| Console menu framework | `code/consolemenu.cpp` | Rows of label, value, step, activate, optional icon and swatch strip; a row may sit on a fixed line; long lists scroll. Big menu font, teal focus row, dark panel over the backdrop that a screen can size or drop, held-key repeat, Shift or a shoulder button steps by five, the shell's click as the focus moves. A screen can draw its own content through a canvas, register mouse targets, and run an idle hook each pass; pointing at a row focuses it, a click accepts, and the two prompts are clickable. While a row with an action of its own has focus the accept prompt names that action, such as Edit on a name row or Play on the track row. On the skirmish and LAN host setup screens the pad's start button is the only way to start, with its own glyph and Start at the right of the prompt line; A there only acts on rows with an action of their own, such as Edit on the name, and the A prompt stays blank elsewhere. Select Campaign starts on A alone. |
| Native pad reader | `code/gamepad.cpp` | XInput, loaded at first use. D-pad or left stick, A accept, B back, LB/RB fast step. Under Steam the pad must be on the Gamepad template, not Keyboard & Mouse. |
| Skirmish setup | `code/consoleskirmish.cpp` | Console screen: name (read only), side with faction icon, colour swatch strip, numbered map list with preview, then the numeric rows and toggles. Writes the same session fields as the dialog. |
| Game select page | `code/grphmenu.cpp`, `code/grphmimg.cpp` | Original artwork kept. Spatial d-pad navigation over the buttons, Tiberian Sun selected on entry, unselected discs darkened 70 percent, B or Escape takes the page's Back button. A button still held from the previous screen is not a press. |
| Menu pages | `code/newmenu.cpp` | Under the controller scheme the Tiberian Sun and Firestorm pages are console screens on their own backdrops: New Campaign, Load Mission, LAN, Skirmish, Options and Game Select as rows in the menu font, no box, the page theme playing. Intro and Exit keep their artwork at the corners and light when reached: Up or Right for Intro, Down or Left for Exit. Internet and the tour are left out. Keyboard & Mouse keeps the artwork pages. |
| LAN | `code/consolelan.cpp` | Console screens running the dialogs' discovery and join protocol unchanged. Game list: your name, Host New Game, one row per game found, the lobby count, a joining line while a request is out, and the reason when a join fails. Host lobby: the skirmish rows live, the joined players under the map preview in their colours with faction icon and host or ready mark, Start checks for a second player, everyone ready, and room on the map. Guest lobby: the host's rows read only, own side and colour requested from the host, Ready as the accept, Leave signs off. Each lobby screen has a Chat row showing the latest message that opens a chat screen: the lobby's messages and notices down the page, A to type one on the on-screen keyboard, sent to the game's players or, before joining, to everyone in the lobby. No kick, and the generated map is skipped in the map list. The Keyboard & Mouse dialogs are untouched; the packet handler only skips the guest dialog and message boxes while the console screens are up. |
| Button prompts | `code/padglyph.cpp`, `assets/input-prompts/` | The accept and back prompts on every console screen and the briefing carry the button's glyph from Kenney's CC0 Input Prompts pack, baked into `code/padglyphdata.h` by `tools/padglyphs.py` and sampled down to the prompt's size with alpha. `PromptStyle` in `SUN.INI` picks the set or plain text; Auto is Deck when Steam's `SteamDeck` variable says so, else PlayStation when the system lists a Sony game controller, else Xbox while a pad is connected. Under Proton the host kernel's device list is read through drive Z, which still names the real pad when Steam Input has replaced it with its virtual Xbox one; on Windows the game's own device list is used, where Steam Input's pad reads as Xbox and PlayStation is chosen on the options screen. |
| Options | `code/consoleoptions.cpp` | Console screen: control scheme, button prompts, resolution from the display's mode list, scale mode, integer scaling, stretch movies, game speed, the pad's pointer, fast pointer and stick scroll speeds in place of the mouse's scroll rate and coasting, detail, campaign difficulty, cameo text, action lines, tool tips, and Audio and Controls rows. Accept saves. |
| Controls | `code/consoleoptions.cpp` | Console screen from Options and the pause menu listing what the pad's buttons do, each face button with its glyph, and a note that the bindings are not final until the in-game scheme exists. |
| Audio | `code/consoleoptions.cpp` | Console screen from Options and the pause menu: music, sound and voice volume stepped live, and in play shuffle, repeat, a Now Playing row, a Track row that steps through the songs by name with A on the row playing it, and Stop Music. Accept keeps the volumes and saves; back restores them. |
| Select Campaign | `code/init.cpp` | Side select in the manner of Remastered: the backdrop's own GDI and Nod discs enlarged at left and right, both dim until a side is picked, then the picked one lit and grown with its campaign named under the title; names beneath in faction colours, difficulty along the bottom. Left and right pick the side, or the act when a side has two, with the shell's hover click; A or a click on the lit emblem starts. Writes the difficulty option like the dialog. |
| Load Mission | `code/loaddlg.cpp` | Console list of the save games, newest first, date at the left and description at the right, a star for multiplayer saves; long lists scroll. |
| On-screen keyboard | `code/consolekeyboard.cpp` | A console screen with a text field and a row of digits, three rows of letters and marks, then Space, Delete, Caps, and Done. D-pad or stick moves, A types, X deletes, Y adds a space, B backs out with the text untouched; letters are capitals at the start of each word and lower case after, with Caps locking them on; a real keyboard types straight in, Enter finishes, Escape backs out; the mouse picks keys. Opened by A on the Name row of the skirmish and LAN screens and the Save As row of the pause menu. |
| Play, pointer | `code/gamepad.cpp` | Under the controller scheme in play the left stick and the d-pad move the pointer, the stick with a squared response and the d-pad at a steady rate, a shoulder button speeding either; at the screen's edge the distance the pointer cannot travel scrolls the map, so the edge moves at the pointer's own pace. The three paces come from `PadPointerSpeed`, `PadFastSpeed` and `PadScrollSpeed` in `SUN.INI`, 1 to 10, and the mouse scroll settings leave the pad alone. Cross is the left mouse button and circle a right-button tap at the pointer, so selecting, ordering, deploying, placing, band boxing and cancelling all follow the mouse paths, while a held circle never drag-scrolls. The message loop does not count these clicks as a real mouse. |
| Play, commands | `code/gamepad.cpp` | In PlayStation terms: square cycles the sidebar modes repair, sell, power, waypoint, off; L3 deploys; R3 centres on the base; R2 scatters and R1 with R2 guards; R1 with circle rebuilds the last building; L2 with square, triangle or circle makes team 1, 2 or 3 and L1 with the same selects it and centres on it; R1 with L1 holds the force fire key and R1 with L2 the force move key for the next cross; Select allies with the owner of the selected unit, which the engine only allows against a human player in a game with allies on. The right stick scrolls the map as a right-button drag does. Cross held still for half a second on one of the player's units selects every unit of its type on screen, and held on a little longer widens that to the whole map; held still on the ground it selects every combat unit on screen, and held on longer every combat unit on the map, leaving out harvesters, engineers and vehicles that deploy into buildings. Each of the four says what it selected in the message list at the top left. Each button runs the engine's own named command, so the keyboard bindings stay the source of the behaviour. |
| Play, sidebar | `code/sidebar.cpp`, `code/gamepad.cpp` | Under the controller scheme the sidebar strips are replaced by a fixed grid in the manner of Retaliation: two columns, the player's side on the left and the other side on the right, with rows for structures, infantry, vehicles and aircraft, and a bottom row holding the current superweapon and a cell that cycles to the next. A side's construction yard shows all four of its sections, dulled until the factory exists; without one only the sections whose factory the player holds appear. A section cell shows what it is building, or its last build once idle, or the factory. Triangle takes the pad in and out; the outline is bright inside and dim outside, and it parks where it was left. Inside, the d-pad or stick moves between the cells, the four mode buttons above them and the radar. Cross on a section builds or places the current or last item, or opens the section's grid when it has none; in a grid cross builds the marked item, and the structures grid closes on a build since a yard builds one thing at a time. Circle on a section that is building holds, then cancels; on an idle section it opens the grid; in a grid it cancels the marked build or steps back to the sections. Square opens or closes the grid without cancelling anything. Placing a building, aiming a superweapon or picking a mode button hands focus back to the map with the ghost, target or mode cursor on the pointer, and triangle out of a grid parks the sidebar on that section. From the map, R1 with cross works the parked cell: place, build the last item again, add one more, or queue the marked grid item. On the radar, cross held moves a marker over the map and letting go jumps the view there. The sidebar in Tiberian Sun is always on screen, so Retaliation's pin has no counterpart. |
| Dialogs | `code/gamepad.cpp`, `code/msgloop.cpp` | While any dialog is open the pad drives it through key messages: d-pad as arrows with repeat, A as Enter, B as Escape. Covers the message boxes, Version, and the multiplayer lobby for now. |
| In-game menu | `code/consoleingame.cpp` | Under the controller scheme the Escape menu in play is a console screen of its own, like the main menus: the render size drops to 640x400 on entry and returns to the play size on exit, so the text and artwork match the shell on every panel. "Game Paused" above a box sized to its rows, in the menu pages' colours, on the mission's briefing plate rather than the shell backdrop, focus starting on Return To Mission. Game Options opens the console options screen without its display rows or the campaign difficulty, which the original in-game dialog never applied either; Audio Options opens the audio screen; Controls lists the bindings; Mission Briefing restates; Save Game offers the save in a box under a suggested name, the mission's side and number from its map file ahead of its name for a campaign, and on Save writes a fresh slot, reports beneath the pause box, and posts the message the quick save posts for when play resumes; Load Game opens the console load list; Restart Mission and Abort Mission ask as one row whose value flips between No and Yes, No to begin with, as the C&C pause menu does. In a LAN game Save and Load go through the multiplayer paths and Restart becomes Surrender. The menu loop's pause, resume, restart and exit handling is shared with the dialog. |
| Mission briefing | `code/restate.cpp` | Under the controller scheme the page renders at the shell's 640x400 size so it fills the panel like the console screens, then the size it was opened at returns. The buttons give way to console prompts: A continues or turns the page, B plays the mission video when there is one. Before the mission starts the last page closes with OK; restated from the pause menu it reads Resume Mission. |
| Score screens | `code/mpscore.cpp`, `code/score.cpp`, `code/scenario.cpp` | Under the controller scheme the skirmish and LAN score screen and the campaign score screen render at the shell's size so they fill the display, the prompt reads Continue behind the accept glyph, and the accept button dismisses them. A hall of fame place takes the name on the on-screen keyboard. |
| Shell render size | `code/mainopt.cpp`, `code/init.cpp` | Under the controller scheme the shell renders at 640x400, the size its artwork was drawn for, so the presenter fits it to the panel height with pillars on wide screens. The play resolution returns when a scenario starts. The keyboard scheme keeps the shell at the play resolution. |

## Findings that shape the work

- The engine drops the system's key repeat on purpose, so held keys must be
  polled, not received as events.
- The menu font `FULLFNT3` has three frames per glyph. Frame 2 is the
  readable text; frames 0 and 1 are fade-in bloom layers. Its side palette
  only exists in the campaign side mixes, so it cannot be loaded at the main
  menu; `MSFont::Set_Color` recolours the font from its own palette instead.
- The map preview loader uses `AlternateSurface` as scratch space. Anything
  that caches a backdrop there gets corrupted when the map changes.
- The game select page's backdrop is a movie. Anything painted over it must
  be repainted after every movie frame and must not compound.
- The discs on the game select page have no unlit image; their unlit look is
  part of the backdrop. Only the lit image is separate.
- The shell menus are 640x400 layouts. They fill the screen only when the
  render resolution is 640x400, which on a 16:10 panel is an exact fit.

## Next

1. Untested on the sidebar: the Nod side, both as the player and as the
   captured column; the superweapon row with a weapon charged, cross to fire
   and the arrow to cycle; a grid longer than the panel scrolling; the hall
   of fame name on the on-screen keyboard and the score screen's restore
   after it.
2. The rest of the play scheme: snap to units; R2 with the right stick for
   zoom once the zoom setting exists, with scatter moving to a tap; the
   Controls screen redrawn for play with a button cluster in the manner of
   Retaliation's, which the current list and its "not final" note stand in
   for.
3. A Steam Input layout for the game, shipped in the repo, so the pad is on
   the Gamepad template without the player building one. Steam only defaults
   a layout the app owner publishes, so this is a file to import once, and
   Steam keeps the choice per player after that.
4. LAN lobby extras: kick as a row on the host lobby, and the generated map
   once its setup has a console screen. The lobbies and chat are still
   untested between two machines.
5. A zoom setting for play, on the right stick: the render frame takes the
   panel's aspect and a zoom value picks its height, so no screen shows bars
   and each device tunes its own sprite size. A 32:9 panel at 2x felt right
   in testing.
6. Sidebar shadows: `assets/sidebar-shadows/` and `tools/sidebarshadows.py`
   bake a black silhouette of what a section builds over its cell, and every
   slot is empty until original art exists. The game's own sprites are not to
   be traced for it.
7. Later: the display work in the direction notes.

## Parked

- Arrow keys beep on the Deck under Proton on every press. Present on the
  upstream build too, not caused by this fork.
- The screen flashes white at launch, before the Westwood logo. Stock
  Tiberian Sun does the same under Proton, so it sits below the game. A
  black window brush and a black frame presented right after renderer init
  both failed to remove it. Worth a targeted look at the end of the project.
- Held-key map scrolling uses the edge-scroll speed for the `ScrollRate`
  option and may want a rate of its own.
