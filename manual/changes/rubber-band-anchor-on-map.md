---
title: Keep a drag selection's first corner on the map while the view scrolls
category: fix
release: 0.2.0
targets:
- type: system
  id: target-selection
  effect: changed
credit: [gibbo101]
---

Under the `Controller` control scheme a drag selection's first corner stays where it was on the map while the view scrolls, at the screen's edge or with the stick, so the box grows across the scroll instead of being carried along. A box that reaches past the view selects the player's objects by their map positions, so what scrolled off screen is selected too. The `KeyboardMouse` scheme keeps the corner at its screen position as before.
