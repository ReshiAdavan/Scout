import pathlib, subprocess, tempfile, sys, json, csv, io
binary = str(pathlib.Path(sys.argv[1]).resolve())
with tempfile.TemporaryDirectory() as directory:
    root = pathlib.Path(directory); (root / 'data').mkdir()
    (root / 'keywords.txt').write_text('foo"bar\n')
    (root / 'data' / 'a,b.txt').write_bytes('α\r\nfoo"bar\n'.encode())
    def run(fmt):
        result = subprocess.run([binary, 'data', '--format', fmt], cwd=root, capture_output=True, text=True, timeout=15)
        assert result.returncode == 0, result.stderr
        return result.stdout
    m = json.loads(run('json'))['matches'][0]
    assert (m['byteOffset'], m['line'], m['column'], m['text']) == (4, 2, 1, 'foo"bar'), m
    row = list(csv.DictReader(io.StringIO(run('csv'))))[0]
    assert row['text'] == 'foo"bar' and row['file'] == 'data/a,b.txt', row
    assert json.loads(run('jsonl')) == m
    assert run('editor').startswith('data/a,b.txt:2:1:')
print('Structured output tests passed')
