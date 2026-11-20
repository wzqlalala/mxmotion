import re

def main():
    with open(r'src\PostIOFile\mIMxdbFile1.cpp', encoding='utf-8') as f:
        txt = f.read()

    # Find all functions and their bodies
    for m in re.finditer(r'^(?:\w+::)?(?:bool|void|int|QVector<[^>]+>|QHash<[^>]+>)\s+mIMxdbFile1::(\w+)\s*\(', txt, re.MULTILINE):
        func_name = m.group(1)
        func_start = m.start()
        func_line = txt[:func_start].count('\n') + 1

        brace = txt.find('{', func_start)
        depth = 0
        end = brace
        for i in range(brace, len(txt)):
            if txt[i] == '{':
                depth += 1
            elif txt[i] == '}':
                depth -= 1
                if depth == 0:
                    end = i
                    break

        func_txt = txt[brace:end+1]
        has_ifstream = 'std::ifstream' in func_txt
        has_fs_read = 'fs.read' in func_txt
        has_fs_seekg = 'fs.seekg' in func_txt

        if has_ifstream or has_fs_read or has_fs_seekg:
            print(f'{func_line}: {func_name}')
            if has_ifstream:
                print('  has std::ifstream: true')
            if has_fs_read:
                print('  has fs.read: true')
            if has_fs_seekg:
                print('  has fs.seekg: true')

            if 'loopcount' in func_txt:
                if 'eleId' in func_txt or 'comVarCount' in func_txt or 'cellType' in func_txt:
                    print('  pattern: per-element multi-node')
                else:
                    print('  pattern: per-entity scalar/vector')

            for ln in func_txt.splitlines():
                if 'std::ifstream' in ln or 'fs.open' in ln or 'fs.seekg' in ln or 'fs.read' in ln:
                    print('  ', ln.strip()[:120])
            print()

if __name__ == '__main__':
    main()
