import pathlib, subprocess, tempfile, sys, time, signal, json
binary=str(pathlib.Path(sys.argv[1]).resolve())
with tempfile.TemporaryDirectory() as directory:
    root=pathlib.Path(directory); (root/'data').mkdir(); (root/'keywords.txt').write_text('foo\n')
    target=root/'data'/'a'; target.write_text('foo')
    with (root/'events').open('w') as out, (root/'errors').open('w') as err:
        proc=subprocess.Popen([binary,'data','--watch','--poll-ms','1','--format','jsonl'],cwd=root,stdout=out,stderr=err)
        try:
            deadline=time.monotonic()+3
            while time.monotonic()<deadline and not (root/'events').read_text():time.sleep(.01)
            assert proc.poll() is None
            for i in range(100):
                target.unlink(missing_ok=True)
                time.sleep(.001)
                target.write_text('foo' + 'x'*(i%10))
                time.sleep(.001)
            target.unlink(missing_ok=True)
            deadline=time.monotonic()+5
            while time.monotonic()<deadline:
                assert proc.poll() is None,(root/'errors').read_text()
                events=[json.loads(x) for x in (root/'events').read_text().splitlines()]
                if events and events[-1]['event']=='removed':break
                time.sleep(.01)
            else:raise AssertionError('No final removal after churn')
            proc.send_signal(signal.SIGINT);assert proc.wait(timeout=3)==0
        finally:
            if proc.poll() is None:proc.kill();proc.wait()
print('Watch survives repeated create/delete/write races')
