"""Ensure the compiled phrase tables cover every mod-generated narration key."""
from collections import Counter
import json
from pathlib import Path
import re

SOURCE = Path(__file__).resolve().parents[1] / "src"
LITERAL = r'L("(?:[^"\\]|\\.)*")'


def literals(text):
    return {json.loads(value) for value in re.findall(LITERAL, text)}


def table_literals(filename, pattern):
    source = (SOURCE / filename).read_text(encoding="utf-8")
    match = re.search(pattern, source, re.DOTALL)
    assert match, f"Translation key table moved in {filename}"
    return literals(match.group(1))


def message_keys():
    result = set()
    for path in (*SOURCE.glob("*.cpp"), *SOURCE.glob("*.hpp")):
        source = path.read_text(encoding="utf-8")
        result.update(json.loads(key) for key in re.findall(r"\btr\(\s*" + LITERAL, source))
    online = (SOURCE / "online_menus.cpp").read_text(encoding="utf-8")
    result.update(json.loads(key) for key in re.findall(r"\bcaptioned\([^,]+,\s*" + LITERAL, online))
    result.update(table_literals("story_commands.cpp", r"labels\[22\]\{([^}]+)\}"))
    commands = (SOURCE / "story_commands.cpp").read_text(encoding="utf-8")
    scroll = re.findall(r'\{L"Image_More(?:UP|Bottom)",\s*' + LITERAL + r"\}", commands)
    assert len(scroll) == 2
    result.update(map(json.loads, scroll))
    result.update(table_literals("fight_announcements.cpp", r"labels\[\]\{([^}]+)\}"))
    return result


def placeholders(text):
    return Counter(re.findall(r"\{[0-9]\}", text))


expected = message_keys()
names = {"ar", "de", "es", "es-419", "fr", "it", "ja", "ko", "pl", "pt-BR", "ru", "th", "zh-Hans", "zh-Hant"}
files = sorted((SOURCE / "locale").glob("*.inc"))
assert {file.stem for file in files} == names
row = re.compile(r"\{" + LITERAL + r", " + LITERAL + r"\},")
for file in files:
    keys = []
    for line in file.read_text(encoding="utf-8").splitlines():
        match = row.fullmatch(line)
        assert match, f"{file.name}: malformed translation row"
        key, value = map(json.loads, match.groups())
        assert value.strip() and all(c >= " " and c != "\ufffd" for c in value), (file.name, key)
        assert placeholders(key) == placeholders(value), (file.name, key, value)
        keys.append(key)
    assert keys == sorted(expected), (file.name, expected - set(keys), set(keys) - expected)
print(f"{len(files)} catalogs, {len(expected)} messages each")
