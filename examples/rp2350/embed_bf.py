#!/usr/bin/env python3
import pathlib
import sys

TOKENS = set('><+-.,[]')


def main() -> int:
    if len(sys.argv) != 3:
        print('usage: embed_bf.py <input.b> <output.h>', file=sys.stderr)
        return 1

    src = pathlib.Path(sys.argv[1])
    out = pathlib.Path(sys.argv[2])

    code = src.read_text(encoding='utf-8', errors='ignore')
    filtered = ''.join(ch for ch in code if ch in TOKENS)

    out.parent.mkdir(parents=True, exist_ok=True)
    with out.open('w', encoding='ascii') as f:
        f.write('#ifndef BF_PROGRAM_GENERATED_H\n')
        f.write('#define BF_PROGRAM_GENERATED_H\n\n')
        f.write('static const char g_bf_program[] =\n')

        chunk = 64
        for i in range(0, len(filtered), chunk):
            part = filtered[i:i + chunk]
            escaped = part.replace('\\', '\\\\').replace('"', '\\"')
            f.write(f'    "{escaped}"\n')

        f.write('    ;\n\n')
        f.write('static const unsigned int g_bf_program_len = sizeof(g_bf_program) - 1;\n\n')
        f.write('#endif // BF_PROGRAM_GENERATED_H\n')

    return 0


if __name__ == '__main__':
    raise SystemExit(main())
