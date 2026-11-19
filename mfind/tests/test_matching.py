import pathlib, subprocess, tempfile, sys, json
binary = str(pathlib.Path(sys.argv[1]).resolve())
with tempfile.TemporaryDirectory() as directory:
    root = pathlib.Path(directory); (root/'data').mkdir(); (root/'keywords.txt').write_text('foo\n')
    text = 'x'*8191 + ' FOO foo_bar foo1 (fOo) foo'
    (root/'data'/'a.txt').write_text(text)
    result = subprocess.run([binary,'data','--ignore-case','--whole-word','--format','json'],cwd=root,text=True,capture_output=True)
    assert result.returncode == 0, result.stderr
    matches = json.loads(result.stdout)['matches']
    assert [m['text'] for m in matches] == ['FOO','fOo','foo'], matches
    assert [m['byteOffset'] for m in matches] == [8192,8210,8215], matches
print('Case and word-boundary tests passed')
