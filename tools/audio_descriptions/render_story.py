"""Adapt Echos of Gaia's verified offline Fish renderer to reviewed MVCI windows.

The reference checkout is read-only. Credentials/references/takes stay private;
only a separately validated final pack is eligible for deployment.
"""
import argparse
import hashlib
import importlib
import json
import math
import re
import sys
from pathlib import Path

sys.dont_write_bytecode = True


def validate_request(directory):
    def unique(pairs):
        result = {}
        for key, value in pairs:
            if key in result: raise ValueError('Duplicate request key')
            result[key] = value
        return result

    raw = (Path(directory) / 'manifest.json').read_bytes()
    if len(raw) > 1048576: raise ValueError('Oversized render request')
    data = json.loads(raw.decode('utf-8-sig'), object_pairs_hook=unique)
    return validate_request_data(data)


def validate_request_data(data):
    if not isinstance(data, dict): raise ValueError('Render request must be an object')
    if (set(data) != {'schema', 'model', 'clips', 'films'} or data['schema'] != 1 or
            data['model'] != 's2.1-pro-free' or not isinstance(data['clips'], list) or
            not isinstance(data['films'], list) or not 1 <= len(data['clips']) <= 4096):
        raise ValueError('Unsupported or private render request')
    clips = set()
    for row in data['clips']:
        if (set(row) != {'id', 'text'} or not isinstance(row['id'], str) or
                not re.fullmatch(r'[a-z0-9][a-z0-9-]*', row['id']) or row['id'] in clips or
                not isinstance(row['text'], str) or not 1 <= len(row['text']) <= 2048 or
                not row['text'].isprintable() or not any(c.isalnum() for c in row['text'])):
            raise ValueError('Invalid reviewed caption')
        clips.add(row['id'])
    movies, assigned = set(), set()
    for film in data['films']:
        if (set(film) != {'movie', 'sourceSHA256', 'sourceBytes', 'duration', 'sequenceOffset', 'fps', 'nativeFrames', 'cues'} or
                not isinstance(film['movie'], str) or not re.fullmatch(r'[0-9]-[A-Z]_DEMO[0-9]{2}', film['movie']) or
                film['movie'] in movies or not re.fullmatch(r'[a-f0-9]{64}', film['sourceSHA256']) or
                type(film['sourceBytes']) is not int or film['sourceBytes'] <= 0 or film['fps'] != 30 or
                type(film['nativeFrames']) is not int or not 1 <= film['nativeFrames'] <= 10000000 or
                not isinstance(film['cues'], list) or not 1 <= len(film['cues']) <= 512):
            raise ValueError('Invalid source movie')
        if (not all(type(film[k]) in (int, float) and math.isfinite(film[k]) for k in ('duration', 'sequenceOffset')) or
                not 0 < film['duration'] <= 3600 or not 0 <= film['sequenceOffset'] <= 3600 or
                abs(film['duration'] - film['nativeFrames']/30) > .034):
            raise ValueError('Source clock/rate mismatch')
        movies.add(film['movie'])
        previous = 0
        for cue in film['cues']:
            if (set(cue) != {'id', 'start', 'end'} or cue['id'] not in clips or cue['id'] in assigned or
                    not all(type(cue[k]) in (int, float) and math.isfinite(cue[k]) for k in ('start', 'end')) or
                    cue['start'] < previous or cue['end'] <= cue['start'] or cue['end'] > film['duration']):
                raise ValueError('Invalid/overlapping source window')
            previous = cue['end']
            assigned.add(cue['id'])
    if assigned != clips: raise ValueError('Every caption needs exactly one reviewed window')
    return data


def main():
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument('--ff9-tools', type=Path, required=True)
    args, remaining = parser.parse_known_args()
    reference = args.ff9_tools.resolve()
    if not (reference / 'render_fish.py').is_file():
        raise ValueError('Echos of Gaia renderer is unavailable')
    sys.path.insert(0, str(reference))
    renderer = importlib.import_module('render_fish')
    renderer.validate_pack = validate_request
    previous_open = renderer.open_job

    def open_job(root, identity):
        identity['mvci_adapter_sha256'] = hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
        previous_open(root, identity)

    renderer.open_job = open_job
    sys.argv = [sys.argv[0]] + remaining
    renderer.main()


if __name__ == '__main__':
    main()
