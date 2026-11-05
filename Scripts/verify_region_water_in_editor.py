"""Verify native surface meshes in all saved plots, without editing or saving levels.

Editor Cmd: py "<game>/Scripts/verify_region_water_in_editor.py" "<source-region>"
"""
import json
from pathlib import Path
import sys
import time
import unreal

source = Path(sys.argv[1]).resolve()
reports = Path(unreal.Paths.project_saved_dir()).resolve() / 'RegionImport/WaterMeshes'
reports.mkdir(parents=True, exist_ok=True)
queue = [f'tile_x{x}_y{y}' for y in range(10) for x in range(10)]
state = {'complete': [], 'pending': None, 'failed': None}
handle = None
busy = False
next_time = time.monotonic() + 1


def tick(delta_seconds):
    global handle, busy, next_time
    if busy or time.monotonic() < next_time:
        return
    try:
        if state['pending']:
            name, started = state['pending']
            path = reports / (name + '.json')
            assert path.is_file() and path.stat().st_mtime_ns > started, f'{name}: missing fresh mesh report'
            report = json.loads(path.read_text(encoding='utf-8-sig'))
            expected = json.loads((source / 'export' / name / 'water.json').read_text(encoding='utf-8'))
            assert report['rivers'] == len(expected['rivers']) and report['lakes'] == len(expected['lakes'])
            assert not report['failed_bodies'], f"{name}: unsupported surfaces {report['failed_bodies']}"
            assert report['nonempty_surface_meshes'] == report['rivers'] + report['lakes']
            state['complete'].append(name)
            state['pending'] = None
        if not queue:
            unreal.unregister_slate_post_tick_callback(handle)
            handle = None
            unreal.log('All 100 saved levels have complete nonempty native water surface meshes.')
        else:
            name = queue.pop(0)
            state['pending'] = [name, time.time_ns()]
            busy = True
            try:
                unreal.SystemLibrary.execute_console_command(None, f'LordSimulator.VerifyRegionWater Tile={name}')
            finally:
                busy = False
            next_time = time.monotonic() + 1
    except Exception as error:
        state['failed'] = str(error)
        unreal.unregister_slate_post_tick_callback(handle)
        handle = None
        unreal.log_error(str(error))
    (reports / 'batch-status.json').write_text(json.dumps(state, indent=2), encoding='utf-8')


handle = unreal.register_slate_post_tick_callback(tick)
unreal.log('Started read-only verification of every saved native water surface.')
