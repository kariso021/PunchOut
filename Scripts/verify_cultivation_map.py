import unreal

levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert levels.load_level('/Game/Levels/L_MountainGate')
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
starts = [a for a in actors if isinstance(a, unreal.PlayerStart)]
assert len(starts) == 1
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
floors = [a for a in actors if a.get_actor_label() in ('Training court', 'Mountain shelf')]
assert len(floors) == 2
for floor in floors:
    component = floor.static_mesh_component
    assert component.get_collision_enabled() != unreal.CollisionEnabled.NO_COLLISION
    assert str(component.get_collision_profile_name()) == 'BlockAll'
    assert component.static_mesh.get_editor_property('body_setup') is not None
assert starts[0].get_actor_location().z == 110
assert world.get_world_settings().get_editor_property('default_game_mode') is None, 'Mountain Gate must inherit PunchOutGameMode from the project'
assert not unreal.EditorAssetLibrary.does_asset_exist('/Game/Levels/L_OfficePrototype')
unreal.log('PUNCHOUT_VERIFY_OK: saved map loads; one player start; floors block pawn and have physics bodies; game mode inherited; office map absent. PIE movement remains to be checked.')
