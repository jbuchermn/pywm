prog_python = import('python').find_installation('python3')

texture_shaders = [
  'noeffect',
  'basic',
  'fancy'
]

primitive_shaders = [
  'rounded_corners_rect'
]

texture_shader_files = []
foreach s : texture_shaders
  texture_shader_files += [
    './texture/' + s + '/vertex.c',
    './texture/' + s + '/fragment_rgba.c',
    './texture/' + s + '/fragment_rgbx.c',
    './texture/' + s + '/fragment_ext.c',
    './texture/' + s + '/fragment_lock_rgba.c',
    './texture/' + s + '/fragment_lock_rgbx.c',
    './texture/' + s + '/fragment_lock_ext.c',
  ]
endforeach

primitive_shader_files = []
foreach s : primitive_shaders
  primitive_shader_files += [
    './primitive/' + s + '/vertex.c',
    './primitive/' + s + '/fragment.c',
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
