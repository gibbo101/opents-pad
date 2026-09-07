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
| `Auto` | `Xbox` while an XInput controller is connected, otherwise `Text` |
| `Text` | The prompt words alone |
| `Xbox` | A ring and letter in the Xbox colours: green A, red B, blue X, yellow Y |
| `PlayStation` | The four shapes: cross, circle, square, triangle |
| `Deck` | A pale ring and white letter, the Steam Deck's own buttons |

The glyph is drawn before the prompt's text on every console-style screen under the `Controller` control scheme, and on the mission briefing's prompts there. It only names the button; the accept and back buttons themselves do not move between styles. A name the game does not recognize leaves the setting at `Auto`.

The game cannot tell one controller from another, since Steam presents every pad to it as an Xbox controller, so the style is chosen rather than detected. The console-style options screen has a Button Prompts row that steps through the five values and shows the result on its own prompts as it does; accepting the screen saves the choice. Under the `KeyboardMouse` control scheme the setting has no effect.
