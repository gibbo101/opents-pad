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
| `Auto` | `Controller` while a controller is connected, otherwise `KeyboardMouse` until a controller button is pressed on a menu screen |
| `KeyboardMouse` | The original dialogs, driven by the mouse |
| `Controller` | The console-style screens, driven by the arrow keys, Enter, and Escape, by the first connected XInput controller, or by the mouse |

A settings file without the key reads as `Auto`, so an install that showed the dialogs shows the console-style screens once a controller is connected; write `ControlScheme=KeyboardMouse` to keep the dialogs. A name the game does not recognize leaves the setting at `KeyboardMouse`. [Controller scheme](/systems/controller-scheme/) owns how `Auto` finds a controller, which screens each scheme presents, and the controls in play.

The setting changes from either scheme: the options page has a Switch To Controller button, enabled while a controller is connected, and the console-style options screen has a Control Scheme row offering all three values. A choice made on either screen is saved as that scheme, so `Auto` only returns by choosing it on the console screen or writing it in the file. Choosing `KeyboardMouse` from the console screen asks for a real key press or mouse click within ten seconds and reverts without one. Holding the controller's menu and back buttons together for a second switches to `Controller` from any screen. The change takes effect at the next screen.
