prog_python = import('python').find_installation('python3')

texture_shaders = [
  'noeffect',
  'basic',
  'fancy'
]

primitive_shaders = [
  'rounded_corners_rect',
  'rounded_corners_border'
]

texture_shader_files = []
foreach s : texture_shaders
  texture_shader_files += [
    './texture/' + s + '/vertex.glsl',
    './texture/' + s + '/fragment_rgba.glsl',
    './texture/' + s + '/fragment_rgbx.glsl',
    './texture/' + s + '/fragment_ext.glsl',
    './texture/' + s + '/fragment_lock_rgba.glsl',
    './texture/' + s + '/fragment_lock_rgbx.glsl',
    './texture/' + s + '/fragment_lock_ext.glsl',
  ]
endforeach

primitive_shader_files = []
foreach s : primitive_shaders
  primitive_shader_files += [
    './primitive/' + s + '/vertex.glsl',
    './primitive/' + s + '/fragment.glsl',
  ]
endforeach

texture_shaders_c = custom_target(
    'texture_shaders.c',
    output : 'texture_shaders.c',
    input : ['generate_texture_shaders.py'] + texture_shader_files,
    command : [prog_python, '@INPUT0@', '@OUTPUT@']
)

primitive_shaders_c = custom_target(
    'primitive_shaders.c',
    output : 'primitive_shaders.c',
    input : ['generate_primitive_shaders.py'] + primitive_shader_files,
    command : [prog_python, '@INPUT0@', '@OUTPUT@']
)
