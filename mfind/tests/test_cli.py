import pathlib, subprocess, tempfile, sys
binary = str(pathlib.Path(sys.argv[1]).resolve())
with tempfile.TemporaryDirectory() as directory:
    root = pathlib.Path(directory)
    (root / 'data').mkdir()
    (root / 'keywords.txt').write_text('foo\n')
    (root / 'data' / 'a.txt').write_text('foo')
    def run(*args):
        return subprocess.run([binary, *args], cwd=root, text=True, capture_output=True, timeout=10)
    result = run('data')
    assert result.returncode == 0 and 'foo' in result.stdout and '\x1b' not in result.stdout, result
    assert run('data', '--extension', 'cpp').returncode == 1
    assert run('data', '--threads', '0').returncode == 2
    assert run('data', '--bogus').returncode == 2
    assert run('data', '--keywords', 'missing').returncode == 2
    assert run('--help').returncode == 0
    assert run('--version').returncode == 0
print('CLI contract tests passed')
