# Controller support

This fork adds a controller control scheme to OpenTS. The guiding reference is
Command & Conquer: Red Alert Retaliation on the PlayStation: list-style menus,
one focused row, left and right to change a value, one button to accept and
one to go back. The Steam Deck is the primary test device, but nothing is
Deck-specific; every screen size from a 7-inch panel to a 32:9 ultrawide is a
target. [CONTROLS.md](CONTROLS.md) is the player's reference for every
button and owns what each piece does; the table below records how each is
built and where it stands.

## Rules

- `ControlScheme` in `SUN.INI` selects `KeyboardMouse` or `Controller`, or
  `Auto`, its default, which takes `Controller` while a pad is connected.
  Under `KeyboardMouse` the game is unchanged. Under `Controller` some
  screens are replaced or extended. Everything added is reversible by
  flipping the setting; no original screen is edited away.
- Controller controls are a scheme of their own, not a mapping of pad buttons
  onto keyboard hotkeys. In play each button runs the engine's own named
  command, so the keyboard bindings stay the source of the behavior; the
  pad's chords are fixed, with no binding table of their own.
- Console-style screens are focus based. Moving a mouse pointer with a stick
  is not acceptable anywhere in menus. A pointer stays right for the tactical
  map, with snapping to units.
- Button prompts show the pad's glyphs: Xbox, PlayStation, or Steam Deck
  styles, with plain text as the fallback. Auto knows the Deck from Steam's
  `SteamDeck` environment variable and a pad's maker from the system's
  device list; under Proton every pad arrives as an Xbox pad, so the host
  kernel's list is read through drive Z, where the real pad is still named
  behind Steam Input's virtual one. On Windows a Steam Input pad reads as
  Xbox, so there PlayStation is a setting rather than a detection.

## What exists

| Piece | Where | State |
| --- | --- | --- |
| `ControlScheme` option | `code/options.cpp`, manual key page | Done; [Choosing the scheme](CONTROLS.md#choosing-the-scheme). Steam hands a game its virtual pad a moment after the window exists, so Auto keeps checking through the startup movies, and waits up to two seconds before game select only when the system lists a controller-class device or a pad was seen earlier. A settings file without the key reads as Auto; this is an intended change for existing installs, recorded in the manual's change record with the `KeyboardMouse` line as the way back. |
| Console menu framework | `code/consolemenu.cpp` | Rows of label, value, step, activate, optional icon and swatch strip; a row may sit on a fixed line; long lists scroll. Big menu font, teal focus row, dark panel over the backdrop that a screen can size or drop, held-key repeat, Left Shift or a shoulder button steps by five, the shell's click as the focus moves. A screen can draw its own content through a canvas, register mouse targets, and run an idle hook each pass; pointing at a row focuses it, a click accepts, and the two prompts are clickable. While a row with an action of its own has focus the accept prompt names that action, such as Edit on a name row or Play on the track row. A screen can give the pad's menu button a job done from any row, shown mid-line with its glyph. Cross accepts from any row without an action of its own. |
| Native pad reader | `code/gamepad.cpp` | XInput, loaded at first use. D-pad or left stick, cross accept, circle back, L1 and R1 fast step. Under Steam the pad must be on the Gamepad template, not Keyboard & Mouse. |
| Skirmish setup | `code/consoleskirmish.cpp` | Console screen: name (read only), side with faction icon, color swatch strip, numbered map list with preview, then the numeric rows and toggles. Writes the same session fields as the dialog and seeds the game speed from the saved option; the LAN host screen seeds the session the same way. |
| Game select page | `code/grphmenu.cpp`, `code/grphmimg.cpp` | Original artwork kept. Spatial d-pad navigation over the buttons, Tiberian Sun selected on entry, unselected discs darkened 70 percent, circle or Escape takes the page's Back button. A button still held from the previous screen is not a press. |
| Menu pages | `code/newmenu.cpp` | Under the controller scheme the Tiberian Sun and Firestorm pages are console screens on their own backdrops: New Campaign, Load Mission, LAN, Skirmish, Options and Game Select as rows in the menu font, no box, the page theme playing. Intro and Exit keep their artwork at the corners and light when reached: Up or Right for Intro, Down or Left for Exit. Internet and the tour are left out. Keyboard & Mouse keeps the artwork pages. |
| LAN | `code/consolelan.cpp` | Console screens running the dialogs' discovery and join protocol unchanged; the screens are in [LAN](CONTROLS.md#lan). A joining line shows while a request is out. The Keyboard & Mouse dialogs are untouched; the packet handler only skips the guest dialog and message boxes while the console screens are up. |
| Button prompts | `code/padglyph.cpp`, `assets/input-prompts/` | The glyphs come from Kenney's CC0 Input Prompts pack, baked into `code/padglyphdata.h` by `tools/padglyphs.py` and sampled down to the prompt's size with alpha. `PromptStyle` in `SUN.INI` picks the set; the Auto order is in [Button prompts](CONTROLS.md#button-prompts). |
| Options | `code/consoleoptions.cpp` | Console screen; the rows are in [Options, Audio and Controls](CONTROLS.md#options-audio-and-controls). The Zoom row steps the pad's render height in place of a resolution list, the three pad speed rows and Unit Snap stand in for the mouse's scroll rate and coasting, and the volumes are on the Audio screen. Accept saves. |
| Controls | `code/consoleoptions.cpp` | Console screen from Options and the pause menu: a drawn pad with callouts, paged across the face buttons, the shoulders and triggers, and the sticks. |
| Audio | `code/consoleoptions.cpp` | Console screen from Options and the pause menu; volumes stepped live, and in play the track controls of the sound dialog. Accept keeps the volumes and saves; back restores them. |
| Select Campaign | `code/init.cpp` | Side select in the manner of Remastered: the backdrop's own GDI and Nod discs enlarged at left and right, both dim until a side is picked, then the picked one lit and grown with its campaign named under the title; names beneath in faction colors, difficulty along the bottom. Left and right pick the side, or the act when a side has two, with the shell's hover click; cross or a click on the lit emblem starts. Writes the difficulty option like the dialog. |
| Load Mission | `code/loaddlg.cpp` | Console list of the save games, newest first, date at the left and description at the right, a star for multiplayer saves; long lists scroll. |
| On-screen keyboard | `code/consolekeyboard.cpp` | A console screen with a text field and a row of digits, three rows of letters and marks, then Space, Delete, Caps, and Done; the buttons are in [On-screen keyboard](CONTROLS.md#on-screen-keyboard). Opened by the menu button on the skirmish and LAN games screens and on the pause menu's Save As box, and by cross on the LAN games list's Name row. |
| Play, zoom | `code/mainopt.cpp`, `code/gamepad.cpp` | Under the controller scheme play renders at a zoom, not the keyboard scheme's resolution; the ladder and baseline are in [Zoom](CONTROLS.md#zoom). `Play_Display_Mode` settles it from `PadZoomWidth` and `PadZoomHeight` in `SUN.INI`, falling back to the baseline whenever the saved pair is empty or off the panel's shape. The pad records each step and the main loop applies it beside the in-game menu, since the pump can run mid-draw. The stepped size is saved once the stepping stops; nothing is announced on screen. The tracking window never follows the frame under the controller scheme; the drawable area is the panel. |
| Play, sidebar size | `code/video.cpp`, `code/bgfxbackend.cpp`, `code/vidscale.cpp`, `code/mainopt.cpp` | Under the controller scheme the sidebar is presented apart from the frame, so its share of the screen stops changing with the zoom. The sidebar surface is allocated at the height of the pad's own panel, the radar and mode buttons over five rows of two cells and the bottom cap, read from the sidebar art's top, middle and bottom pieces without the add-on foot plate, 449 in the game's art, whatever height the map renders at, with `Max_Visible` held to five rows under the scheme, and the presenter draws it as a second quad scaled to fill the panel's height beside the map's quad, so the five rows always fill the screen; the frame's own sidebar columns are never shown, and its map columns take the shape of the panel left beside the sidebar, so `Pad_Zoom_Width` adds the sidebar's 168 columns to a map width on that shape. The point conversions in `vidscale.cpp` map each quad back to its own columns, and a position in the sidebar's columns is held to the sidebar's height rather than the frame's, since the two differ away from the baseline. The sidebar's own blit into the frame is skipped and only marks the frame due. Keyboard & Mouse keeps the sidebar in the frame. |
| Play, top bar | `code/tab.cpp`, `code/video.cpp`, `code/vidscale.cpp`, `code/mainopt.cpp` | Under the controller scheme the top bar is kept, in the manner of C&C1, but presented apart from the frame like the sidebar: `TabSurface` is drawn at the sidebar's scale across the map's part of the screen, so the bar never changes size with the zoom, and the frame's own top rows are never shown. The bar carries the Options tab at its left, with the pad's menu button glyph before the label and the pair centered on the tab, the beam, and the mission timer's tab at its right; the credits stay on the sidebar. Its own width follows from the screen and the sidebar's scale, `BarWidth` in the scale info, so the tab bar code draws into it at that width. Mouse positions on the bar are brought onto the frame's top rows with their x scaled to the map's columns, and the tab's hit width is scaled the same way. The map's part of the frame is rendered for the screen left beside the sidebar and under the bar, so `Pad_Zoom_Width` fits the map's rows to that. The keyboard scheme keeps the bar in the frame as it was. |
| Play, pointer | `code/gamepad.cpp` | The buttons, paces and snap are in [On the map](CONTROLS.md#on-the-map). The snap resolves the pointer's cell as a click does and scans the cells around it, since the engine's `Close_Object` only reaches a quarter cell. The paces are in map pixels against a 600-high yardstick. The pad is polled from the frame loop, so the game thread only sets a velocity and a box each poll and a 60 Hz multimedia timer moves the OS pointer, which keeps the steps small however slowly frames come; the timer thread attaches its input to the game thread's so the cursor shape stays the game's, and banks the travel the box refused for the game thread to scroll by. Cross is the left mouse button and circle a right-button tap at the pointer, so selecting, ordering, deploying, placing, band boxing and canceling all follow the mouse paths. The message loop does not count these clicks as a real mouse. |
| Play, commands | `code/gamepad.cpp` | The chords are in [On the map](CONTROLS.md#on-the-map), [Teams](CONTROLS.md#teams) and [Waypoints](CONTROLS.md#waypoints). Each button runs the engine's own named command, so the keyboard bindings stay the source of the behavior. The double and triple taps count a unit as a combat unit when it carries a weapon or gains one by deploying, and each of its four selections announces itself in the message list. |
| Play, sidebar slide | `code/sidebar.cpp`, `code/video.cpp`, `code/gamepad.cpp`, `code/tab.cpp`, `code/tactical.cpp` | Under the controller scheme the sidebar starts away from the map, in the manner of Retaliation and C&C1 on the PlayStation: the map takes the whole width under the bar at all times and the panel only ever slides over it, 200 ms from the presenter, drawn last over the map and the bar, so showing or hiding it never moves the view. The generic click plays as it sets off in and out. The buttons and the sticky state are in [Sidebar](CONTROLS.md#sidebar); the sticky state is saved as `PadSidebarSticky` in `SUN.INI` and restored on the next scenario's first tick. Nothing but the pad opens the panel, as in C&C1 on the PlayStation, where deploying the MCV leaves the panel away until the player calls it. While the panel is in, the view may scroll past the map's edge by the strip the panel covers, `ViewCoveredRight` on the tactical view, and the pad's pointer stops at the panel's edge; positions on the panel reach it only once it is fully in. The bar ends in a Sidebar tab carrying the triangle glyph, both tab glyphs drawn fitted to their content since the menu button's art carries more margin than a face button's, and a mouse click on it slides the panel in; the panel covers that tab when in. The credits are the panel's alone, on its own strip, so the bar shows none; the timer's tab keeps left of the Sidebar tab, or of the panel once it covers that. A box dragged past the view's edge selects by map position, since only what was drawn is in the tactical view's selectable list. |
| Play, sidebar | `code/sidebar.cpp`, `code/gamepad.cpp` | Under the controller scheme the sidebar strips are replaced by a fixed grid in the manner of Retaliation; the layout and buttons are in [Sidebar](CONTROLS.md#sidebar). Each structures cell carries its side's emblem in the top right. The player's construction yard shows all four of their side's sections, dulled until the factory exists; the other side's sections appear as their factories are captured, and its structures section once any of their buildings is held, since the game's one yard type is owned by both sides and stands as the face of either structures cell. Leaving a grid with nothing building puts the factory back on the section cell. The outline is bright inside and dim outside, and every cell's caption starts three pixels in from the cell's edge, clear of the outline, which runs along the edge itself. R1 with triangle is Retaliation's pin, the sticky panel in the sidebar slide row above. |
| Play, cheats | `code/gamepad.cpp` | Retaliation's cheat codes, keyed in on the sidebar's four mode buttons; the codes are in [Cheats](CONTROLS.md#cheats). The last six symbols are matched against the codes, a match plays the options sound and announces itself in the message list, and the mode buttons stand in for Retaliation's glyph row in its order so each code is walked as on the PlayStation. Solo games only. A superweapon the player holds is charged; one they lack is granted for a single shot. |
| Dialogs | `code/gamepad.cpp`, `code/msgloop.cpp` | While any dialog is open the pad drives it through key messages: d-pad as arrows with repeat, cross as Enter, circle as Escape. Covers the message boxes, Version, and the multiplayer lobby for now. |
| In-game menu | `code/consoleingame.cpp` | Under the controller scheme the Escape menu in play is a console screen of its own, like the main menus: the render size drops to 640x400 on entry and returns to the play size on exit, so the text and artwork match the shell on every panel. "Game Paused" above a box sized to its rows, in the menu pages' colors, on the mission's score plate rather than the shell backdrop, the campaign's or the multiplayer one, focus starting on Return To Mission. The rows are in [Pause menu](CONTROLS.md#pause-menu). Save Game takes the mission's side and number from its map file for the suggested name and posts the message the quick save posts for when play resumes. The menu loop's pause, resume, restart and exit handling is shared with the dialog. |
| Mission briefing | `code/restate.cpp` | Under the controller scheme the page renders at the shell's 640x400 size so it fills the panel like the console screens, then the size it was opened at returns. The buttons give way to console prompts; the labels are in [Menus and screens](CONTROLS.md#menus-and-screens). |
| Score screens | `code/mpscore.cpp`, `code/score.cpp`, `code/scenario.cpp` | Under the controller scheme the skirmish and LAN score screen and the campaign score screen render at the shell's size so they fill the display, the prompt reads Continue behind the accept glyph, and the accept button dismisses them. A hall of fame place takes the name on the on-screen keyboard. |
| Shell render size | `code/mainopt.cpp`, `code/init.cpp`, `code/scenario.cpp` | Under the controller scheme the shell renders at 640x400, the size its artwork was drawn for, so the presenter fits it to the panel height with pillars on wide screens. The loading screen is shell artwork too, so a scenario start switches to 640x400 for it and the play size returns once the scenario is read. The keyboard scheme keeps the shell at the play resolution. |

## Findings that shape the work

- The engine drops the system's key repeat on purpose, so held keys must be
  polled, not received as events.
- The menu font `FULLFNT3` has three frames per glyph. Frame 2 is the
  readable text; frames 0 and 1 are fade-in bloom layers. Its side palette
  only exists in the campaign side mixes, so it cannot be loaded at the main
  menu; `MSFont::Set_Color` recolors the font from its own palette instead.
- The map preview loader uses `AlternateSurface` as scratch space. Anything
  that caches a backdrop there gets corrupted when the map changes.
- The game select page's backdrop is a movie. Anything painted over it must
  be repainted after every movie frame and must not compound.
- The discs on the game select page have no unlit image; their unlit look is
  part of the backdrop. Only the lit image is separate.
- The shell menus are 640x400 layouts. They fill the screen only when the
  render resolution is 640x400, which on a 16:10 panel is an exact fit.
- The engine's view clamp turns a map refit near the map's right edge into a
  shift, so the sidebar slides over the map rather than refitting it.

## Next

1. Untested on the sidebar: a grid longer than the panel scrolling. Untested
   in play: a mission-end movie presented without the sidebar, and a
   multi-page briefing turned back a page.
2. A Steam Input layout for the game, shipped in the repo, so the pad is on
   the Gamepad template without the player building one. Steam only defaults
   a layout the app owner publishes, so this is a file to import once, and
   Steam keeps the choice per player after that. Deferred to the release
   checklist: until there are players other than the author it saves one
   menu visit and would be tested by nobody.
3. LAN lobby extras: kick as a row on the host lobby, and the generated map
   once its setup has a console screen. The lobbies and chat are still
   untested between two machines.
4. Deleting a save from the pad. The console save box always writes a new
   slot and the console load screen has no delete, so saves only accumulate;
   the mouse load dialog's delete is the only way to remove one.
5. Later: the display work in the direction notes.

## Parked

- Zoom's smooth glide. Each step is a mode change and a full redraw, and on
  the Deck the mode change measured 3 to 9 ms from 480 to 1080 high, under a
  frame, so the stepped zoom did not need one. If it ever does, the
  presenter can animate a crop of the current frame between rungs and commit
  the mode change at each rung. The presenter draws the map and the
  sidebar as separate quads for the fixed-size sidebar, so the glide would
  only animate the map's quad.
- Drop pods have been verified in a Firestorm game only, which is the only
  game whose rules define them.
- Arrow keys beep on the Deck under Proton on every press. Present on the
  upstream build too, not caused by this fork.
- A bigger pointer on the Deck. Doubling the automatic `CursorScale` under
  the controller scheme showed on a PC, where it was too big, but the Deck's
  compositor draws the cursor at a size of its own whatever image it is
  given: a 6x test image came out the same size there. The doubling was
  dropped. A bigger Deck pointer means drawing it inside the picture, at the
  frame rate, so it waits.
- The pad pointer is hidden at a scenario's start and fades a few seconds
  after the pad stops moving it. The Deck's compositor shows its cursor on
  mouse motion alone, and the upstream build does the same, so it is not
  the fork's doing. A one-pixel nudge held for a poll while idle kept it
  but looked twitchy. Moving the pad brings it back.
- The screen flashes white at launch, before the Westwood logo. Stock
  Tiberian Sun does the same under Proton, so it sits below the game. A
  black window brush and a black frame presented right after renderer init
  both failed to remove it. Worth a targeted look at the end of the project.
- Held-key map scrolling uses the edge-scroll speed for the `ScrollRate`
  option and may want a rate of its own.
