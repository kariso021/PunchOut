# PunchOut
Punch out of fucking Company

## GAS foundation

`PunchOutCore` is the shared runtime plugin. `APunchOutPlayerState` owns the replicated Ability System Component, and the existing Third Person Blueprint now derives from the Core `APunchOutCharacter` to bind its GAS avatar on possession. Its movement and animation defaults are preserved.

Verified: Editor build, Blueprint compilation/default preservation, and `PunchOut.Core.AvatarOwnership` automation test. Combat features, Experience loading, ability grants/input, and network PIE validation remain pending. The architecture and progress are recorded in `artifacts/punchout-lyra-gas-design.md`.

## Office prototype

Open `/Game/Levels/L_OfficePrototype` and press Play. This is also the default startup map.

- WASD: move; mouse: rotate the third-person camera; Space: jump.
- Click the play viewport to capture input. Shift+F1 releases the mouse; Esc stops play.
- The office uses basic shapes, 5 m walls, desks, partitions, a meeting area, storage, and an open exit.
- The character and movement animations use Unreal Engine 5.8's Third Person template assets.
- For a local network test, set Number of Players to 2 and Net Mode to Play As Listen Server in Play settings.

Verified: character/controller/game mode Blueprint compilation, two-player PIE connection, server-to-client character position replication, and office collision traces. Matchmaking and escape rules are not implemented yet.
