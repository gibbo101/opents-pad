# Controller support

This fork adds a controller control scheme to OpenTS. The guiding reference is
Command & Conquer: Red Alert Retaliation on the PlayStation: list-style menus,
one focused row, left and right to change a value, one button to accept and
one to go back. The Steam Deck is the primary test device, but nothing is
Deck-specific; every screen size from a 7-inch panel to a 32:9 ultrawide is a
target. [CONTROLS.md](CONTROLS.md) is the player's reference for every
button.

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
| Console menu framework | `code/consolemenu.cpp` | Rows of label, value, step, activate, optional icon and swatch strip; a row may sit on a fixed line; long lists scroll. Big menu font, teal focus row, dark panel over the backdrop that a screen can size or drop, held-key repeat, Shift or a shoulder button steps by five, the shell's click as the focus moves. A screen can draw its own content through a canvas, register mouse targets, and run an idle hook each pass; pointing at a row focuses it, a click accepts, and the two prompts are clickable. While a row with an action of its own has focus the accept prompt names that action, such as Edit on a name row or Play on the track row. A screen can give the pad's menu button a job done from any row, shown mid-line with its glyph: the name keyboard on the skirmish and LAN games screens, the chat on the lobbies. A accepts from any row without an action of its own, so it starts the skirmish and host screens everywhere but on a Chat row. |
| Native pad reader | `code/gamepad.cpp` | XInput, loaded at first use. D-pad or left stick, A accept, B back, LB/RB fast step. Under Steam the pad must be on the Gamepad template, not Keyboard & Mouse. |
| Skirmish setup | `code/consoleskirmish.cpp` | Console screen: name (read only), side with faction icon, colour swatch strip, numbered map list with preview, then the numeric rows and toggles. Game Speed starts from the saved option rather than the session's fastest, and the LAN host screen seeds the session the same way. Writes the same session fields as the dialog. |
| Game select page | `code/grphmenu.cpp`, `code/grphmimg.cpp` | Original artwork kept. Spatial d-pad navigation over the buttons, Tiberian Sun selected on entry, unselected discs darkened 70 percent, B or Escape takes the page's Back button. A button still held from the previous screen is not a press. |
| Menu pages | `code/newmenu.cpp` | Under the controller scheme the Tiberian Sun and Firestorm pages are console screens on their own backdrops: New Campaign, Load Mission, LAN, Skirmish, Options and Game Select as rows in the menu font, no box, the page theme playing. Intro and Exit keep their artwork at the corners and light when reached: Up or Right for Intro, Down or Left for Exit. Internet and the tour are left out. Keyboard & Mouse keeps the artwork pages. |
| LAN | `code/consolelan.cpp` | Console screens running the dialogs' discovery and join protocol unchanged. Game list: your name, Host New Game, one row per game found, the lobby count, a joining line while a request is out, and the reason when a join fails. Host lobby: the skirmish rows live, the joined players under the map preview in their colours with faction icon and host or ready mark, Start checks for a second player, everyone ready, and room on the map. Guest lobby: the host's rows read only, own side and colour requested from the host, Ready as the accept, Leave signs off. Each lobby screen has a Chat row showing the latest message that opens a chat screen: the lobby's messages and notices down the page, A to type one on the on-screen keyboard, sent to the game's players or, before joining, to everyone in the lobby. No kick, and the generated map is skipped in the map list. The Keyboard & Mouse dialogs are untouched; the packet handler only skips the guest dialog and message boxes while the console screens are up. |
| Button prompts | `code/padglyph.cpp`, `assets/input-prompts/` | The accept and back prompts on every console screen and the briefing carry the button's glyph from Kenney's CC0 Input Prompts pack, baked into `code/padglyphdata.h` by `tools/padglyphs.py` and sampled down to the prompt's size with alpha. `PromptStyle` in `SUN.INI` picks the set or plain text; Auto is Deck when Steam's `SteamDeck` variable says so, else PlayStation when the system lists a Sony game controller, else Xbox while a pad is connected. Under Proton the host kernel's device list is read through drive Z, which still names the real pad when Steam Input has replaced it with its virtual Xbox one; on Windows the game's own device list is used, where Steam Input's pad reads as Xbox and PlayStation is chosen on the options screen. |
| Options | `code/consoleoptions.cpp` | Console screen: control scheme, button prompts, a Zoom row stepping the pad's render height in place of a resolution, scale mode, stretch movies, game speed, the pad's pointer, fast pointer and stick scroll speeds in place of the mouse's scroll rate and coasting, the unit snap radius, detail, campaign difficulty, cameo text, action lines, tool tips, and Audio and Controls rows. Accept saves. |
| Controls | `code/consoleoptions.cpp` | Console screen from Options and the pause menu listing what the pad's buttons do, each face button with its glyph, and a note that the bindings are not final until the in-game scheme exists. |
| Audio | `code/consoleoptions.cpp` | Console screen from Options and the pause menu: music, sound and voice volume stepped live, and in play shuffle, repeat, a Now Playing row, a Track row that steps through the songs by name with A on the row playing it, and Stop Music. Accept keeps the volumes and saves; back restores them. |
| Select Campaign | `code/init.cpp` | Side select in the manner of Remastered: the backdrop's own GDI and Nod discs enlarged at left and right, both dim until a side is picked, then the picked one lit and grown with its campaign named under the title; names beneath in faction colours, difficulty along the bottom. Left and right pick the side, or the act when a side has two, with the shell's hover click; A or a click on the lit emblem starts. Writes the difficulty option like the dialog. |
| Load Mission | `code/loaddlg.cpp` | Console list of the save games, newest first, date at the left and description at the right, a star for multiplayer saves; long lists scroll. |
| On-screen keyboard | `code/consolekeyboard.cpp` | A console screen with a text field and a row of digits, three rows of letters and marks, then Space, Delete, Caps, and Done. D-pad or stick moves, A types, X deletes, Y adds a space, Start or A on Done finishes, B backs out with the text untouched; letters are capitals at the start of each word and lower case after, with Caps locking them on; a real keyboard types straight in, Enter finishes, Escape backs out; the mouse picks keys. Opened by the menu button on the skirmish and LAN games screens, by A on the LAN games list's Name row, and by A on the Save As row of the pause menu. |
| Play, zoom | `code/mainopt.cpp`, `code/gamepad.cpp` | Under the controller scheme play renders at a zoom, not the keyboard scheme's resolution: a render height from a ladder of 480 to 1440, never above the panel, with the width on the panel's shape so no screen shows bars. `Play_Display_Mode` settles it from `PadZoomWidth` and `PadZoomHeight` in `SUN.INI`, falling back to a baseline of 768 high, 600 on a Deck, whenever the saved pair is empty or off the panel's shape. R1 with the right stick up or down steps it in play, a step per push and every quarter second held; the pad records the step and the main loop applies it beside the in-game menu, since the pump can run mid-draw. Each step keeps the view's centre, saves, and logs how long the mode change took; nothing is announced on screen. The tracking window never follows the frame under the controller scheme; the drawable area is the panel. |
| Play, sidebar size | `code/video.cpp`, `code/bgfxbackend.cpp`, `code/vidscale.cpp`, `code/mainopt.cpp` | Under the controller scheme the sidebar is presented apart from the frame, so its share of the screen stops changing with the zoom. The sidebar surface is allocated at the height of the pad's own panel, the radar and mode buttons over five rows of two cells and the art's foot plate, read from the sidebar art's top, middle, bottom and add-on pieces, 512 in the game's art, whatever height the map renders at, with `Max_Visible` held to five rows under the scheme so the foot plate never becomes a sixth, and the presenter draws it as a second quad scaled to fill the panel's height beside the map's quad, so the five rows always fill the screen; the frame's own sidebar columns are never shown, and its map columns take the shape of the panel left beside the sidebar, so `Pad_Zoom_Width` adds the sidebar's 168 columns to a map width on that shape. The point conversions in `vidscale.cpp` map each quad back to its own columns, and a position in the sidebar's columns is held to the sidebar's height rather than the frame's, since the two differ away from the baseline. The sidebar's own blit into the frame is skipped and only marks the frame due. Keyboard & Mouse keeps the sidebar in the frame. |
| Play, top bar | `code/tab.cpp`, `code/video.cpp`, `code/vidscale.cpp`, `code/mainopt.cpp` | Under the controller scheme the top bar is kept, in the manner of C&C1, but presented apart from the frame like the sidebar: `TabSurface` is drawn at the sidebar's scale across the map's part of the screen, so the bar never changes size with the zoom, and the frame's own top rows are never shown. The bar carries the Options tab at its left, with the pad's menu button glyph before the label and the pair centred on the tab, the beam, and the mission timer's tab at its right; the credits stay on the sidebar. Its own width follows from the screen and the sidebar's scale, `BarWidth` in the scale info, so the tab bar code draws into it at that width. Mouse positions on the bar are brought onto the frame's top rows with their x scaled to the map's columns, and the tab's hit width is scaled the same way. The map's part of the frame is rendered for the screen left beside the sidebar and under the bar, so `Pad_Zoom_Width` fits the map's rows to that. The keyboard scheme keeps the bar in the frame as it was. |
| Play, pointer | `code/gamepad.cpp` | Under the controller scheme in play the left stick and the d-pad move the pointer, and when they come to rest with a unit, infantry, aircraft or building within `PadSnap` eighths of a cell of the pointer (0 off, 4 the default), found by resolving the pointer's cell as a click does and scanning the cells around it (the engine's `Close_Object` only reaches a quarter cell), the pointer is warped onto its centre, cloaked enemies excepted; the stick at a steady rate for its deflection and the d-pad from half pace to full within 150 ms of a press, so a tap stays within a cell and a hold is at pace at once, R1 speeding either, the paces in map pixels against a 600-high yardstick so a push covers the same ground at any zoom or panel; at the screen's edge the distance the pointer cannot travel scrolls the map, so the edge moves at the pointer's own pace. The pad is polled from the frame loop, so the game thread only sets a velocity and a box each poll and a 60 Hz multimedia timer moves the OS pointer, which keeps the steps small however slowly frames come; the timer thread attaches its input to the game thread's so the cursor shape stays the game's, and banks the travel the box refused for the game thread to scroll by. The three paces come from `PadPointerSpeed`, `PadFastSpeed` and `PadScrollSpeed` in `SUN.INI`, 1 to 10, and the mouse scroll settings leave the pad alone. Cross is the left mouse button and circle a right-button tap at the pointer, so selecting, ordering, deploying, placing, band boxing and cancelling all follow the mouse paths, while a held circle never drag-scrolls. The message loop does not count these clicks as a real mouse. |
| Play, commands | `code/gamepad.cpp` | In PlayStation terms: square cycles the sidebar modes repair, sell, power, waypoint, off, passing over a mode the engine refuses; L3 deploys; R3 centres on the base; R2 scatters and R1 with R2 guards, and in waypoint mode R2 takes back the last waypoint of the path while it has one; L2 with square, triangle, circle or cross makes team 1, 2, 3 or 4, and L1 with the same selects it, or centres the view on it when tapped twice within 400 milliseconds; R1 with L1 orders a force fire at the pointer and R1 with L2 a force move, the moment both are held, with no cross; Select allies with the owner of the selected unit, which the engine only allows against a human player in a game with allies on. The right stick scrolls the map as a right-button drag does. Cross held still for half a second on one of the player's units selects every unit of its type on screen, and held on a little longer widens that to the whole map; held still on the ground it selects every combat unit on screen, and held on longer every combat unit on the map, counting a unit that carries a weapon or gains one by deploying, so harvesters, engineers, sensor arrays and the construction vehicle stay out. Each of the four says what it selected in the message list at the top left. Each button runs the engine's own named command, so the keyboard bindings stay the source of the behaviour. |
| Play, sidebar | `code/sidebar.cpp`, `code/gamepad.cpp` | Under the controller scheme the sidebar strips are replaced by a fixed grid in the manner of Retaliation: two columns, the player's side on the left and the other side on the right, with rows for structures, infantry, vehicles and aircraft, and a bottom row holding the current superweapon and a cell that cycles to the next. Each structures cell carries its side's emblem in the top right. A side's construction yard shows all four of its sections, dulled until the factory exists; without one only the sections whose factory the player holds appear. A section cell shows what it is building, or its last build once idle, or the factory; leaving a grid with nothing building puts the factory back. Triangle takes the pad in and out; the outline is bright inside and dim outside, and it parks where it was left. Inside, the d-pad or stick moves between the cells, the four mode buttons above them and the radar. Cross on a section builds or places the current or last item, or opens the section's grid when it has none; in a grid cross builds the marked item, and the structures grid closes on a build since a yard builds one thing at a time. Circle on a section that is building holds, then cancels; on an idle section it opens the grid; in a grid it cancels the marked build or steps back to the sections. Square opens or closes the grid without cancelling anything. Placing a building, aiming a superweapon or picking a mode button hands focus back to the map with the ghost, target or mode cursor on the pointer, and triangle out of a grid parks the sidebar on that section. From the map, R1 with circle works the parked cell, as Retaliation rebuilds the last object: place, build the last item again, add one more, or queue the marked grid item. On the radar, cross held moves a marker over the map and letting go jumps the view there. The sidebar in Tiberian Sun is always on screen, so Retaliation's pin has no counterpart. |
| Play, cheats | `code/gamepad.cpp` | Retaliation's cheat codes, keyed in on the sidebar's four mode buttons: inside the sidebar, circle on Repair, Sell, Power or Waypoint enters that button as a symbol with a click, and the last six symbols are matched against the codes; a match plays the options sound and says what it did in the message list. Solo games only. The mode buttons stand in for Retaliation's glyph row in its order, Repair for cross, Sell for circle, Power for triangle and Waypoint for square, so each code is walked as on the PlayStation. Credits: Repair, Repair, Waypoint, Sell, Sell, Sell. Reveal the map with the radar on, or put the shroud back as it stood before the reveal, keeping what units see now: Power, Power, Repair, Sell, Power, Waypoint. Win the mission: Sell, Sell, Power, Repair, Repair, Waypoint. Ion cannon: Sell, Repair, Sell, Sell, Repair, Waypoint. Multi missile: Waypoint, Sell, Power, Repair, Sell, Sell. Chemical missile: Waypoint, Repair, Sell, Repair, Power, Power. Hunter seeker: Repair, Repair, Repair, Sell, Power, Waypoint. Drop pods: Waypoint, Waypoint, Sell, Sell, Power, Power. A superweapon the player holds is charged; one they lack is granted for a single shot. |
| Dialogs | `code/gamepad.cpp`, `code/msgloop.cpp` | While any dialog is open the pad drives it through key messages: d-pad as arrows with repeat, A as Enter, B as Escape. Covers the message boxes, Version, and the multiplayer lobby for now. |
| In-game menu | `code/consoleingame.cpp` | Under the controller scheme the Escape menu in play is a console screen of its own, like the main menus: the render size drops to 640x400 on entry and returns to the play size on exit, so the text and artwork match the shell on every panel. "Game Paused" above a box sized to its rows, in the menu pages' colours, on the mission's score plate rather than the shell backdrop, the campaign's or the multiplayer one, focus starting on Return To Mission. Game Options opens the console options screen without its display rows or the campaign difficulty, which the original in-game dialog never applied either; Audio Options opens the audio screen; Controls lists the bindings; Mission Briefing restates; Save Game offers the save in a box under a suggested name, the mission's side and number from its map file ahead of its name for a campaign, and on Save writes a fresh slot, reports beneath the pause box, and posts the message the quick save posts for when play resumes; Load Game opens the console load list; Restart Mission and Abort Mission ask as one row whose value flips between No and Yes, No to begin with, as the C&C pause menu does. In a LAN game Save and Load go through the multiplayer paths and Restart becomes Surrender. The menu loop's pause, resume, restart and exit handling is shared with the dialog. |
| Mission briefing | `code/restate.cpp` | Under the controller scheme the page renders at the shell's 640x400 size so it fills the panel like the console screens, then the size it was opened at returns. The buttons give way to console prompts: A continues or turns the page, B plays the mission video when there is one. Before the mission starts the last page closes with OK; restated from the pause menu it reads Resume Mission. |
| Score screens | `code/mpscore.cpp`, `code/score.cpp`, `code/scenario.cpp` | Under the controller scheme the skirmish and LAN score screen and the campaign score screen render at the shell's size so they fill the display, the prompt reads Continue behind the accept glyph, and the accept button dismisses them. A hall of fame place takes the name on the on-screen keyboard. |
| Shell render size | `code/mainopt.cpp`, `code/init.cpp`, `code/scenario.cpp` | Under the controller scheme the shell renders at 640x400, the size its artwork was drawn for, so the presenter fits it to the panel height with pillars on wide screens. The loading screen is shell artwork too, so a scenario start switches to 640x400 for it and the play size returns once the scenario is read. The keyboard scheme keeps the shell at the play resolution. |

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

The sidebar block comes first, in this order, because the later items
depend on the sidebar's final shape. Its first item, the sidebar at a fixed
on-screen size, is in: the presenter draws the sidebar as its own quad.

1. The top bar is done, C&C1 style: the bar stays, at a constant size like
   the sidebar, with Options at its left and the timer at its right, and
   the credits on the sidebar. Tried and dropped on the way: an Options
   cell across the sidebar's top, and a tab with the menu glyph over the
   map's corner; both read wrong on the Deck. The bar model is the one
   that leaves room to hide the sidebar later, with the credits moving onto
   the bar as Retaliation puts the money over the map.
2. Untested on the sidebar: the Nod side as the captured column; a grid
   longer than the panel scrolling; the hall of fame name on the on-screen
   keyboard and the score screen's restore after it.
3. The rest of the play scheme: the snap onto units is in, a `PadSnap` option
   defaulting to half a cell; the Controls screen is now the drawn
   pad with paged callouts.
4. A Steam Input layout for the game, shipped in the repo, so the pad is on
   the Gamepad template without the player building one. Steam only defaults
   a layout the app owner publishes, so this is a file to import once, and
   Steam keeps the choice per player after that.
5. LAN lobby extras: kick as a row on the host lobby, and the generated map
   once its setup has a console screen. The lobbies and chat are still
   untested between two machines.
6. Sidebar shadows: `assets/sidebar-shadows/` and `tools/sidebarshadows.py`
   bake a black silhouette of what a section builds over its cell, and every
   slot is empty until original art exists. The game's own sprites are not to
   be traced for it.
7. Later: the display work in the direction notes.

## Parked

- Zoom's smooth glide. Each step is a mode change and a full redraw, and on
  the Deck the mode change measured 3 to 9 ms from 480 to 1080 high, under a
  frame, so the stepped zoom did not need one. If it ever does, the
  presenter can animate a crop of the current frame between rungs and commit
  the mode change at each rung. The presenter now draws the map and the
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
  mouse motion alone, and last night's build did the same, so it is not the
  fork's doing. Tried and dropped: a one-pixel nudge held for a poll while
  idle, which kept it but looked twitchy. Moving the pad brings it back.
- The screen flashes white at launch, before the Westwood logo. Stock
  Tiberian Sun does the same under Proton, so it sits below the game. A
  black window brush and a black frame presented right after renderer init
  both failed to remove it. Worth a targeted look at the end of the project.
- Held-key map scrolling uses the edge-scroll speed for the `ScrollRate`
  option and may want a rate of its own.
- R1 with Triangle, Retaliation's sidebar pin. The sidebar in Tiberian Sun
  is always on screen, so the chord has no job yet.
