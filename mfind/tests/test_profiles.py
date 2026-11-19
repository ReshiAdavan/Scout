import pathlib, subprocess, tempfile, sys, json
binary = str(pathlib.Path(sys.argv[1]).resolve())
with tempfile.TemporaryDirectory() as directory:
    root=pathlib.Path(directory); (root/'data').mkdir(); (root/'data'/'a.txt').write_text('OLD_API old_api')
    config='version 1\nprofile migration\nroot "data"\nignore-case true\nkeyword "legacy" "migration" "old_api"\nkeyword "audit" "security" "old_api"\n'
    (root/'scout.conf').write_text(config)
    def run(): return subprocess.run([binary,'--config','scout.conf','--profile','migration','--format','json'],cwd=root,text=True,capture_output=True)
    result=run(); assert result.returncode==0,result.stderr
    matches=json.loads(result.stdout)['matches']; assert len(matches)==4,matches
    assert {m['id'] for m in matches}=={'legacy','audit'}
    (root/'scout.conf').write_text(config+'keyword "legacy" "other" "foo"\n')
    assert run().returncode==2
print('Profile and identity tests passed')
