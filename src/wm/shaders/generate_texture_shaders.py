import os
import sys

base = os.path.dirname(os.path.realpath(__file__))
base_textures = os.path.join(base, 'texture')

texture_files = [
    'vertex.glsl',
    'fragment_rgba.glsl',
    'fragment_rgbx.glsl',
    'fragment_ext.glsl',
    'fragment_lock_rgba.glsl',
    'fragment_lock_rgbx.glsl',
    'fragment_lock_ext.glsl',
]

with open(sys.argv[1], "w") as out:
    out.write("""
#define _POSIX_C_SOURCE 200809L
#include "wm/wm_renderer.h"

void wm_texture_shaders_init(struct wm_renderer* renderer){

    """)

    skip = True
    for subdir, _, files in os.walk(base_textures):
        if skip:
            skip = False
            continue

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
        
        out.write(f"""
    wm_renderer_add_texture_shaders(renderer, "{os.path.split(subdir)[1]}", {",".join(strs)});
        """)

    out.write("}");

