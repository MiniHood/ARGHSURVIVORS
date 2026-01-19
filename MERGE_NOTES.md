# Merge Notes (2026-01-19)

## Summary
- Add a refresh helper for ambient vehicle spawn points to re-run selection after the loot system is ready.
- Add one game project dependency.
- Normalize line endings in README and currency component (no logic changes).

## Changes
- Added `ARGH_RefreshNow()` to `SCR_AmbientVehicleSpawnPointComponent` to rerun `ForceUpdate` with the saved faction.
- Updated `addon.gproj` dependencies to include `6660335795193F58`.
- Line-ending-only edits in `README.md` and `Scripts/Game/Currency/ADM_CurrencyComponent.c`.

## Testing
- Not run (no automated tests invoked).
