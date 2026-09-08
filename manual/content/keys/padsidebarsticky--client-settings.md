---
key: PadSidebarSticky
scope: client-settings
label: Controller sidebar sticky
see_also: [ControlScheme, PadZoomHeight]
when_omitted:
  kind: value
  value: "no"
---

Whether the sidebar stays over the map under the `Controller` control scheme instead of sliding out when the controller leaves it. The game writes this key itself each time the sticky state changes, so a sticky sidebar is back on the first tick of the next scenario, the controller staying on the map. [Controller scheme](/systems/controller-scheme/#sidebar) has how the sidebar slides and how the sticky state is set and released.

At `no` the sidebar slides out whenever the controller leaves it, and nothing but the controller brings it in. The keyboard scheme never reads this key.
