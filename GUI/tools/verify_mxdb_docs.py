import os
import re

base = os.path.abspath('doc/后处理文件格式')
folders = ['通用','显式','电机']
files = [f'v{i:02d}.{ext}' for i in range(1,6) for ext in ('mxdb0','mxdb')]

errors = []

for folder in folders:
    p = os.path.join(base, folder)
    for f in files:
        fp = os.path.join(p, f)
        if not os.path.isfile(fp):
            errors.append(f'{folder}/{f}: file missing')
            continue
        with open(fp, 'r', encoding='utf-8') as fh:
            text = fh.read()
        # version and end tag must match filename
        m_version = re.search(r'0xA1B1C1(0[1-5])', text)
        m_endtag = re.search(r'0xA1B1C1D([1-5])', text)
        ver = f[1:3]  # '01' etc
        ver_cmp = ver[1] if ver.startswith('0') else ver
        if m_version and m_version.group(1) != ver:
            errors.append(f'{folder}/{f}: version 0xA1B1C1{m_version.group(1)} != v{ver}')
        if m_endtag and m_endtag.group(1) != ver_cmp:
            errors.append(f'{folder}/{f}: endTag 0xA1B1C1D{m_endtag.group(1)} != v{ver}')
        # precision check
        has_float = '[float' in text or '[float]' in text
        has_double = '[double' in text or '[double]' in text
        if folder == '显式':
            if not has_float:
                errors.append(f'{folder}/{f}: expected float precision')
            if '[double[3]]\tnodeCoord' in text or '[double]\t\ttimeOrFreqValue' in text:
                errors.append(f'{folder}/{f}: nodeCoord/time should be float in 显式')
        else:
            if has_float:
                errors.append(f'{folder}/{f}: unexpected float precision (should be double)')

if errors:
    print('ERRORS:')
    for e in errors:
        print(e)
    raise SystemExit(1)
else:
    print('ALL CHECKS PASSED')
