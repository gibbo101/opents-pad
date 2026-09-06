---
key: ControlScheme
summary: Whether the game presents its keyboard and mouse dialogs or the console-style screens meant for a controller.
when_omitted:
  kind: value
  value: KeyboardMouse
---

The setting names one of two schemes, in any capitalization:

| Value | Result |
| --- | --- |
| `KeyboardMouse` | The original dialogs, driven by the mouse |
| `Controller` | Console-style screens: a list of rows, one in focus, driven by the arrow keys, Enter, and Escape, or by the first connected XInput controller |

The scheme decides which screen appears, not how a screen behaves. Under `Controller` the skirmish setup screen is a console-style list that edits the same settings as the dialog and starts the same game, with the map list numbered and a shoulder button or Shift stepping five at a time; every other screen is still the original dialog until it gains a console-style version. Under `KeyboardMouse` nothing differs from a build without this setting.

A name the game does not recognize leaves the setting at the value it already had, which is `KeyboardMouse` unless a previous save wrote something else. No in-game control changes it yet; edit the file.
