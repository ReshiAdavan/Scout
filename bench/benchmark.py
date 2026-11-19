#!/usr/bin/env python3
"""Deterministic correctness-checked end-to-end benchmark (POSIX, Python 3)."""
import argparse, json, os, pathlib, platform, select, signal, subprocess, tempfile, time
parser=argparse.ArgumentParser()
parser.add_argument('--binary',default='mfind/mfind')
parser.add_argument('--scale',type=int,default=1)
args=parser.parse_args()
if args.scale<1: parser.error('scale must be positive')
binary=str(pathlib.Path(args.binary).resolve())
report={'platform':platform.platform(),'python':platform.python_version(),'scale':args.scale,'binary':binary,'runs':[]}
report['compiler']=subprocess.run(['clang++','--version'],capture_output=True,text=True).stdout.splitlines()[0]
def measured(command,cwd):
    with tempfile.TemporaryFile() as out, tempfile.TemporaryFile() as err:
        started=time.perf_counter(); proc=subprocess.Popen(command,cwd=cwd,stdout=out,stderr=err)
        _,status,usage=os.wait4(proc.pid,0); proc.returncode=os.waitstatus_to_exitcode(status)
        elapsed=time.perf_counter()-started
        out.seek(0); err.seek(0); data=out.read(); errors=err.read()
        if proc.returncode not in (0,1): raise RuntimeError(errors.decode())
        rss=usage.ru_maxrss if platform.system()=='Darwin' else usage.ru_maxrss*1024
        return json.loads(data),json.loads(errors)['statistics'],elapsed,rss
with tempfile.TemporaryDirectory() as directory:
    root=pathlib.Path(directory)
    corpora={
        'small-files':{f'{i}.txt':('plain text foo\n'*10) for i in range(50*args.scale)},
        'large-file':{'large.txt':('x'*8189+'foo\n')*(32*args.scale)},
        'dense':{'dense.txt':'abababa\n'*(500*args.scale)},
        'many-patterns':{'patterns.txt':' '.join(f'pattern{i:03d}' for i in range(100))*args.scale},
    }
    for name,files in corpora.items():
        folder=root/name; folder.mkdir(); data=folder/'data'; data.mkdir()
        words=['foo','aba','bab'] if name!='many-patterns' else [f'pattern{i:03d}' for i in range(100)]
        (folder/'keywords.txt').write_text('\n'.join(words)+'\n')
        expected=[]; byte_count=0
        for file,text in files.items():
            (data/file).write_text(text); byte_count+=len(text.encode())
            for word in words:
                start=0
                while True:
                    pos=text.find(word,start)
                    if pos<0:break
                    expected.append(('data/'+file,pos,word)); start=pos+1
        expected.sort()
        for mode in ['all','longest-per-start','leftmost-longest']:
            candidates=sorted(expected,key=lambda m:(m[0],m[1],-len(m[2]),words.index(m[2])))
            selected=[]; previous_file=None; previous_start=-1; end=0
            for item in candidates:
                file,pos,word=item
                if mode=='all' or file!=previous_file or (pos!=previous_start if mode=='longest-per-start' else pos>=end):
                    selected.append(item); previous_file=file; previous_start=pos; end=pos+len(word)
            for temperature in ['cold','warm']:
                cache=folder/'.cache'/mode
                if temperature=='cold' and cache.exists():cache.unlink()
                command=[binary,'data','--format','json','--stats','--cache',str(cache),'--match-mode',mode]
                result,stats,elapsed,rss=measured(command,folder)
                actual=sorted((m['file'],m['byteOffset'],m['keyword']) for m in result['matches'])
                assert actual==sorted(selected),(name,mode,temperature)
                report['runs'].append({'corpus':name,'mode':mode,'cache':temperature,'seconds':elapsed,'bytes':byte_count,'bytesPerSecond':byte_count/elapsed,'peakRssBytes':rss,'matches':len(actual),'stats':stats})
    watch=root/'watch';watch.mkdir();(watch/'data').mkdir();(watch/'keywords.txt').write_text('foo\n')
    target=watch/'data'/'a';target.write_text('foo')
    proc=subprocess.Popen([binary,'data','--watch','--poll-ms','50','--format','jsonl'],cwd=watch,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
    buffer=b''
    def next_event():
        global buffer
        deadline=time.monotonic()+10
        while b'\n' not in buffer:
            if time.monotonic()>deadline:raise RuntimeError('watch timed out')
            if select.select([proc.stdout],[],[],.1)[0]:
                chunk=os.read(proc.stdout.fileno(),65536)
                if not chunk:raise RuntimeError('watch exited')
                buffer+=chunk
        line,buffer=buffer.split(b'\n',1);return json.loads(line)
    try:
        assert next_event()['event']=='added';latencies=[]
        for i in range(6):
            start=time.perf_counter()
            if i%2==0:target.unlink();expected_event='removed'
            else:target.write_text('foo');expected_event='added'
            assert next_event()['event']==expected_event
            latencies.append((time.perf_counter()-start)*1000)
        report['watch']={'pollMs':50,'changeLatencyMs':latencies}
    finally:
        proc.send_signal(signal.SIGINT);proc.wait(timeout=5)
print(json.dumps(report,indent=2))
