import pathlib, subprocess, tempfile, sys, json
binary=str(pathlib.Path(sys.argv[1]).resolve())
with tempfile.TemporaryDirectory() as directory:
    root=pathlib.Path(directory); (root/'data').mkdir(); (root/'keywords.txt').write_text('foo\n')
    (root/'data'/'space name.txt').write_bytes('😀foo\r\nαfoo\rfoo'.encode())
    args=[binary,'data','--format','lsp','--cache','.cache/index']
    for _ in range(2):
        r=subprocess.run(args,cwd=root,text=True,capture_output=True); assert r.returncode==0,r.stderr
        locations=json.loads(r.stdout)
        assert locations[0]['uri'].endswith('/space%20name.txt'),locations
        assert [m['range'] for m in locations]==[
            {'start':{'line':0,'character':2},'end':{'line':0,'character':5}},
            {'start':{'line':1,'character':1},'end':{'line':1,'character':4}},
            {'start':{'line':2,'character':0},'end':{'line':2,'character':3}}],locations
print('UTF-16 range and URI tests passed')
