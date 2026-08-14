
#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_gamepad.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_oldnames.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_scancode.h>
#include <SDL3/SDL_video.h>
#include <math.h>
#include <sdl3webgpu/sdl3webgpu.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <transformations/transformations.h>
#include <webgpu/webgpu.h>

#define WINDOW_W 720.0
#define WINDOW_H 720.0

#define MATRIX_SIZE 16

struct WGPU_Components {
  WGPUInstance instance;
  WGPUDevice device;
  WGPUSurface surface;
  WGPUShaderModule shader_module;
  WGPURenderPipeline render_pipeline;
  WGPUBindGroup bind_group;
  WGPUQueue queue;
  WGPUVertexState vertex_state;
};

struct SDL_Components {
  SDL_Window *window;
  SDL_Gamepad *gamepad;
};

struct Uniform_Values {
  float color[4];
  float transform[16];
};

struct Player_Info {
  float vertex_position;
};

static struct Uniform_Values uniform_default_values = {{0.1, 0, 1.0, 1.0},
                                                       {
                                                           1,
                                                           0,
                                                           0,
                                                           0,
                                                           0,
                                                           1,
                                                           0,
                                                           0,
                                                           0,
                                                           0,
                                                           1,
                                                           0,
                                                           0,
                                                           0,
                                                           0,
                                                           1,
                                                       }};
float vertex_buffer_values[] = {0.0, 0.5, 0, -0.5, -0.5, 0, 0.5, -0.5, 0};
static struct WGPU_Components wgpu_components = {NULL};
static WGPUBuffer uniform_buffer = {NULL};
static WGPUBuffer vertex_buffer = {NULL};

static struct SDL_Components sdl_components = {NULL};

bool initiate();
void main_loop();
void terminate();
char *load_shader();
void adapter_callback(WGPURequestAdapterStatus status, WGPUAdapter adapter,
                      WGPUStringView message, void *userdata1, void *userdata2);
void device_callback(WGPURequestDeviceStatus status, WGPUDevice device,
                     WGPUStringView message, void *userdata1, void *userdata2);
void update_player_transformation(float *directions, float *rotations,
                                  float *scalings);

void print_matrix(float *matrix) {
  for (int i = 1; i <= MATRIX_SIZE; i++) {
    if (i % (int)sqrt(MATRIX_SIZE) == 0) {
      printf("x:%f, y:%f, z:%f\n", matrix[i - 4], matrix[i - 3], matrix[i - 2]);
    }
  }
  puts("");
}

int main() {

  if (!initiate())
    return -1;

  main_loop();

  terminate();

  return 0;
}

bool initiate() {
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    puts("Failed to init SDL!");
    return false;
  }

  sdl_components.window =
      SDL_CreateWindow("Teste", WINDOW_W, WINDOW_H, SDL_WINDOW_RESIZABLE);
  if (!sdl_components.window) {
    SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
    return false;
  }

  if (!SDL_Init(SDL_INIT_GAMEPAD)) {
    puts("No gamepad connected, using keyboard");
  } else {
    puts("Gamepad connected");
    //  load_gamepads();
  }

  // Initializes basic config
  WGPUInstanceDescriptor instance_desc = WGPU_INSTANCE_DESCRIPTOR_INIT;
  wgpu_components.instance = wgpuCreateInstance(&instance_desc);
  if (!wgpu_components.instance) {
    puts("couldn't create an instance");
  }

  WGPURequestAdapterOptions adapter_opt = WGPU_REQUEST_ADAPTER_OPTIONS_INIT;
  WGPURequestAdapterCallbackInfo adapter_callback_info =
      WGPU_REQUEST_ADAPTER_CALLBACK_INFO_INIT;
  WGPUAdapter adapter;
  adapter_callback_info.userdata1 = &adapter;
  adapter_callback_info.callback = adapter_callback;
  wgpuInstanceRequestAdapter(wgpu_components.instance, &adapter_opt,
                             adapter_callback_info);

  WGPUDeviceDescriptor device_desc = WGPU_DEVICE_DESCRIPTOR_INIT;
  WGPURequestDeviceCallbackInfo callback_info =
      WGPU_REQUEST_DEVICE_CALLBACK_INFO_INIT;
  callback_info.userdata1 = &wgpu_components.device;
  callback_info.callback = device_callback;
  wgpuAdapterRequestDevice(adapter, &device_desc, callback_info);
  if (!wgpu_components.device) {
    puts("Device failed");
    return -1;
  }

  wgpu_components.surface =
      SDL_GetWGPUSurface(wgpu_components.instance, sdl_components.window);
  WGPUSurfaceCapabilities capabilites;
  wgpuSurfaceGetCapabilities(wgpu_components.surface, adapter, &capabilites);
  WGPUSurfaceConfiguration surface_config = WGPU_SURFACE_CONFIGURATION_INIT;
  surface_config.device = wgpu_components.device;
  surface_config.format = capabilites.formats[0];
  surface_config.width = WINDOW_W;
  surface_config.height = WINDOW_H;
  surface_config.viewFormatCount = capabilites.formatCount;
  surface_config.viewFormats = &capabilites.formats[0];
  surface_config.alphaMode = capabilites.alphaModes[0];
  wgpuSurfaceConfigure(wgpu_components.surface, &surface_config);
  wgpuAdapterRelease(adapter);

  // Initializes pipeline
  WGPUShaderSourceWGSL shader_source = WGPU_SHADER_SOURCE_WGSL_INIT;
  shader_source.chain.sType = WGPUSType_ShaderSourceWGSL;

  const char *shader_code = load_shader();
  shader_source.code = (WGPUStringView){shader_code, WGPU_STRLEN};
  WGPUShaderModuleDescriptor shader_desc = WGPU_SHADER_MODULE_DESCRIPTOR_INIT;
  shader_desc.nextInChain = &shader_source.chain;
  wgpu_components.shader_module =
      wgpuDeviceCreateShaderModule(wgpu_components.device, &shader_desc);

  // preparing vertex buffer
  WGPUBufferDescriptor vertex_buffer_desc = WGPU_BUFFER_DESCRIPTOR_INIT;
  vertex_buffer_desc.label = (WGPUStringView){"Vertex Buffer", WGPU_STRLEN};
  vertex_buffer_desc.usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst;
  vertex_buffer_desc.size = sizeof(vertex_buffer_values);
  vertex_buffer =
      wgpuDeviceCreateBuffer(wgpu_components.device, &vertex_buffer_desc);
  WGPUVertexAttribute vertex_attribute = WGPU_VERTEX_ATTRIBUTE_INIT;
  vertex_attribute.format = WGPUVertexFormat_Float32x3;
  vertex_attribute.shaderLocation = 0;
  WGPUVertexBufferLayout vertex_buffer_layout = WGPU_VERTEX_BUFFER_LAYOUT_INIT;
  vertex_buffer_layout.stepMode = WGPUVertexStepMode_Vertex;
  vertex_buffer_layout.attributes = &vertex_attribute;
  vertex_buffer_layout.attributeCount = 1;
  vertex_buffer_layout.arrayStride = sizeof(float) * 3;

  // preparing vertex state, using vertex buffer layout
  wgpu_components.vertex_state = WGPU_VERTEX_STATE_INIT;
  wgpu_components.vertex_state.bufferCount = 1;
  wgpu_components.vertex_state.buffers = &vertex_buffer_layout;
  wgpu_components.vertex_state.entryPoint = (WGPUStringView){"vs", WGPU_STRLEN};

  WGPURenderPipelineDescriptor render_pipeline_desc =
      WGPU_RENDER_PIPELINE_DESCRIPTOR_INIT;
  render_pipeline_desc.vertex = wgpu_components.vertex_state;
  render_pipeline_desc.vertex.module = wgpu_components.shader_module;
  WGPUFragmentState fragment = WGPU_FRAGMENT_STATE_INIT;
  fragment.module = wgpu_components.shader_module;
  fragment.entryPoint = (WGPUStringView){"fs", WGPU_STRLEN};
  WGPUBlendState blendState = WGPU_BLEND_STATE_INIT;
  WGPUColorTargetState colorTarget = WGPU_COLOR_TARGET_STATE_INIT;
  colorTarget.blend = &blendState;
  colorTarget.format = capabilites.formats[0];
  fragment.targetCount = 1;
  fragment.targets = &colorTarget;
  render_pipeline_desc.fragment = &fragment;
  // mudar depois pra fazer o teste
  render_pipeline_desc.primitive.cullMode = WGPUCullMode_None;
  wgpu_components.render_pipeline = wgpuDeviceCreateRenderPipeline(
      wgpu_components.device, &render_pipeline_desc);
  wgpuShaderModuleRelease(wgpu_components.shader_module);

  // preparing uniforms buffer
  WGPUBufferDescriptor uniform_buffer_descriptor = WGPU_BUFFER_DESCRIPTOR_INIT;
  uniform_buffer_descriptor.label =
      (WGPUStringView){"Uniform Buffer", WGPU_STRLEN};
  uniform_buffer_descriptor.usage =
      WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;
  uniform_buffer_descriptor.size = sizeof(uniform_default_values);
  uniform_buffer = wgpuDeviceCreateBuffer(wgpu_components.device,
                                          &uniform_buffer_descriptor);
  WGPUBindGroupEntry bind_group_entry = WGPU_BIND_GROUP_ENTRY_INIT;
  bind_group_entry.binding = 0;
  bind_group_entry.buffer = uniform_buffer;
  WGPUBindGroupDescriptor uniform_buffer_bind_group_desc =
      WGPU_BIND_GROUP_DESCRIPTOR_INIT;
  uniform_buffer_bind_group_desc.entries = &bind_group_entry;
  uniform_buffer_bind_group_desc.entryCount = 1;
  uniform_buffer_bind_group_desc.layout =
      wgpuRenderPipelineGetBindGroupLayout(wgpu_components.render_pipeline, 0);
  wgpu_components.bind_group = wgpuDeviceCreateBindGroup(
      wgpu_components.device, &uniform_buffer_bind_group_desc);

  wgpu_components.queue = wgpuDeviceGetQueue(wgpu_components.device);

  return true;
}

void main_loop() {
  SDL_Event event;
  bool is_running = true;
  Sint16 left_axis_x, left_axis_y, right_axis_x, right_axis_y;

  float directions[] = {0, 0, 1};
  float rotations[] = {0, 0, 0};
  float scalings[] = {1, 1};

  float *translation_matrix = NULL;
  float *scaling_matrix = NULL;
  float *rotation_matrix = NULL;
  float *transformation_matrix = NULL;

  while (is_running) {
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
      case SDL_EVENT_QUIT:
        is_running = false;
        break;

      case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
        printf("Button pressed: %s\n",
               SDL_GetGamepadStringForButton(
                   (SDL_GamepadButton)event.gbutton.button));
        break;

      case SDL_EVENT_GAMEPAD_BUTTON_UP:
        printf("Button released: %s\n",
               SDL_GetGamepadStringForButton(
                   (SDL_GamepadButton)event.gbutton.button));
        break;

      case SDL_EVENT_GAMEPAD_AXIS_MOTION:
        left_axis_x =
            SDL_GetGamepadAxis(sdl_components.gamepad, SDL_GAMEPAD_AXIS_LEFTX);
        left_axis_y =
            SDL_GetGamepadAxis(sdl_components.gamepad, SDL_GAMEPAD_AXIS_LEFTY);

        right_axis_x =
            SDL_GetGamepadAxis(sdl_components.gamepad, SDL_GAMEPAD_AXIS_RIGHTX);
        right_axis_y =
            SDL_GetGamepadAxis(sdl_components.gamepad, SDL_GAMEPAD_AXIS_RIGHTY);
        break;
      }
    }

    update_player_transformation(directions, rotations, scalings);
    translation_matrix =
        apply_rotation(rotations[0], rotations[1], rotations[2]);
    rotation_matrix =
        apply_translation(directions[0], directions[1], directions[2]);
    scaling_matrix = apply_scaling(scalings[0], scalings[1], 0);

    transformation_matrix =
        multiply_matrices(scaling_matrix, translation_matrix);
    transformation_matrix =
        multiply_matrices(transformation_matrix, rotation_matrix);

    //apply_perspective(transformation_matrix);
    normalize_matrix(transformation_matrix);
    print_matrix(transformation_matrix);
    memcpy(uniform_default_values.transform, transformation_matrix,
           sizeof(float) * MATRIX_SIZE);

    wgpuQueueWriteBuffer(wgpu_components.queue, vertex_buffer, 0,
                         vertex_buffer_values, sizeof(vertex_buffer_values));
    wgpuQueueWriteBuffer(wgpu_components.queue, uniform_buffer, 0,
                         &uniform_default_values,
                         sizeof(uniform_default_values));

    WGPUSurfaceTexture surface_texture = WGPU_SURFACE_TEXTURE_INIT;
    wgpuSurfaceGetCurrentTexture(wgpu_components.surface, &surface_texture);
    WGPUTextureViewDescriptor texture_view_desc =
        WGPU_TEXTURE_VIEW_DESCRIPTOR_INIT;
    WGPUTextureView texture_view =
        wgpuTextureCreateView(surface_texture.texture, &texture_view_desc);
    if (!texture_view) {
      puts("Texture View error!");
      return;
    }

    WGPUCommandEncoderDescriptor command_encoder_desc =
        WGPU_COMMAND_ENCODER_DESCRIPTOR_INIT;
    WGPUCommandEncoder command_encoder = wgpuDeviceCreateCommandEncoder(
        wgpu_components.device, &command_encoder_desc);

    WGPURenderPassColorAttachment color_attachment =
        WGPU_RENDER_PASS_COLOR_ATTACHMENT_INIT;
    color_attachment.view = texture_view;
    color_attachment.loadOp = WGPULoadOp_Clear;
    color_attachment.storeOp = WGPUStoreOp_Store;
    color_attachment.clearValue = (const WGPUColor){0.5, 0.6, 0.7, 1.0};

    WGPURenderPassDescriptor render_pass_desc =
        WGPU_RENDER_PASS_DESCRIPTOR_INIT;
    render_pass_desc.colorAttachmentCount = 1;
    render_pass_desc.colorAttachments = &color_attachment;
    // A render pass encoder
    WGPURenderPassEncoder render_pass_encoder =
        wgpuCommandEncoderBeginRenderPass(command_encoder, &render_pass_desc);

    wgpuRenderPassEncoderSetPipeline(render_pass_encoder,
                                     wgpu_components.render_pipeline);

    wgpuRenderPassEncoderSetVertexBuffer(render_pass_encoder, 0, vertex_buffer,
                                         0, sizeof(vertex_buffer_values));

    wgpuRenderPassEncoderSetBindGroup(render_pass_encoder, 0,
                                      wgpu_components.bind_group, 0, 0);
    wgpuRenderPassEncoderDraw(render_pass_encoder, 3, 1, 0, 0);
    wgpuRenderPassEncoderEnd(render_pass_encoder);

    WGPUCommandBufferDescriptor command_buffer_desc =
        WGPU_COMMAND_BUFFER_DESCRIPTOR_INIT;
    WGPUCommandBuffer command_buffer =
        wgpuCommandEncoderFinish(command_encoder, &command_buffer_desc);
    wgpuCommandEncoderRelease(command_encoder);

    wgpuQueueSubmit(wgpu_components.queue, 1, &command_buffer);
    wgpuSurfacePresent(wgpu_components.surface);
    wgpuCommandBufferRelease(command_buffer);

    wgpuTextureViewRelease(texture_view);

    SDL_Delay(16); // Sleep 16ms (~60FPS)
  }
}

void terminate() {
  wgpuBindGroupRelease(wgpu_components.bind_group);
  wgpuBufferRelease(uniform_buffer);
  wgpuRenderPipelineRelease(wgpu_components.render_pipeline);
  wgpuSurfaceUnconfigure(wgpu_components.surface);
  wgpuSurfaceRelease(wgpu_components.surface);
  wgpuDeviceRelease(wgpu_components.device);
  wgpuInstanceRelease(wgpu_components.instance);
  SDL_CloseGamepad(sdl_components.gamepad);
  SDL_DestroyWindow(sdl_components.window);
  SDL_Quit();
}

char *load_shader() {
  FILE *file = fopen("src/shaders/main.wgsl", "r");
  if (file == NULL) {
    puts("Shader file not found");
    return false;
  }
  long length;
  fseek(file, 0, SEEK_END);
  length = ftell(file);
  fseek(file, 0, SEEK_SET);
  char *buffer = malloc(length + 1);
  buffer = malloc(length);
  if (buffer) {
    fread(buffer, 1, length, file);
    buffer[length] = '\0';
  }
  fclose(file);
  return buffer;
}

void adapter_callback(WGPURequestAdapterStatus status, WGPUAdapter adapter,
                      WGPUStringView message, void *userdata1,
                      void *userdata2) {
  if (status == WGPURequestAdapterStatus_Success) {
    *(WGPUAdapter *)userdata1 = adapter;
    printf("Adapter loaded: %p\n", adapter);
  } else {
    printf("Adapter error: %s\n", message.data);
  }
}

void device_callback(WGPURequestDeviceStatus status, WGPUDevice device,
                     WGPUStringView message, void *userdata1, void *userdata2) {
  if (status == WGPURequestDeviceStatus_Success) {
    *(WGPUDevice *)userdata1 = device;
    printf("Device loaded: %p\n", device);

  } else {
    printf("Device error: %s\n", message.data);
  }
}

void update_player_transformation(float *directions, float *rotations,
                                  float *scalings) {
  const bool *key_states = SDL_GetKeyboardState(NULL);

  // direction
  if (key_states[SDL_SCANCODE_D]) {
    directions[0] += 0.1;
  }
  if (key_states[SDL_SCANCODE_A]) {
    directions[0] += -0.1;
  }
  if (key_states[SDL_SCANCODE_W]) {
    directions[2] += 0.1;
  }
  if (key_states[SDL_SCANCODE_S]) {
    directions[2] += -0.1;
  }
  if (key_states[SDL_SCANCODE_E]) {
    directions[1] += 0.1;
  }
  if (key_states[SDL_SCANCODE_Q]) {
    directions[1] += -0.1;
  }

  // rotation
  if (key_states[SDL_SCANCODE_UP]) {
    rotations[1] += 0.1;
  }
  if (key_states[SDL_SCANCODE_DOWN]) {
    rotations[1] += -0.1;
  }
  if (key_states[SDL_SCANCODE_LEFT]) {
    rotations[0] += -0.1;
  }
  if (key_states[SDL_SCANCODE_RIGHT]) {
    rotations[0] += 0.1;
  }
  if (key_states[SDL_SCANCODE_RSHIFT]) {
    rotations[2] += 0.1;
  }
  if (key_states[SDL_SCANCODE_RCTRL]) {
    rotations[2] += -0.1;
  }

  // scaling
  if (key_states[SDL_SCANCODE_PAGEDOWN]) {
    scalings[0] += 0.5;
  }
  if (key_states[SDL_SCANCODE_DELETE]) {
    scalings[0] += -0.5;
  }
  if (key_states[SDL_SCANCODE_HOME]) {
    scalings[1] += 0.5;
  }
  if (key_states[SDL_SCANCODE_END]) {
    scalings[1] += -0.5;
  }
}
