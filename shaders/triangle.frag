// shaders/triangle.frag 
#version 450 
layout(location = 0) in  vec3 fragColor; 
layout(location = 0) out vec4 outColor;  // pas gl_FragColor en Vulkan 
void main() { outColor = vec4(fragColor, 1.0); } 
 
// Compilation via Jenga prebuild : 
// glslc shaders/triangle.vert -o shaders/triangle.vert.spv 
// glslc shaders/triangle.frag -o shaders/triangle.frag.spv 