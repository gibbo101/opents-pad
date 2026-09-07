---
title: Replace the sidebar strips with a section grid under the controller scheme
category: feature
release: 0.2.0
targets:
- type: key
  id: ControlScheme
  effect: changed
credit: [gibbo101]
---

Under the `Controller` control scheme the sidebar shows a fixed grid instead of the two build strips: the player's side in the left column and the other side in the right, with rows for structures, infantry, vehicles, and aircraft and a bottom row for the current superweapon and a cell that cycles to the next. A side's construction yard shows all four of its sections, dulled until each factory exists; without one only the sections whose factory the player holds appear. A section cell shows what it is building, its last build once idle, or the factory. The fourth face button takes the controller in and out of the sidebar and the outline parks where it was left. Inside, accept on a section builds or places its current or last item or opens the section's grid, back on a building section holds then cancels and on an idle one opens the grid, and the third face button opens or closes the grid. Placing a building, aiming a superweapon, or picking a repair, sell, power, or waypoint mode returns focus to the map with the cursor set. From the map the right shoulder with accept works the parked cell. On the radar, accept held moves a marker and releasing it jumps the view there. The keyboard scheme is unchanged.
