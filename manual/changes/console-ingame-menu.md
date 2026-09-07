---
title: Add a console-style in-game menu under the controller scheme
category: feature
release: 0.2.0
targets:
- type: key
  id: ControlScheme
  effect: changed
credit: [gibbo101]
---

Under the `Controller` control scheme the in-game menu is a console-style screen of its own, rendered at the shell's 640x400 size like the main menus, on the mission's score plate, with Game Options, Audio Options, Controls, Mission Briefing, Save Game, Load Game, Restart Mission, Abort Mission, and Return To Mission as rows, with restart and abort asking for confirmation. Save writes a fresh slot under the mission's name and reports in the box, Load opens the console-style load list, and in a LAN game Save and Load take the multiplayer paths and Restart becomes Surrender. The console-style LAN screens gained a host lobby and a guest lobby, and the pre-mission briefing now closes with OK rather than Resume Mission. A Controls screen, from the options screen and the pause menu, lists what the controller's buttons do with their glyphs and notes that the bindings are not final. The console-style options screen's volumes moved to an Audio screen of their own, which in play also offers shuffle, repeat, the track to play, and a stop, as the sound controls dialog does. The `KeyboardMouse` dialogs are unchanged.
