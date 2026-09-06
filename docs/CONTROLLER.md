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
- Button prompts must show the connected pad's glyphs: Xbox, PlayStation, or
  Steam Deck styles, with plain text as the fallback.

## What exists

| Piece | Where | State |
| --- | --- | --- |
| `ControlScheme` option | `code/options.cpp`, manual key page | Done. Defaults to Auto: Controller when a pad is connected at launch. Switched from the console options screen or the options page's Switch To Controller button. |
| Console menu framework | `code/consolemenu.cpp` | Rows of label, value, step, activate, optional icon and swatch strip. Big menu font, teal focus row, dark panel over the backdrop, held-key repeat, Shift or a shoulder button steps by five. |
| Native pad reader | `code/gamepad.cpp` | XInput, loaded at first use. D-pad or left stick, A accept, B back, LB/RB fast step. Under Steam the pad must be on the Gamepad template, not Keyboard & Mouse. |
| Skirmish setup | `code/consoleskirmish.cpp` | Console screen: name (read only), side with faction icon, colour swatch strip, numbered map list with preview, then the numeric rows and toggles. Writes the same session fields as the dialog. |
| Shell menu pages | `code/grphmenu.cpp`, `code/grphmimg.cpp` | Original artwork kept. Spatial d-pad navigation over the buttons, Tiberian Sun selected on entry, unselected discs darkened 70 percent, B or Escape takes the page's Back button. A button still held from the previous screen is not a press. |
| Options | `code/consoleoptions.cpp` | Console screen: control scheme, resolution from the display's mode list, scale mode, integer scaling, stretch movies, game speed, scroll rate and coasting, detail, campaign difficulty, cameo text, action lines, tool tips, three volumes stepped live. Accept saves; back restores the volumes. |
| New Campaign | `code/init.cpp` | Console screen: the campaign stepped like a list, difficulty under it, Start to begin. Writes the difficulty option like the dialog. |
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

1. A zoom setting for play: the render frame takes the panel's aspect and a
   zoom value picks its height, so no screen shows bars and each device tunes
   its own sprite size. A 32:9 panel at 2x felt right in testing.
2. Glyph sets for prompts and a `PromptStyle` option (Auto, Xbox,
   PlayStation, Deck) with pad type detection.
3. An on-screen keyboard for the player name.
4. A Steam Input layout for the game, shipped in the repo, so the pad is on
   the Gamepad template without the player building one. Steam only defaults
   a layout the app owner publishes, so this is a file to import once, and
   Steam keeps the choice per player after that.
5. Later: the in-game control scheme, the sidebar, and the display work in
   the direction notes.

## Parked

- Arrow keys beep on the Deck under Proton on every press. Present on the
  upstream build too, not caused by this fork.
- The screen flashes white at launch, before the Westwood logo. Stock
  Tiberian Sun does the same under Proton, so it sits below the game. A
  black window brush and a black frame presented right after renderer init
  both failed to remove it. Worth a targeted look at the end of the project.
- Held-key map scrolling uses the edge-scroll speed for the `ScrollRate`
  option and may want a rate of its own.
