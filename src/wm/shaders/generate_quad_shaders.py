import os
import sys

base = os.path.join(os.path.dirname(os.path.realpath(__file__)), 'quad')

files = [
    'vertex.glsl',
    'fragment.glsl',
    'fragment_downsample.glsl',
    'fragment_upsample.glsl',
]

with open(sys.argv[1], "w") as out:
    out.write("#define _POSIX_C_SOURCE 200809L\n")


    for f in files:
        print("Generating %s" % f)
        src = ""
        with open(os.path.join(base, f), 'r') as inp:
            src = "\"" + inp.read().replace("\n", "\\n\"\n\"") + "\""
        out.write("static const char %s[] = %s;\n" % ("quad_" + f.replace('.glsl', '_src'), src))

