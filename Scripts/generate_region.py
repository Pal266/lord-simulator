"""Run the repository's documented generation commands, stopping on any failure."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import time

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--docs-repo', required=True, type=Path)
parser.add_argument('--output-dir', required=True, type=Path)
args = parser.parse_args()
repo = args.docs_repo.resolve()
planet = repo / 'fictional_map/scripts'
region = repo / 'fictional_map/regions/southern_coast_range'
logs = args.output_dir.resolve()
logs.mkdir(parents=True, exist_ok=True)
manifest = {
    'source_commit': subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=repo, text=True).strip(),
    'region_config': json.loads((region / 'region.json').read_text(encoding='utf-8-sig')),
    'source_sha256': {}, 'steps': [],
}
for path in sorted((repo / 'fictional_map').rglob('*.py')):
    manifest['source_sha256'][str(path.relative_to(repo))] = hashlib.sha256(path.read_bytes()).hexdigest()
manifest['source_sha256']['region.json'] = hashlib.sha256((region / 'region.json').read_bytes()).hexdigest()
steps = []
for command in [['plates.py', '7'], ['heightmap.py'], ['climate.py'], ['hydrology.py']]:
    steps.extend([(planet, command), (planet, ['validate.py'])])
steps.extend((region / 'scripts', command) for command in [
    ['extract_region.py'], ['regional_model.py'], ['validate_region.py'],
    ['refine_25m.py'], ['validate_landforms.py'], ['detail_plots.py', '--tiles', 'all'],
    ['validate_plots.py'], ['export_water.py'], ['validate_water.py'],
])
env = dict(os.environ, PYTHONUTF8='1', PYTHONUNBUFFERED='1', MPLBACKEND='Agg')
for index, (cwd, args) in enumerate(steps):
    log = logs / f'{index:02d}-{Path(args[0]).stem}.log'
    step = dict(command=[sys.executable, *args], cwd=str(cwd), log=str(log), started=time.time())
    manifest['steps'].append(step)
    (logs / 'manifest.json').write_text(json.dumps(manifest, indent=2), encoding='utf-8')
    print(f'START {index}: {" ".join(args)}', flush=True)
    with log.open('w', encoding='utf-8') as output:
        result = subprocess.run([sys.executable, '-u', *args], cwd=cwd, env=env, stdout=output, stderr=subprocess.STDOUT)
    step.update(exit_code=result.returncode, finished=time.time())
    (logs / 'manifest.json').write_text(json.dumps(manifest, indent=2), encoding='utf-8')
    print(f'END {index}: exit {result.returncode}; {step["finished"]-step["started"]:.1f}s', flush=True)
    if result.returncode:
        print(log.read_text(encoding='utf-8')[-6000:], flush=True)
        sys.exit(result.returncode)
manifest['outputs_sha256'] = {}
for pattern in ['export/tile_*/heightmap.png', 'export/tile_*/water.json', 'output/*.npz', 'output/validation*.json']:
    for path in sorted(region.glob(pattern)):
        manifest['outputs_sha256'][str(path.relative_to(region))] = hashlib.sha256(path.read_bytes()).hexdigest()
(logs / 'manifest.json').write_text(json.dumps(manifest, indent=2), encoding='utf-8')
print('ALL GENERATION AND VALIDATION STAGES PASSED', flush=True)
