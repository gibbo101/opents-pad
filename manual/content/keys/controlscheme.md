---
key: ControlScheme
summary: Whether the game presents its keyboard and mouse dialogs or the console-style screens meant for a controller.
when_omitted:
  kind: value
  value: Auto
---

The setting names one of three schemes, in any capitalization:

| Value | Result |
| --- | --- |
| `Auto` | `Controller` when an XInput controller is connected at launch, otherwise `KeyboardMouse` |
| `KeyboardMouse` | The original dialogs, driven by the mouse |
| `Controller` | Console-style screens: a list of rows, one in focus, driven by the arrow keys, Enter, and Escape, by the first connected XInput controller, or by the mouse |

The scheme decides which screen appears, not how a screen behaves. Under `Controller` the skirmish setup and options screens are console-style lists that edit the same settings as the dialogs, with the map list numbered and a shoulder button or Shift stepping five at a time; the campaign screen is a side select with the two emblems and the difficulty; the mission briefing takes the controller's accept and back in place of its buttons; every other screen is still the original dialog until it gains a console-style version. The controller's menu button presses Escape wherever the game reads keys, so it skips a movie and opens the in-game menu. Under `Controller` the shell menus also render at 640x400, the size their artwork was drawn for, and the configured resolution applies once a game starts. Under `KeyboardMouse` nothing differs from a build without this setting.

A name the game does not recognize leaves the setting at `KeyboardMouse`. The setting changes from either scheme: the options page has a Switch To Controller button, enabled while a controller is connected, and the console-style options screen has a Control Scheme row offering all three values. A choice made on either screen is saved as that scheme, so `Auto` only returns by choosing it on the console screen or writing it in the file. Choosing `KeyboardMouse` from the console screen asks for a real key press or mouse click within ten seconds and reverts without one. Holding the controller's menu and back buttons together for a second switches to `Controller` from any screen. The change takes effect at the next screen.
