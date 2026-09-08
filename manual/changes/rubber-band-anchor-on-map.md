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

A drag selection kept its first corner at a screen position, so scrolling the view during the drag, at the screen's edge or with the controller's stick, carried the whole box along instead of growing it. The corner now stays where it was on the map, and the box grows across the scroll; a box that reaches past the view selects the player's objects by their map positions, so what scrolled off screen is selected too.
