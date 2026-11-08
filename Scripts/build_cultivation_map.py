"""Build the playable mountain-gate blockout in Unreal Editor."""
import math
import unreal

actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assets = unreal.EditorAssetLibrary
path = '/Game/Levels/L_MountainGate'
assert levels.new_level(path), 'Could not create mountain-gate map'

def material(name, color):
    result = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        name, '/Game/Cultivation/Materials', unreal.Material, unreal.MaterialFactoryNew())
    expression = unreal.MaterialEditingLibrary.create_material_expression(
        result, unreal.MaterialExpressionConstant3Vector)
    expression.set_editor_property('constant', unreal.LinearColor(*color, 1))
    unreal.MaterialEditingLibrary.connect_material_property(expression, '', unreal.MaterialProperty.MP_BASE_COLOR)
    unreal.MaterialEditingLibrary.recompile_material(result)
    assets.save_loaded_asset(result)
    return result

stone = material('M_GateStone', (0.19, 0.24, 0.25))
ground = material('M_MountainEarth', (0.13, 0.18, 0.12))
wood = material('M_GateTimber', (0.22, 0.065, 0.035))
roof = material('M_GateRoof', (0.055, 0.09, 0.11))

def shape(name, mesh, location, scale, mat, rotation=(0, 0, 0)):
    actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*location), unreal.Rotator(*rotation))
    actor.set_actor_label(name)
    actor.set_actor_scale3d(unreal.Vector(*scale))
    actor.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/' + mesh))
    actor.static_mesh_component.set_material(0, mat)
    return actor

# Flat, connected collision surfaces: gate terrace, training court, descending path.
shape('Training court', 'Cube', (0, 0, -70), (32, 26, 1.4), stone)
shape('Mountain shelf', 'Cube', (-1500, 0, -145), (65, 42, 1.5), ground)
for i in range(12):
    shape('Descent step %02d' % i, 'Cube', (-1750-i*150, 0, -15-i*12), (1.5, 8, 0.3), stone)
for y in (-1100, 1100):
    shape('Gate pillar', 'Cube', (1200, y, 350), (1.4, 1.4, 7), wood)
    shape('Boundary wall', 'Cube', (1400, y*1.65, 200), (1.5, 12, 4), stone)
shape('Sealed mountain gate', 'Cube', (1400, 0, 280), (1, 20, 5.6), wood)
shape('Gate lintel', 'Cube', (1200, 0, 710), (2, 27, 1), wood)
for y, angle in ((-750, -22), (750, 22)):
    shape('Sloping tiled roof', 'Cube', (1200, y, 870), (5, 17, 0.65), roof, (0, 0, angle))
for i in range(14):
    angle = i * math.tau / 14
    x, y = math.cos(angle)*6500, math.sin(angle)*6500
    shape('Distant peak %02d' % i, 'Cone', (x, y, 500), (18+i%3*5, 18+i%3*5, 30+i%4*10), stone)
for x, y in ((-1000, -1000), (-500, 1100), (-2200, 1500)):
    shape('Weathered rock', 'Sphere', (x, y, 100), (3, 2, 3), stone)
    shape('Pine trunk', 'Cylinder', (x, y, 400), (0.55, 0.55, 7), wood)
    shape('Pine crown', 'Cone', (x, y, 820), (5, 5, 9), ground)
sun = actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 1500), unreal.Rotator(-25, -35, 0))
sun.light_component.set_editor_property('intensity', 5)
sun.light_component.set_light_color(unreal.LinearColor(1, 0.79, 0.58))
actors.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector())
sky = actors.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 1200))
sky.light_component.set_editor_property('real_time_capture', True)
actors.spawn_actor_from_class(unreal.ExponentialHeightFog, unreal.Vector(0, 0, -200))
start = actors.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(550, 0, 110), unreal.Rotator(0, 180, 0))
start.set_actor_label('Exiled disciple start')
assert levels.save_current_level(), 'Map save failed'
assert assets.does_asset_exist(path)
assert len(actors.get_all_level_actors()) > 40
# Remove the obsolete map only after the replacement has been saved.
assert assets.delete_asset('/Game/Levels/L_OfficePrototype'), 'Office map deletion failed'
unreal.log('PUNCHOUT_MOUNTAIN_GATE_OK: map saved, player start and collision geometry created; office map removed')
