glslc -fshader-stage=vert shader/basic_vert.glsl -o shader/basic_vert.spv
glslc -fshader-stage=frag shader/basic_frag.glsl -o shader/basic_frag.spv
glslc -fshader-stage=vert shader/colored_vert.glsl -o shader/colored_vert.spv
glslc -fshader-stage=frag shader/colored_frag.glsl -o shader/colored_frag.spv
glslc -fshader-stage=frag shader/standard_frag.glsl -o shader/standard_frag.spv
glslc -fshader-stage=vert shader/standard_vert.glsl -o shader/standard_vert.spv