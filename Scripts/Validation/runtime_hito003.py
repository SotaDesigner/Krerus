import unreal
import time
import json
import traceback
from pathlib import Path

output_folder = Path(unreal.Paths.project_saved_dir()) / "Tests/Hito003"
output_folder.mkdir(parents=True, exist_ok=True)

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
output = (output_folder / "003-runtime.json")
level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
unreal.EditorLoadingAndSavingUtils.load_map('/Game/Dev/Maps/L_DevSandbox')
state = {'start': time.monotonic(), 'step': -1, 'checks': []}

def check(name, passed, detail):
    state['checks'].append({'test': name, 'passed': bool(passed), 'detail': str(detail)})
    output.write_text(json.dumps(state['checks'], indent=2))
    unreal.log('HITO003_TEST ' + name + ' ' + str(passed) + ' ' + str(detail))

def command(text):
    unreal.SystemLibrary.execute_console_command(state['world'], text, state['pc'])

def step():
    pc = state['pc']
    pawn = state['pawn']
    boom = pawn.get_editor_property('camera_boom')
    movement = pawn.get_editor_property('character_movement')
    cursor = lambda: pc.get_editor_property('show_mouse_cursor')
    index = state['step']
    if index == 0:
        check('cursor_idle', cursor(), pc.get_class().get_name())
        state['position'] = pawn.get_actor_location()
        command('Input.+key W')
    elif index == 1:
        distance = (pawn.get_actor_location() - state['position']).length()
        check('W_movement', distance > 10, distance)
        command('Input.+key LeftShift')
    elif index == 2:
        check('sprint', abs(movement.max_walk_speed - 600) < 1, movement.max_walk_speed)
        command('Input.-key LeftShift')
    elif index == 3:
        check('sprint_return', abs(movement.max_walk_speed - 300) < 2, movement.max_walk_speed)
        command('Input.-key W')
        command('Input.+key LeftMouseButton')
    elif index == 4:
        check('left_cursor_hidden', not cursor(), cursor())
        state['rotation'] = pc.get_control_rotation()
        command('Input.+action IA_Look X=1.0 Y=0.0')
    elif index == 5:
        yaw = pc.get_control_rotation().yaw - state['rotation'].yaw
        check('left_orbit', abs(yaw) > 1, yaw)
        command('Input.-action IA_Look')
        command('Input.+key RightMouseButton')
    elif index == 6:
        check('both_buttons', not cursor() and pawn.get_editor_property('is_camera_control_active'), cursor())
        command('Input.-key LeftMouseButton')
    elif index == 7:
        check('right_remains_active', not cursor() and pawn.get_editor_property('is_camera_control_active'), cursor())
        command('Input.-key RightMouseButton')
    elif index == 8:
        check('release_both', cursor() and not pawn.get_editor_property('is_camera_control_active'), cursor())
        state['rotation'] = pc.get_control_rotation()
        command('Input.+action IA_Look X=1.0 Y=0.0')
    elif index == 9:
        yaw = pc.get_control_rotation().yaw - state['rotation'].yaw
        check('idle_ignores_look', abs(yaw) < 0.1, yaw)
        command('Input.-action IA_Look')
        command('Input.+key MouseWheelAxis 1')
    elif index == 10:
        command('Input.-key MouseWheelAxis')
        state['zoom_in'] = boom.target_arm_length
        check('zoom_in', 299 <= boom.target_arm_length < 1400, boom.target_arm_length)
        command('Input.+key MouseWheelAxis -1')
    elif index == 11:
        command('Input.-key MouseWheelAxis')
        check('zoom_out', state['zoom_in'] < boom.target_arm_length <= 1801, boom.target_arm_length)
        command('Input.+key RightMouseButton')
        command('Input.+action IA_Look X=0.0 Y=10.0')
    elif index == 12:
        pitch = pc.get_control_rotation().pitch
        if pitch > 180:
            pitch -= 360
        check('pitch_limit', -75.1 <= pitch <= 45.1, pitch)
        command('Input.-action IA_Look')
        pc.set_control_rotation(unreal.Rotator(pitch=0, yaw=90, roll=0))
        state['position'] = pawn.get_actor_location()
        command('Input.+key W')
    elif index == 13:
        distance = pawn.get_actor_location() - state['position']
        check('right_movement_camera_yaw', distance.y > 10 and abs(distance.x) < abs(distance.y)*0.25, distance)
        command('Input.-key RightMouseButton')
    elif index == 14:
        state['position'] = pawn.get_actor_location()
        command('Input.+key LeftMouseButton')
        pc.set_control_rotation(unreal.Rotator(pitch=0, yaw=180, roll=0))
    elif index == 15:
        distance = pawn.get_actor_location() - state['position']
        check('free_movement_keeps_reference', distance.y > 10 and abs(distance.x) < abs(distance.y)*0.25, distance)
        command('Input.-key W')
        command('Input.+key RightMouseButton')
    elif index == 16:
        command('Input.-key RightMouseButton')
    elif index == 17:
        check('left_remains_active', not cursor() and not pawn.get_editor_property('is_camera_control_active'), cursor())
        command('Input.-key LeftMouseButton')
    elif index == 18:
        check('release_both_reverse', cursor(), cursor())
        level.editor_request_end_play()
        state['ending'] = True

def tick(dt):
    try:
        now = time.monotonic()
        worlds = unreal.EditorLevelLibrary.get_pie_worlds(False)
        if state.get('ending') and not worlds:
            unreal.unregister_slate_post_tick_callback(handle)
            unreal.SystemLibrary.quit_editor()
            return
        if state['step'] == -1 and worlds:
            pc = unreal.GameplayStatics.get_player_controller(worlds[0], 0)
            pawn = unreal.GameplayStatics.get_player_pawn(worlds[0], 0)
            if pc and pawn:
                state.update(world=worlds[0], pc=pc, pawn=pawn, step=0, last=now)
        elif state['step'] >= 0 and not state.get('ending') and now-state['last'] > 1.5:
            step()
            state['step'] += 1
            state['last'] = now
        if now-state['start'] > 90:
            raise RuntimeError('Timeout en paso ' + str(state['step']))
    except Exception:
        check('runtime_exception', False, traceback.format_exc())
        level.editor_request_end_play()
        unreal.unregister_slate_post_tick_callback(handle)
        unreal.SystemLibrary.quit_editor()

handle = unreal.register_slate_post_tick_callback(tick)
level.editor_request_begin_play()
