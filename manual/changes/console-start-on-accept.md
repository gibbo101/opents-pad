---
title: Start a game on accept and open the keyboard on the menu button
category: feature
release: 0.2.0
targets:
- type: key
  id: ControlScheme
  effect: changed
- type: key
  id: PadPointerSpeed
  effect: changed
credit: [gibbo101]
---

Under the `Controller` control scheme the skirmish and LAN host screens start the game on the accept button from any row, and the controller's menu button opens the on-screen keyboard from any row: the player name on the skirmish and LAN games screens, the chat in a lobby. The name row on the skirmish screen no longer opens the keyboard on accept. The built-in pointer pace at `PadPointerSpeed` `5` is doubled, so a saved `10` now moves twice as fast as before and `5` gives the old `10`.
