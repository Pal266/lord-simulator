"""Run the verified native importer once per editor tick; stop on the first failure.

In Unreal's Cmd console:
  py "<game>/Scripts/import_region_in_editor.py" "<docs>/fictional_map/regions/southern_coast_range"

Requires the LordSimulatorEditor module compiled for the running editor.
The scientific pipeline and prepare_region_inputs.py must have passed first.
"""
import json
from pathlib import Path
import sys
import time
import unreal

source = Path(sys.argv[1]).resolve()
project = Path(unreal.Paths.project_dir()).resolve()
reports = project / 'Saved/RegionImport'
provenance = json.loads((reports / 'source-run.json').read_text())
assert provenance['tile_count'] == 100 and provenance['world_seed'] == 7
all_tiles = [f'tile_x{x}_y{y}' for y in range(10) for x in range(10)]
queue = sys.argv[2:] or all_tiles.copy()
assert len(queue) == len(set(queue)) and set(queue).issubset(all_tiles), 'Invalid or duplicate tile selection'
status_path = reports / ('batch-status.json' if len(queue) == 100 else 'subset-status.json')
state = {'complete': [], 'pending': None, 'failed': None}
handle = None
next_time = time.monotonic() + 1
busy = False


def stop(error=None):
    global handle
    state['failed'] = error
    status_path.write_text(json.dumps(state, indent=2))
    unreal.unregister_slate_post_tick_callback(handle)
    handle = None
    if error:
        unreal.log_error(error)
    else:
        unreal.log(f"All {len(state['complete'])} requested native plot imports and source-data comparisons passed.")


def tick(delta_seconds):
    global next_time, busy
    if busy or time.monotonic() < next_time:
        return
    try:
        pending = state['pending']
        if pending:
            name, started = pending
            path = reports / (name + '.json')
            assert path.is_file() and path.stat().st_mtime_ns > started, f'{name}: native importer did not produce a fresh success report'
            report = json.loads(path.read_text(encoding='utf-8-sig'))
            water = json.loads((source / 'export' / name / 'water.json').read_text())
            assert report['tile'] == name and report['vertices'] == 4033
            assert report['all_encoded_heights_unchanged']
            assert report['rivers'] == len(water['rivers']) and report['lakes'] == len(water['lakes'])
            state['complete'].append(name)
            state['pending'] = None
            status_path.write_text(json.dumps(state, indent=2))
        if not queue:
            stop()
            return
        name = queue.pop(0)
        state['pending'] = [name, time.time_ns()]
        status_path.write_text(json.dumps(state, indent=2))
        busy = True
        try:
            unreal.SystemLibrary.execute_console_command(None,
                f'LordSimulator.ImportRegion SourceRegion="{source.as_posix()}" Tile={name}')
        finally:
            busy = False
        next_time = time.monotonic() + 1
    except Exception as error:
        stop(str(error))


handle = unreal.register_slate_post_tick_callback(tick)
unreal.log(f'Started {len(queue)}-plot offline import, with no water filtering.')
