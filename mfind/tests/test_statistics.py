import pathlib, subprocess, tempfile, sys, json
binary=str(pathlib.Path(sys.argv[1]).resolve())
with tempfile.TemporaryDirectory() as directory:
    root=pathlib.Path(directory); (root/'data').mkdir(); (root/'keywords.txt').write_text('foo\nbar\nabsent\n')
    (root/'data'/'a').write_text('foo foo bar'); (root/'data'/'b').write_text('foo')
    def run(*args): return subprocess.run([binary,'data','--format','json','--stats',*args],cwd=root,text=True,capture_output=True)
    result=run('--require-all','foo','--require-all','bar'); assert result.returncode==0,result.stderr
    matches=json.loads(result.stdout)['matches']; stats=json.loads(result.stderr)['statistics']
    assert len(matches)==stats['occurrences']==3 and stats['matchingFiles']==1,stats
    assert stats['keywords']['foo']=={'occurrences':2,'files':1}
    assert stats['keywords']['absent']['occurrences']==0
    assert len(json.loads(run('--require-any','foo').stdout)['matches'])==4
    assert run('--require-all','unknown').returncode==2
    assert run('--require-all','absent').returncode==1
print('Boolean query and statistics tests passed')
