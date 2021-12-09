import re
import os
import sys

base = os.path.dirname(os.path.realpath(__file__))
base_textures = os.path.join(base, 'primitive')

texture_files = [
    'vertex.glsl',
    'fragment.glsl',
]

regexp_int = re.compile(r'.*int params_int\[(\d*)\]');
regexp_float = re.compile(r'.*float params_float\[(\d*)\]');

with open(sys.argv[1], "w") as out:
    out.write("""
#define _POSIX_C_SOURCE 200809L
#include "wm/wm_renderer.h"

void wm_primitive_shaders_init(struct wm_renderer* renderer){

    """)

    skip = True
    for subdir, _, files in os.walk(base_textures):
        if skip:
            skip = False
            continue

        n_params_int = 0
        n_params_float = 0
        strs = []
        successful = True
        for f in texture_files:
            if f not in files:
                print("Could not find %s shader in %s" % (f, subdir))
                successful = False
                continue

            with open(os.path.join(subdir, f), 'r') as file:
                strs += ["\"" + file.read().replace("\n", "\\n\"\n\"") + "\""]

        if not successful:
            continue

        match_int = regexp_int.match("".join(strs).replace("\"", "").replace("\n", ""))
        if match_int is not None:
            n_params_int = int(match_int.groups()[0])

        match_float = regexp_float.match("".join(strs).replace("\"", "").replace("\n", ""))
        if match_float is not None:
            n_params_float = int(match_float.groups()[0])

        print("[DEBUG] Texture has %d, %d parameters" % (n_params_int, n_params_float))
        
        out.write(f"""
    wm_renderer_add_primitive_shader(renderer, "{os.path.split(subdir)[1]}", {",".join(strs)}, {n_params_int}, {n_params_float});
        """)

    out.write("}");

