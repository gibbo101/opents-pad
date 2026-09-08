---
title: Take the controller's accept button on the score screens
category: feature
release: 0.2.0
targets:
- type: key
  id: ControlScheme
  effect: changed
- type: command
  id: fixed:dismiss-multiplayer-score
  effect: changed
credit: [gibbo101]
---

Under the `Controller` control scheme the skirmish and LAN score screen and the campaign score screen render at the shell's 640x400 size so they fill the display, their prompt reads Continue behind the accept button's glyph, and the accept button dismisses them alongside a click, Escape, or space. A place in the campaign hall of fame takes its name on the on-screen keyboard instead of typed keys. The keyboard scheme is unchanged.
