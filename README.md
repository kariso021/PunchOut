# PunchOut
Punch Out of Who You Were.

## Learning alongside development

Story milestones, ownership decisions, CS/graphics experiments, and verification evidence are tracked in [the learning checklist](artifacts/punchout-learning-checklist.md). Implementation completion and learning completion are recorded separately.

## GAS foundation

`PunchOutCore` is the shared runtime plugin. `APunchOutPlayerState` owns the replicated Ability System Component, and the existing Third Person Blueprint now derives from the Core `APunchOutCharacter` to bind its GAS avatar on possession. Its movement and animation defaults are preserved.

Verified: Editor build, Blueprint compilation/default preservation, and `PunchOut.Core.AvatarOwnership` automation test. Combat features, Experience loading, ability grants/input, and network PIE validation remain pending. The architecture and progress are recorded in `artifacts/punchout-lyra-gas-design.md`.

## Mountain gate prototype

### Punch prototype

During Playing (after starting the match), left-click requests a server-only GAS punch. The sphere sweep follows horizontal controller aim from the Pawn center + 20 cm vertically: 140 cm travel, 20 cm radius (160 cm maximum tip reach), first Visibility blocker only. Green/red capsules show miss/hit and a yellow point marks contact on the server/standalone viewport. This is an initial tuning choice, not PUBG's actual implementation or values.

`Plugins/PunchOutCombat/Source/PunchOutCombat/Private/Abilities/PunchOutPunchAbility.cpp` contains the query. Defaults are in its public header. Recovery is 0.45 s; repeated activation is blocked while active and unpossession cancels recovery. Damage uses the prototype `ApplyPointDamage` bridge (10 damage); there is no HealthSet, damage GameplayEffect, punch montage, hit-timing notify, prediction, or lag compensation yet. Ordinary scenery will block a punch but will not lose health unless it implements damage handling. This Combat plugin is currently always loaded; Experience/Game Feature activation remains pending.

Validation on 2026-11-04: Editor build succeeded after closing Unreal Editor. `PunchOut.Combat.PunchRange` passed (front hit, side/behind/out-of-range misses, wall blocking), as did `PunchOut.Core.AvatarOwnership`. Interactive input/animation feel and network validation remain pending. The pre-existing GameFeatureData Asset Manager scan error remains unrelated to this always-loaded prototype module.

Open `/Game/Levels/L_MountainGate` and press Play. This is the default startup and game map.

- WASD: move; mouse: rotate the third-person camera; Space: jump.
- The cultivation-world blockout contains a sealed sect gate, a training court, a descending stone path, pines, and distant mountain peaks.
- Geometry and materials are placeholders; the character uses the existing Third Person template.
- The office map has been removed. Office character/animation assets remain for compatibility with existing references.
- Map generation and validation scripts live in `Scripts/`. Run the build script only when intentionally rebuilding the blockout.

Verification: map saved by Unreal Editor commandlet. Interactive play and visual lighting review remain pending.
