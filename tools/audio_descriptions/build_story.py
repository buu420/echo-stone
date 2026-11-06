"""Validate the public MVCI narration pack and compile its fixed runtime catalog."""
import argparse
import hashlib
import io
import json
import math
import re
import sys
import wave
from pathlib import Path
from render_story import validate_request_data
sys.dont_write_bytecode = True


def validate_pack(root):
    root = Path(root)
    def read_json(name):
        raw = (root/name).read_bytes()
        if len(raw)>1048576: raise ValueError('Oversized public metadata')
        def unique(pairs):
            result = {}
            for k,v in pairs:
                if k in result: raise ValueError('Duplicate public metadata key')
                result[k]=v
            return result
        return json.loads(raw.decode('utf-8-sig'),object_pairs_hook=unique)
    manifest = read_json('manifest.json')
    if (set(manifest) != {'schema','product','version','model','voice','films','clips'} or
            manifest['product'] != 'MVCIAccess' or manifest['version'] != '0.1.0' or
            manifest['voice'] != 'User-authorized personal voice; generated offline'):
        raise ValueError('Private or unsupported public metadata')
    request=dict(schema=manifest['schema'],model=manifest['model'],films=manifest['films'],
                 clips=[dict(id=c['id'],text=c['text']) for c in manifest['clips']])
    validate_request_data(request)
    records={}
    for c in manifest['clips']:
        if (set(c) != {'id','text','file','sha256','bytes','duration'} or c['file'] != c['id']+'.wav' or
                not re.fullmatch(r'[a-f0-9]{64}',c['sha256']) or type(c['bytes']) is not int or not 46<=c['bytes']<=2097152):
            raise ValueError('Invalid recording identity')
        file=root/c['file']
        if file.is_symlink(): raise ValueError('Symlink in public recordings')
        raw=file.read_bytes()
        if len(raw)!=c['bytes'] or hashlib.sha256(raw).hexdigest()!=c['sha256']:
            raise ValueError('Changed public recording')
        with wave.open(io.BytesIO(raw),'rb') as w:
            if (w.getparams()[:3] != (1,2,24000) or raw[36:40]!=b'data' or
                    len(raw)!=44+2*w.getnframes() or int.from_bytes(raw[4:8],'little')!=len(raw)-8 or type(c['duration']) not in (int,float) or not math.isfinite(c['duration']) or
                    abs(w.getnframes()/24000-c['duration'])>.000001):
                raise ValueError('Noncanonical recording or duration')
        records[c['id']]=c
    if {p.name for p in root.iterdir()} != {'manifest.json','coverage.json'}|{c['file'] for c in records.values()}:
        raise ValueError('Unlisted or private file in public recordings')
    for film in manifest['films']:
        size=0
        for cue in film['cues']:
            clip=records[cue['id']]
            if clip['duration']+.15>cue['end']-cue['start']+1e-9: raise ValueError('Voice lacks dispatch room in the reviewed window')
            size+=clip['bytes']
        if size>33554432: raise ValueError('Movie bank exceeds32MiB')
    coverage=read_json('coverage.json')
    if set(coverage)!={'schema','total','films'} or coverage['schema']!=1 or coverage['total']!=80 or len(coverage['films'])!=80:
        raise ValueError('Incomplete coverage ledger')
    seen={};
    for row in coverage['films']:
        if (set(row)!={'movie','recorded_clips','visual_preroll_described'} or
                not re.fullmatch(r'[0-9]-[A-Z]_DEMO[0-9]{2}',row['movie']) or row['movie'] in seen or
                type(row['recorded_clips']) is not int or not 0<=row['recorded_clips']<=512 or
                type(row['visual_preroll_described']) is not bool):
            raise ValueError('Invalid film coverage row')
        seen[row['movie']]=row['recorded_clips']
    expected={f['movie']:len(f['cues']) for f in manifest['films']}
    if any(n!=expected.get(movie,0) for movie,n in seen.items()) or not set(expected)<=set(seen):
        raise ValueError('Coverage differs from recorded cues')
    return manifest


def catalog_source(manifest):
    clips = {c['id']: c for c in manifest['clips']}
    lines = ['// Generated from the validated public narration manifest. Do not hand edit.',
             '#include "mvci/descriptions.hpp"', 'namespace mvci {',
             'const std::vector<DescriptionMovie>& description_catalog(){',
             ' static const std::vector<DescriptionMovie> movies{']
    for film in manifest['films']:
        lines.append('  {L'+json.dumps(film['movie'])+','+repr(film['duration'])+','+repr(film['sequenceOffset'])+',{')
        for cue in film['cues']:
            c = clips[cue['id']]
            lines.append('   {'+','.join([json.dumps(c['id']), json.dumps(c['file']), json.dumps(c['sha256']),
                                           'L'+json.dumps(c['text'], ensure_ascii=True), repr(cue['start']),
                                           repr(cue['end']), repr(c['duration']), str(c['bytes'])])+'},')
        lines.append('  }},')
    lines += [' };return movies;', '}', '}', '']
    return '\n'.join(lines)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pack', type=Path, required=True)
    parser.add_argument('--write-catalog', type=Path)
    parser.add_argument('--check-catalog', type=Path)
    args = parser.parse_args()
    manifest = validate_pack(args.pack)
    source = catalog_source(manifest)
    if args.write_catalog: args.write_catalog.write_text(source, encoding='utf-8')
    if args.check_catalog and args.check_catalog.read_text(encoding='utf-8') != source:
        raise ValueError('Compiled narration catalog differs from public pack')
    print(json.dumps(dict(films=len(manifest['films']), clips=len(manifest['clips']),
                          wave_bytes=sum(c['bytes'] for c in manifest['clips']))))


if __name__ == '__main__': main()
