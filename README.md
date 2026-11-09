# PunchOut
Punch Out of Who You Were.

## Play

Open [PunchOut.uproject](PunchOut.uproject) in Unreal Engine 5.8, press Play, then choose **Host game → Start game**.

- WASD to move, mouse to look, Space to jump, left-click to punch.
- In the host viewport, a green outline marks a missed punch, a red outline marks a collision, and a yellow point shows contact. Punch animation is pending.

## Frontend UI preview

Press **Play** in Unreal Editor, then open [the local control panel](http://127.0.0.1:8088/punchout-ui). **Apply** updates the running game's UI, and **Reset** restores the display driven by the game. To work without starting a game, open **PunchOut UI Preview** from the Window menu or enter `PunchOut.UIDebug` in the Output Log command field.

## Source and tests

- [Game flow](Source/PunchOut/UI/PunchOutUIFlow.cpp)
- [Player and pawn setup](Plugins/PunchOutCore/Source/PunchOutCore)
- [Punch ability and settings](Plugins/PunchOutCombat/Source/PunchOutCombat)
- [Frontend view, view model, and editor preview](Plugins/PunchOutFrontend)

Run the `PunchOut.` tests in Unreal's Automation panel.

[Architecture notes](artifacts/punchout-lyra-gas-design.md) · [Learning checklist](artifacts/punchout-learning-checklist.md)
