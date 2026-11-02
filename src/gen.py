#!/usr/bin/env python3
"""Generates the forwarding thunks, header and .def file from exports.txt."""
import os
here = os.path.dirname(os.path.abspath(__file__))
out = os.path.join(here, '..', 'build')
os.makedirs(out, exist_ok=True)
names = open(os.path.join(here, 'exports.txt')).read().split()
fw = [n for n in names if n != 'GetAdaptersInfo']

asm = ['.data', '.globl real_ptrs', '.balign 8', 'real_ptrs:'] + ['  .quad 0'] * len(fw) + ['.text']
for i, n in enumerate(fw):
    asm += [f'.globl thunk_{i}', f'thunk_{i}:', f'  jmp *real_ptrs+{8*i}(%rip)']
open(os.path.join(out, 'thunks.S'), 'w').write('\n'.join(asm) + '\n')

open(os.path.join(out, 'names.h'), 'w').write(
    'static const char *fw_names[] = {\n' + ',\n'.join(f'  "{n}"' for n in fw) +
    f'\n}};\n#define FW_COUNT {len(fw)}\n')

d = ['LIBRARY iphlpapi.dll', 'EXPORTS'] + [f'  {n}=thunk_{i}' for i, n in enumerate(fw)]
d.append('  GetAdaptersInfo=my_GetAdaptersInfo')
open(os.path.join(out, 'iphlpapi.def'), 'w').write('\n'.join(d) + '\n')
