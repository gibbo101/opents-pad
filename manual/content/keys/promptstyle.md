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
| `Auto` | `Deck` when Steam reports the game is running on a Steam Deck, else `PlayStation` when the system's device list holds a Sony game controller, else `Xbox` while an XInput controller is connected, else `Text` |
| `Text` | The prompt words alone |
| `Xbox` | The coloured A, B, X, Y buttons |
| `PlayStation` | The coloured cross, circle, square, triangle buttons |
| `Deck` | The Steam Deck's white A, B, X, Y buttons |

The glyphs are Kenney's Input Prompts, drawn before the prompt's text on every console-style screen under the `Controller` control scheme, and on the mission briefing's prompts there. It only names the button; the accept and back buttons themselves do not move between styles. A name the game does not recognize leaves the setting at `Auto`.

A controller that reaches the game directly is recognised by its maker in the system's device list. One that Steam Input rewrites, as it does in Big Picture, is presented as an Xbox controller whatever it is, so there the style is chosen rather than detected; the Steam Deck itself is known from the `SteamDeck` environment variable. The console-style options screen has a Button Prompts row that steps through the five values and shows the result on its own prompts as it does; accepting the screen saves the choice. Under the `KeyboardMouse` control scheme the setting has no effect.
