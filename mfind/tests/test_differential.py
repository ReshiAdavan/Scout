import pathlib, subprocess, tempfile, sys, json, random
binary=str(pathlib.Path(sys.argv[1]).resolve()); rng=random.Random(9281)
with tempfile.TemporaryDirectory() as directory:
    root=pathlib.Path(directory);(root/'data').mkdir()
    for case in range(4):
        words=['a','Ab','ba','aba','_a'];text=''.join(rng.choice('abAB_ .\n') for _ in range(600))
        if case==3:text='x'*8191+text+'aba'
        (root/'keywords.txt').write_text('\n'.join(words)+'\n');(root/'data'/'a').write_text(text)
        for insensitive in (False,True):
            for whole in (False,True):
                candidates=[];haystack=text.lower() if insensitive else text
                for index,word in enumerate(words):
                    needle=word.lower() if insensitive else word;start=0
                    while True:
                        pos=haystack.find(needle,start)
                        if pos<0:break
                        end=pos+len(word);start=pos+1
                        wordchar=lambda c:c.isascii() and (c.isalnum() or c=='_')
                        if whole and ((pos and wordchar(text[pos-1])) or (end<len(text) and wordchar(text[end]))):continue
                        candidates.append((pos,word,index))
                for mode in ('all','longest-per-start','leftmost-longest'):
                    expected=[];last=-1;end=0
                    for pos,word,index in sorted(candidates,key=lambda m:(m[0],-len(m[1]),m[2])):
                        if mode=='all' or (pos!=last if mode=='longest-per-start' else pos>=end):
                            expected.append((pos,word));last=pos;end=pos+len(word)
                    args=[binary,'data','--format','json','--match-mode',mode,'--cache','.cache/index']
                    if insensitive:args.append('--ignore-case')
                    if whole:args.append('--whole-word')
                    result=subprocess.run(args,cwd=root,text=True,capture_output=True,timeout=15)
                    assert result.returncode in (0,1),result.stderr
                    actual=[(m['byteOffset'],m['keyword']) for m in json.loads(result.stdout)['matches']]
                    assert sorted(actual)==sorted(expected),(case,insensitive,whole,mode)
    (root/'keywords.txt').write_text('a\n');(root/'data'/'a').write_text('a'*6000)
    args=[binary,'data','--format','json','--stats','--index-mb','1','--cache','.cache/index']
    for _ in range(2):
        result=subprocess.run(args,cwd=root,text=True,capture_output=True,timeout=15)
        assert result.returncode==0,result.stderr
        assert len(json.loads(result.stdout)['matches'])==6000
        stats=json.loads(result.stderr)['statistics'];assert stats['retainedIndexBytes']<=1048576 and stats['evictedFiles']==1,stats
print('48 differential matching cases and bounded-retention checks passed')
