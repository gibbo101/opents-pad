---
key: PromptStyle
summary: Which controller's button glyphs the console-style screens draw beside their accept and back prompts.
when_omitted:
  kind: value
  value: Auto
---

The setting names one of five styles, in any capitalization:

| Value | Result |
| --- | --- |
| `Auto` | `Deck` when Steam reports the game is running on a Steam Deck, else `PlayStation` for a PlayStation controller, else `Xbox` for any other connected controller, else `Text` |
| `Text` | The prompt words alone |
| `Xbox` | The colored A, B, X, Y buttons |
| `PlayStation` | The colored cross, circle, square, triangle buttons |
| `Deck` | The Steam Deck's white A, B, X, Y buttons |

The glyph is drawn before the prompt's text on every console-style screen under the `Controller` control scheme, and on the mission briefing's prompts there. It only names the button; the accept and back buttons themselves do not move between styles. A name the game does not recognize leaves the setting at `Auto`.

A controller's maker is read from the system's device list. Under Proton that is the host's own list, which still names the real controller when Steam Input has replaced it with its virtual Xbox one; on Windows a controller Steam Input rewrites reads as Xbox whatever it is, so there the style is chosen rather than detected. The Steam Deck itself is known from the `SteamDeck` environment variable. The console-style options screen has a Button Prompts row that steps through the five values and shows the result on its own prompts as it does; accepting the screen saves the choice. Under the `KeyboardMouse` control scheme the setting has no effect.
