# PunchOut
Punch Out of Who You Were.

## Play

Open [PunchOut.uproject](PunchOut.uproject) in Unreal Engine 5.8, press Play, then choose **Host game → Start game**.

- WASD to move, mouse to look, Space to jump, left-click to punch.
- In the host viewport, a green outline marks a missed punch, a red outline marks a collision, and a yellow point shows contact. Punch animation is pending.

## Runtime Debug Toolkit

Press **Play** in Unreal Editor, then open [the local control panel](http://127.0.0.1:8088/punchout-ui).

- **UMG Hierarchy** lists the active game widgets and supports name/class search.
- **Widget Inspector** shows geometry, transforms, and reflected properties. Select **Allow display property edits** to change visibility, enabled state, and opacity. **Highlight in game** draws the selected widget's cached bounds.
- **UI State** applies sample ViewModel values to the running game's UI. **Reset to game state** restores the game-driven ViewModel. JSON import previews the change and asks for confirmation.
- **Performance** samples application frame time and process memory and exports CSV.
- **Input & Events** records game viewport Slate input handlers/results and web UI changes in a 256-entry buffer.

Use the category menus to reopen closed windows. Drag window titles to reorder and resize each window from its corner. The browser remembers window order, size, and visibility. To work without starting a game, open **PunchOut UI Preview** from the Window menu or enter `PunchOut.UIDebug` in the Output Log command field. Widget inspection targets PIE.

## Source and tests

- [Game flow](Source/PunchOut/UI/PunchOutUIFlow.cpp)
- [Player and pawn setup](Plugins/PunchOutCore/Source/PunchOutCore)
- [Punch ability and settings](Plugins/PunchOutCombat/Source/PunchOutCombat)
- [Frontend view, view model, and editor preview](Plugins/PunchOutFrontend)

Run the `PunchOut.` tests in Unreal's Automation panel.

[Architecture notes](artifacts/punchout-lyra-gas-design.md) · [Learning checklist](artifacts/punchout-learning-checklist.md)
