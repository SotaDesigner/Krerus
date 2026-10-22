import unreal
import json
from pathlib import Path

output_folder = Path(unreal.Paths.project_saved_dir()) / "Tests/Hito003"
output_folder.mkdir(parents=True, exist_ok=True)

result = {'blueprints': [], 'character': {}, 'mappings': []}
for path in ['/Game/Core/Character/BP_KrerusCharacter', '/Game/Core/Character/AB_KrerusCharacter', '/Game/Core/GameModes/BP_KrerusGameMode']:
    asset = unreal.load_asset(path)
    assert asset, path
    unreal.BlueprintEditorLibrary.compile_blueprint(asset)
    status = str(asset.get_editor_property('status'))
    assert 'UP_TO_DATE' in status, path + ': ' + status
    result['blueprints'].append({'path': path, 'status': status})

bp = unreal.load_asset('/Game/Core/Character/BP_KrerusCharacter')
defaults = unreal.get_default_object(bp.generated_class())
for prop in ['default_mapping_context', 'move_action', 'look_action', 'jump_action', 'sprint_action', 'camera_control_action', 'camera_free_action', 'camera_zoom_action']:
    value = defaults.get_editor_property(prop)
    assert value, 'Referencia vacia: ' + prop
    result['character'][prop] = value.get_path_name()
for prop in ['walk_speed', 'min_camera_distance', 'max_camera_distance', 'camera_zoom_step', 'camera_zoom_interp_speed', 'camera_follow_speed', 'camera_follow_delay']:
    result['character'][prop] = defaults.get_editor_property(prop)
for prop in ['camera_boom', 'follow_camera', 'camera_system']:
    value = defaults.get_editor_property(prop)
    assert value, 'Componente vacio: ' + prop
    result['character'][prop] = value.get_name()
boom = defaults.get_editor_property('camera_boom')
for prop in ['target_arm_length', 'enable_camera_lag', 'camera_lag_speed', 'enable_camera_rotation_lag', 'do_collision_test']:
    result['character']['spring_arm_' + prop] = boom.get_editor_property(prop)
context = defaults.get_editor_property('default_mapping_context')
for mapping in context.get_editor_property('default_key_mappings').get_editor_property('mappings'):
    result['mappings'].append(str(mapping))
assert result['mappings'], 'Mapping Context sin mappings'
assert unreal.load_class(None, '/Script/Krerus.KrerusPlayerController')
(output_folder / "003-blueprints.json").write_text(json.dumps(result, indent=2))
unreal.log('HITO003_VALIDATION_OK')
