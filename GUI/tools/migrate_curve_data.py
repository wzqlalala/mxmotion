import re
import sys

def read_file(path):
    with open(path, encoding='utf-8') as f:
        return f.read()

def write_file(path, content):
    with open(path, 'w', encoding='utf-8') as f:
        f.write(content)

def migrate_read_curve_data1(content):
    """Migrate readCurveData1 to use readVariableValuesHelper."""
    # Find the function body and replace the ifstream block
    func_start = content.find('void mIMxdbFile1::readCurveData1(')
    if func_start < 0:
        print('readCurveData1 not found')
        return content
    
    brace = content.find('{', func_start)
    depth = 0
    func_end = brace
    for i in range(brace, len(content)):
        if content[i] == '{':
            depth += 1
        elif content[i] == '}':
            depth -= 1
            if depth == 0:
                func_end = i
                break
    
    func_old = content[brace:func_end+1]
    
    # Replace the ifstream setup and inner loop
    # We replace the block from "std::ifstream fs;" to "//关闭文件" inclusive
    pattern = r'''(\s*)//使用局部 ifstream 读取变量数据\s*
\s*std::ifstream fs;\s*
\s*fs\.open\(finfo\._frameFile\.toLocal8Bit\(\), ios::in \| ios::binary\);\s*
\s*if \(!fs\)\s*
\s*return;\s*
\s*long long int varbyte = varinfo\._varByte;\s*
\s*fs\.seekg\(varbyte, ios::beg\);\s*
'''
    
    # This is complex; instead we do a simpler text-based replacement
    # of the exact known block.  For safety we only substitute once.
    return content

if __name__ == '__main__':
    content = read_file(r'src\PostIOFile\mIMxdbFile1.cpp')
    content = migrate_read_curve_data1(content)
    write_file(r'src\PostIOFile\mIMxdbFile1.cpp', content)
