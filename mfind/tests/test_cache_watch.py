import pathlib, subprocess, tempfile, sys, json, time, signal
binary=str(pathlib.Path(sys.argv[1]).resolve())
with tempfile.TemporaryDirectory() as directory:
    root=pathlib.Path(directory); (root/'data').mkdir(); (root/'keywords.txt').write_text('foo\n')
    target=root/'data'/'a'; target.write_text('foo')
    args=[binary,'data','--format','json','--stats','--cache','.cache/index']
    def run():
        r=subprocess.run(args,cwd=root,text=True,capture_output=True); assert r.returncode==0,r.stderr
        return json.loads(r.stdout),json.loads(r.stderr)['statistics']
    cold,cs=run(); warm,ws=run(); assert cold==warm and cs['scannedFiles']==1 and ws['cacheHits']==1,(cs,ws)
    (root/'.cache'/'index').write_bytes(b'SCOUT2 truncated')
    assert run()[1]['scannedFiles']==1
    with (root/'events').open('w') as out:
        proc=subprocess.Popen([binary,'data','--watch','--poll-ms','50','--format','jsonl'],cwd=root,stdout=out,stderr=subprocess.PIPE,text=True)
        def wait_events(n):
            deadline=time.monotonic()+5
            while time.monotonic()<deadline:
                lines=(root/'events').read_text().splitlines()
                if len(lines)>=n:return [json.loads(x) for x in lines]
                time.sleep(.05)
            raise AssertionError('Timed out waiting for watch events')
        try:
            assert wait_events(1)[0]['event']=='added'
            time.sleep(.2); assert len((root/'events').read_text().splitlines())==1
            target.unlink(); assert wait_events(2)[1]['event']=='removed'
            proc.send_signal(signal.SIGINT); assert proc.wait(timeout=3)==0
        finally:
            if proc.poll() is None: proc.kill(); proc.wait()
print('Persistent cache and watch-delta tests passed')
