import hashlib
import importlib.util
import io
import json
import sys
import tempfile
import unittest
import wave
from pathlib import Path

sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools/audio_descriptions'))
from build_story import validate_pack, catalog_source


class PublicPackTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)
        memory = io.BytesIO()
        with wave.open(memory, 'wb') as f:
            f.setparams((1, 2, 24000, 0, 'NONE', 'not compressed'))
            f.writeframes(b'\0\0' * 24000)
        data = memory.getvalue()
        self.wave_bytes = data
        (self.root / 'test.wav').write_bytes(data)
        self.manifest = dict(schema=1, product='MVCIAccess', version='0.1.0', model='s2.1-pro-free',
                             voice='User-authorized personal voice; generated offline',
                             clips=[dict(id='test', text='A cloaked woman.', file='test.wav', sha256=hashlib.sha256(data).hexdigest(), bytes=len(data), duration=1.)],
                             films=[dict(movie='0-A_DEMO01', sourceSHA256='a'*64, sourceBytes=400, duration=90., sequenceOffset=7.25, fps=30, nativeFrames=2700, cues=[dict(id='test', start=.5, end=3.)])])
        (self.root / 'coverage.json').write_text(json.dumps(dict(schema=1, total=80, films=[dict(movie=f'{n//26}-{chr(65+n%26)}_DEMO01', recorded_clips=1 if n==0 else 0, visual_preroll_described=False) for n in range(80)])))
        self.save()

    def tearDown(self): self.temp.cleanup()
    def save(self): (self.root / 'manifest.json').write_text(json.dumps(self.manifest))

    def test_exact_public_pack_compiles(self):
        manifest = validate_pack(self.root)
        self.assertIn('A cloaked woman.', catalog_source(manifest))

    def test_corrupt_audio_and_private_extra_files_are_rejected(self):
        with (self.root / 'test.wav').open('ab') as f: f.write(b'changed')
        with self.assertRaises(ValueError): validate_pack(self.root)
        (self.root / 'test.wav').write_bytes(self.wave_bytes)
        (self.root / 'reference-profile.json').write_text('{}')
        with self.assertRaises(ValueError): validate_pack(self.root)

    def test_voice_spilling_into_dialogue_and_private_metadata_are_rejected(self):
        self.manifest['films'][0]['cues'][0]['end'] = 1.4
        self.save()
        with self.assertRaises(ValueError): validate_pack(self.root)
        self.manifest['films'][0]['cues'][0]['end'] = 3.
        self.manifest['reference'] = 'personal-reference.wav'
        self.save()
        with self.assertRaises(ValueError): validate_pack(self.root)

    def test_recording_needs_dispatch_room_within_its_reviewed_window(self):
        self.manifest['films'][0]['cues'][0]['end'] = 1.6
        self.save()
        with self.assertRaisesRegex(ValueError, 'dispatch'): validate_pack(self.root)
        self.manifest['films'][0]['cues'][0]['end'] = 1.65
        self.save()
        self.assertEqual(validate_pack(self.root)['clips'][0]['duration'], 1.)


if __name__ == '__main__': unittest.main()
