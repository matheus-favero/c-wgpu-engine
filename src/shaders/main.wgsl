struct Uniforms{
    color: vec4f,
    translation: mat4x4f
};

struct Vertex_Attribs{
    @location(0) position: vec3f,
    @location(1) color: vec4f
}

struct Output{
    @builtin(position) position: vec4f,
    @location(0) color: vec4f
}

@group(0) @binding(0) var<uniform> uniforms: Uniforms;

@vertex fn vs(
    @builtin(vertex_index) vertex_index: u32,
    vertex: Vertex_Attribs
) -> Output {
    var output: Output;
    output.position = uniforms.translation * vec4f(vertex.position, 1);
    output.color = vertex.color;
    
    return output;
}

@fragment fn fs(Input: Vertex_Attribs) -> @location(0) vec4f {
    return Input.color;
}