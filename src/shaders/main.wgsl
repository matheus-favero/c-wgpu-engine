struct uniformsStruct{
    color: vec4f,
    translation: mat4x4f
};

// struct Vertex{
//     @location(0) position: vec2f
// };

@group(0) @binding(0) var<uniform> uniforms: uniformsStruct;

@vertex fn vs(
    @location(0) vertex: vec2f,
    @builtin(instance_index) instanceIndex: u32
) -> @builtin(position) vec4f {
    // let pos = array(
    //     vec2f(0.0, 0.5),
    //     vec2f(-0.5, -0.5),
    //     vec2f(0.5, -0.5)
    // );
    let final_matrix = uniforms.translation * vec4f(vertex, 0, 1);
    return final_matrix;
}
                         
@fragment fn fs() -> @location(0) vec4f {
    return uniforms.color;
};