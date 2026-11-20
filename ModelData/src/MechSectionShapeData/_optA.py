import io, re, glob, os

BASE = r'D:\MxSim.Rebuild.Dev\Basic\RebuildModelData\src\SectionShapeData'
occ_inc = re.compile(r'^(\s*)(#include\s*[<"].*(?:\.hxx|AIS_|TopoDS_|Standard_|Graphic3d_|Prs3d_)[>"].*)$')
occ_member = re.compile(r'^(\s*)((?:Handle\(|opencascade::handle<|TopoDS_\w+|AIS_\w+)\s*\w+.*;)\s*$')

changed = []
for f in sorted(glob.glob(os.path.join(BASE, '*.h')) + glob.glob(os.path.join(BASE, '*.cpp'))):
    s = io.open(f, encoding='utf-8').read()
    orig = s
    lines = s.splitlines(keepends=True)
    out = []
    for ln in lines:
        stripped = ln.lstrip()
        if stripped.startswith('//'):
            out.append(ln); continue
        m1 = occ_inc.match(ln.rstrip('\r\n'))
        if m1:
            out.append(m1.group(1) + '//' + m1.group(2) + '\n')
            continue
        # 成员声明（.h 文件中非函数体内）
        if f.endswith('.h'):
            m2 = occ_member.match(ln.rstrip('\r\n'))
            if m2 and '(' not in m2.group(2).split('=')[0]:  # 排除函数
                out.append(m2.group(1) + '//' + m2.group(2) + ' //【OCC迁移·Option A】休眠显示链成员\n')
                continue
        out.append(ln)
    s2 = ''.join(out)
    if s2 != orig:
        io.open(f, 'w', encoding='utf-8', newline='').write(s2)
        changed.append(os.path.basename(f))

print('changed %d files:' % len(changed))
for c in changed[:20]: print(' ', c)
