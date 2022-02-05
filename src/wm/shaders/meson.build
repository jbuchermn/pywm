prog_python = import('python').find_installation('python3')

texture_shaders = [
  'noeffect',
  'basic',
  'fancy'
]

primitive_shaders = [
  'rect',
  'rounded_corners_rect',
  'rounded_corners_border',
  'corner'
]

quad_shader_files = [
  'quad/vertex.glsl',
  'quad/fragment.glsl',
  'quad/fragment_downsample.glsl',
  'quad/fragment_upsample.glsl',
]

texture_shader_files = []
foreach s : texture_shaders
  texture_shader_files += [
    './texture/' + s + '/vertex.glsl',
    './texture/' + s + '/fragment_rgba.glsl',
    './texture/' + s + '/fragment_rgbx.glsl',
    './texture/' + s + '/fragment_ext.glsl',
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

quad_shaders_c = custom_target(
    'quad_shaders.c',
    output : 'quad_shaders.c',
    input : ['generate_quad_shaders.py'] + quad_shader_files,
    command : [prog_python, '@INPUT0@', '@OUTPUT@']
)
