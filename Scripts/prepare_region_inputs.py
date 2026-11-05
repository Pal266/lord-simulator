"""Verify a successful canonical generation run and derive its technical overview.

Uses only the offline pipeline's existing NumPy/SciPy/Pillow dependencies.
No part of this script is needed by the packaged game.
"""
import argparse
import hashlib
import json
from pathlib import Path

import numpy as np
from PIL import Image
from scipy.ndimage import map_coordinates


def digest(path):
    result = hashlib.sha256()
    with path.open('rb') as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b''):
            result.update(chunk)
    return result.hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-region', required=True, type=Path)
    parser.add_argument('--generation-manifest', required=True, type=Path)
    parser.add_argument('--output-dir', required=True, type=Path)
    args = parser.parse_args()
    region = args.source_region.resolve()
    manifest = json.loads(args.generation_manifest.read_text(encoding='utf-8-sig'))
    config = json.loads((region / 'region.json').read_text(encoding='utf-8'))
    expected_tiles = {f'tile_x{x}_y{y}' for y in range(10) for x in range(10)}
    assert config['world_seed'] == 7 and config['size_km'] == 100.0
    assert all(config['tiles'][key] == value for key, value in {
        'count_x': 10, 'count_y': 10, 'size_km': 10.0, 'vertices': 4033}.items())
    assert manifest['region_config'] == config
    assert len(manifest['steps']) == 17 and all(step.get('exit_code') == 0 for step in manifest['steps'])
    assert set(p.name for p in (region / 'export').iterdir() if p.is_dir()) == expected_tiles
    repo = region.parents[2]
    for relative, expected in manifest['source_sha256'].items():
        path = region / relative if relative == 'region.json' else repo / relative
        assert digest(path) == expected, f'Generation source changed: {path}'
    for relative, expected in manifest['outputs_sha256'].items():
        assert digest(region / relative) == expected, f'Generation output changed: {relative}'
    rivers = lakes = 0
    for name in sorted(expected_tiles):
        folder = region / 'export' / name
        with Image.open(folder / 'heightmap.png') as image:
            assert image.size == (4033, 4033) and image.mode in ('I;16', 'I'), name
        water = json.loads((folder / 'water.json').read_text())
        assert water['plot'] == name
        rivers += len(water['rivers'])
        lakes += len(water['lakes'])
    # The canonical refined regional elevation grid covers the square plus its model margin.
    # Sample only the 100 km square, north-up, using bilinear interpolation at pixel centres.
    with np.load(region / 'output/refined_25m.npz') as grid:
        x, y, elevation = grid['x_m'], grid['y_m'], grid['elevation_m']
        assert x.ndim == y.ndim == 1 and x[0] < x[-1] and y[0] > y[-1]
        inside_x = np.flatnonzero((x >= -50000) & (x <= 50000))
        inside_y = np.flatnonzero((y >= -50000) & (y <= 50000))
        square = elevation[np.ix_(inside_y, inside_x)]
        low, high = float(square.min()), float(square.max())
        assert np.isfinite(square).all() and high > low
        samples = -50000.0 + (np.arange(1024) + 0.5) * (100000.0 / 1024)
        cols, rows = np.meshgrid((samples - x[0]) / (x[1] - x[0]),
                                 (samples[::-1] - y[0]) / (y[1] - y[0]))
        overview = map_coordinates(elevation, [rows, cols], order=1, mode='nearest', prefilter=False)
        pixels = np.rint(np.clip((overview - low) / (high - low), 0, 1) * 255).astype(np.uint8)
    args.output_dir.mkdir(parents=True, exist_ok=True)
    image_path = args.output_dir / 'region-overview.png'
    Image.fromarray(pixels).save(image_path)
    provenance = {
        'source_commit': manifest['source_commit'], 'world_seed': 7, 'region_config': config,
        'source_sha256': manifest['source_sha256'], 'outputs_sha256': manifest['outputs_sha256'],
        'overview': {'source': 'output/refined_25m.npz', 'resolution': [1024, 1024],
                     'interpolation': 'bilinear at pixel centres', 'north_up': True,
                     'normalization_min_m': low, 'normalization_max_m': high,
                     'sha256': digest(image_path)},
        'tile_count': 100, 'river_count': rivers, 'lake_count': lakes,
    }
    (args.output_dir / 'source-run.json').write_text(json.dumps(provenance, indent=2), encoding='utf-8')
    print(f'Verified 100 heightmaps, {rivers} rivers, {lakes} lakes; wrote {image_path}')


if __name__ == '__main__':
    main()
