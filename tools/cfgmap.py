import re, sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from fnmap import funcmap
FILES = "swedate swehouse swejpl swemmoon swemplan sweph swephlib swecl swehel".split()
WRITERS = ('strcpy','strncpy','strcat','memcpy','memset','sprintf','sscanf')
def classify(code, f, m):
    rest = code[m.end():].lstrip()
    if rest.startswith('=') and not rest.startswith('=='): return 'W'
    if re.match(r'(\+\+|--|\+=|-=|\*=|/=)', rest): return 'W'
    pre = code[:m.start()]
    for w in WRITERS:
        j = pre.rfind(w+'(')
        if j >= 0:
            between = pre[j+len(w)+1:]
            if ',' not in between and between.strip().rstrip('&').strip('( )') in ('','void *','char *','double *'):
                return 'W'
    return 'R'
fields = sys.argv[1:]
FM = {b+'.c': funcmap(b+'.c') for b in FILES}
for f in fields:
    W, R = {}, {}
    for p, fm in FM.items():
        for i, ln in enumerate(open(p, errors='replace'), 1):
            code = ln.split('//')[0]
            for m in re.finditer(r'swed\.'+re.escape(f)+r'\b(\[[^\]]*\])?(\.\w+)?', code):
                d = W if classify(code, f, m)=='W' else R
                d.setdefault(fm.get(i,'?'), []).append(f"{p}:{i}")
    print(f"\nswed.{f}")
    print(f"  W: " + ("; ".join(f"{k}({len(v)})" for k,v in sorted(W.items())) or "-"))
    print(f"  R: " + ("; ".join(f"{k}" for k in sorted(R)) or "-"))
