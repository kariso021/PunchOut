"""Connect the existing character Blueprint to the Core Pawn without replacing its input graph."""
import json
from pathlib import Path
import shutil
import unreal

project = Path(unreal.Paths.project_dir()).resolve()
path = "/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"
bp = unreal.load_asset(path)
parent = unreal.load_class(None, "/Script/PunchOutCore.PunchOutCharacter")
assert bp and parent
old_parent = str(unreal.EditorAssetLibrary.find_asset_data(path).get_tag_value("ParentClass"))
assert "/Script/Engine.Character'" in old_parent or "/Script/PunchOutCore.PunchOutCharacter'" in old_parent

def snapshot():
    defaults = unreal.get_default_object(bp.generated_class())
    movement = defaults.get_editor_property("character_movement")
    mesh = defaults.get_editor_property("mesh")
    return {
        "speed": movement.get_editor_property("max_walk_speed"),
        "jump": movement.get_editor_property("jump_z_velocity"),
        "mesh": mesh.get_editor_property("skeletal_mesh_asset").get_path_name(),
        "animation": mesh.get_editor_property("anim_class").get_path_name(),
        "replicates": defaults.get_editor_property("replicates"),
    }

before = snapshot()
backup = project / "Saved/CoreMigration/BP_ThirdPersonCharacter.before-core.uasset"
backup.parent.mkdir(parents=True, exist_ok=True)
if not backup.exists():
    shutil.copy2(project / "Content/ThirdPerson/Blueprints/BP_ThirdPersonCharacter.uasset", backup)
unreal.BlueprintEditorLibrary.reparent_blueprint(bp, parent)
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
assert snapshot() == before, "Character defaults changed; asset not saved."
assert unreal.EditorAssetLibrary.save_loaded_asset(bp, only_if_is_dirty=False)
result = {"old_parent": old_parent, "new_parent": parent.get_path_name(), "preserved": before}
(backup.parent / "result.json").write_text(json.dumps(result, indent=2), encoding="utf-8")
unreal.log("CORE_MIGRATION " + json.dumps(result))
