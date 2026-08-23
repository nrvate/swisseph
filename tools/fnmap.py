import re
KEYWORDS = {'if','for','while','switch','return','sizeof','do','else','case'}
NAME = re.compile(r'([A-Za-z_]\w*)\s*$')
def funcmap(path):
    """line -> enclosing top-level function. This codebase puts every
    definition at column 0 and closes it with '}' at column 0, so track that
    rather than counting braces (which #if/#else blocks corrupt)."""
    out, cur, pending = {}, "<file>", None
    for i, ln in enumerate(open(path, errors='replace'), 1):
        s = ln.rstrip('\n')
        if s.startswith('}'):
            out[i] = cur; cur = "<file>"; pending = None; continue
        if s[:1] == '{' and pending:
            cur, pending = pending, None
        elif s and s[0] not in ' \t#/*':
            head = s.split('(')[0]
            m = NAME.search(head)
            if m and '(' in s and m.group(1) not in KEYWORDS \
               and not s.rstrip().endswith(';') and '=' not in head:
                if s.rstrip().endswith('{'):
                    cur, pending = m.group(1), None
                else:
                    pending = m.group(1)
        out[i] = cur
    return out
if __name__ == '__main__':
    import sys
    fm = funcmap(sys.argv[1])
    for ln in sys.argv[2:]:
        print(f"  {sys.argv[1]}:{ln:<6} -> {fm.get(int(ln),'?')}")
