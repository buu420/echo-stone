import copy
import importlib.util
import json
import tempfile
import unittest
from pathlib import Path

spec = importlib.util.spec_from_file_location('render_story', Path(__file__).resolve().parents[1] / 'tools/audio_descriptions/render_story.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class RenderInputTests(unittest.TestCase):
    def setUp(self):
        self.request = dict(schema=1, model='s2.1-pro-free', clips=[dict(id='opening-001', text='A hooded woman.')],
                            films=[dict(movie='0-A_DEMO01', sourceSHA256='a'*64, sourceBytes=400,
                                        duration=90., sequenceOffset=7.25, fps=30, nativeFrames=2700,
                                        cues=[dict(id='opening-001', start=.5, end=3.)])])

    def validate(self, data):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / 'manifest.json').write_text(json.dumps(data))
            return module.validate_request(root)

    def test_reviewed_request_is_accepted(self):
        self.assertEqual(self.validate(self.request)['clips'][0]['text'], 'A hooded woman.')

    def test_private_fields_paid_model_and_unassigned_clips_are_rejected(self):
        for field, value in [('model', 's2.1-pro'), ('references', ['private.wav'])]:
            data = copy.deepcopy(self.request)
            data[field] = value
            with self.assertRaises(ValueError): self.validate(data)
        data = copy.deepcopy(self.request)
        data['films'][0]['cues'] = []
        with self.assertRaises(ValueError): self.validate(data)

    def test_nan_overlap_and_outside_source_are_rejected(self):
        for start, end in [(float('nan'), 3), (2, 1), (-1, 3), (89, 91)]:
            data = copy.deepcopy(self.request)
            data['films'][0]['cues'][0].update(start=start, end=end)
            with self.assertRaises(ValueError): self.validate(data)
        data = copy.deepcopy(self.request)
        data['films'][0]['cues'] *= 2
        with self.assertRaises(ValueError): self.validate(data)


if __name__ == '__main__': unittest.main()
