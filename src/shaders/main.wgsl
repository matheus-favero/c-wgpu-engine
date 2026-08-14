struct uniformsStruct{
    color: vec4f,
    translation: mat4x4f
};

@group(0) @binding(0) var<uniform> uniforms: uniformsStruct;

@vertex fn vs(
    @location(0) vertex: vec3f
) -> @builtin(position) vec4f {
    let final_matrix = uniforms.translation * vec4f(vertex, 1);
    
    return final_matrix;
}

@fragment fn fs() -> @location(0) vec4f {
    return uniforms.color;
};