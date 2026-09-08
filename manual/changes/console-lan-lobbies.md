---
title: Add console-style LAN lobbies under the controller scheme
category: feature
release: 0.2.0
targets:
- type: key
  id: ControlScheme
  effect: changed
credit: [gibbo101]
---

Under the `Controller` control scheme the LAN screens are console-style: a game list with the player's name, Host New Game, a row per game found, the lobby count, and the reason when a join fails; a host lobby with the skirmish rows, its game speed starting at the saved option, the joined players under the map preview in their colors with faction icon and host or ready mark, and Start checking for a second player, everyone ready, and room on the map; and a guest lobby with the host's rows read only, the player's own side and color requested from the host, Ready as the accept, and Leave to sign off. The lobbies run the dialogs' discovery and join protocol unchanged, offer no kick, and leave the generated map out of the map list. The `KeyboardMouse` dialogs are unchanged.
