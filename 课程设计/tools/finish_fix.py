"""Build each isolated fix before recording status and committing scoped paths."""
import argparse
from pathlib import Path
import subprocess

p = argparse.ArgumentParser()
p.add_argument('bug')
p.add_argument('summary')
p.add_argument('files', nargs='+')
a = p.parse_args()
subprocess.run(['D:/mingw64/bin/cmake.exe', '--build', 'build-verify', '--target', 'gobang', '--parallel', '8'], check=True)
record = Path('BUGS.md')
lines = record.read_text(encoding='utf-8').splitlines()
for i, line in enumerate(lines):
    if line.startswith('| ' + a.bug + ' |'):
        lines[i] = f'| {a.bug} | 已修复 | {a.summary}；Debug 编译通过，专项验证见下文。 |'
record.write_text('\n'.join(lines)+'\n',encoding='utf-8')
paths = a.files + ['BUGS.md']
subprocess.run(['git','add','--',*paths],check=True)
subprocess.run(['git','commit','-m',f'fix({a.bug}): {a.summary}','--',*paths],check=True)
