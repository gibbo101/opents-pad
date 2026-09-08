---
key: PadSidebarSticky
scope: client-settings
label: Controller sidebar sticky
see_also: [ControlScheme, PadZoomHeight]
when_omitted:
  kind: value
  value: "no"
---

Whether the sidebar stays on screen under the `Controller` control scheme. The sidebar starts away from the map, which then takes the whole width under the bar, and the fourth face button slides it in with the pad on it and out again; the right shoulder with the fourth face button slides it in and keeps it there, and the same again lets it go. The game writes this key itself each time that sticky state changes, so a sticky sidebar is back on the first tick of the next scenario, the pad staying on the map.

`no`, the default, is how Command & Conquer plays on the PlayStation: deploying a construction vehicle does not bring the sidebar up, and the player calls it when it is wanted. The keyboard scheme never reads this key.
