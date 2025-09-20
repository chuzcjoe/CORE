#include "VulkanGraphic.h"

namespace core {
namespace vulkan {

VulkanGraphic::VulkanGraphic(VulkanContext* context, VulkanRenderPass& render_pass)
    : VulkanBase(context), render_pass_(render_pass) {}

void VulkanGraphic::CreatePipeline() {
  // 1. shader stage
  const auto vertex_shader_module = CreateShaderModule(LoadVertexShader());
  const auto fragment_shader_module = CreateShaderModule(LoadFragmentShader());

  VkPipelineShaderStageCreateInfo vert_info{};
  vert_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vert_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vert_info.module = vertex_shader_module;
  vert_info.pName = "main";

  VkPipelineShaderStageCreateInfo frag_info{};
  frag_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  frag_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  frag_info.module = fragment_shader_module;
  frag_info.pName = "main";

  std::vector<VkPipelineShaderStageCreateInfo> shader_stages = {vert_info, frag_info};

  // 2. vertex input state
  const auto vertex_bindings = GetVertexBindingDescriptions();
  const auto vertex_attribute = GetVertexAttributeDescriptions();
  VkPipelineVertexInputStateCreateInfo vertex_input_state{};
  vertex_input_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertex_input_state.vertexBindingDescriptionCount = vertex_bindings.size();
  vertex_input_state.pVertexBindingDescriptions = vertex_bindings.data();
  vertex_input_state.vertexAttributeDescriptionCount = vertex_attribute.size();
  vertex_input_state.pVertexAttributeDescriptions = vertex_attribute.data();

  // 3. input assembly state
  VkPipelineInputAssemblyStateCreateInfo input_assembly_state{};
  input_assembly_state.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  input_assembly_state.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  input_assembly_state.primitiveRestartEnable = VK_FALSE;

  // 4. view port state
  VkPipelineViewportStateCreateInfo viewport_state{};
  viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewport_state.viewportCount = 1;
  viewport_state.scissorCount = 1;

  // 5. rasterization state
  VkPipelineRasterizationStateCreateInfo rasterize_state{};
  rasterize_state.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterize_state.depthClampEnable = VK_FALSE;
  rasterize_state.rasterizerDiscardEnable = VK_FALSE;
  rasterize_state.polygonMode = VK_POLYGON_MODE_FILL;
  rasterize_state.lineWidth = 1.0f;

  rasterize_state.cullMode = SetCullMode();
  rasterize_state.frontFace = SetFrontFace();

  rasterize_state.depthBiasEnable = VK_FALSE;
  rasterize_state.depthBiasConstantFactor = 0.0f;
  rasterize_state.depthBiasClamp = 0.0f;
  rasterize_state.depthBiasSlopeFactor = 0.0f;

  // 6. multisample state
  VkPipelineMultisampleStateCreateInfo multisample_state{};
  multisample_state.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisample_state.sampleShadingEnable = VK_FALSE;
  multisample_state.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  multisample_state.minSampleShading = 1.0f;
  multisample_state.pSampleMask = nullptr;
  multisample_state.alphaToCoverageEnable = VK_FALSE;
  multisample_state.alphaToOneEnable = VK_FALSE;

  // 7. dynamic state
  std::vector<VkDynamicState> dynamic_state_enables = {VK_DYNAMIC_STATE_VIEWPORT,
                                                       VK_DYNAMIC_STATE_SCISSOR};
  VkPipelineDynamicStateCreateInfo dynamic_state{};
  dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamic_state.pDynamicStates = dynamic_state_enables.data();
  dynamic_state.dynamicStateCount = static_cast<uint32_t>(dynamic_state_enables.size());

  // 8. color blending state
  VkPipelineColorBlendAttachmentState color_blend_attachment{};
  color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  color_blend_attachment.blendEnable = VK_FALSE;

  VkPipelineColorBlendStateCreateInfo color_blending_state{};
  color_blending_state.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  color_blending_state.logicOpEnable = VK_FALSE;
  color_blending_state.logicOp = VK_LOGIC_OP_COPY;
  color_blending_state.attachmentCount = 1;
  color_blending_state.pAttachments = &color_blend_attachment;
  color_blending_state.blendConstants[0] = 0.0f;
  color_blending_state.blendConstants[1] = 0.0f;
  color_blending_state.blendConstants[2] = 0.0f;
  color_blending_state.blendConstants[3] = 0.0f;

  // create graphic pipeline
  VkGraphicsPipelineCreateInfo pipeline_info{};
  pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipeline_info.stageCount = shader_stages.size();
  pipeline_info.pStages = shader_stages.data();
  pipeline_info.pVertexInputState = &vertex_input_state;
  pipeline_info.pInputAssemblyState = &input_assembly_state;
  pipeline_info.pViewportState = &viewport_state;
  pipeline_info.pRasterizationState = &rasterize_state;
  pipeline_info.pMultisampleState = &multisample_state;
  pipeline_info.pDepthStencilState = nullptr;
  pipeline_info.pColorBlendState = &color_blending_state;
  pipeline_info.pDynamicState = &dynamic_state;
  pipeline_info.layout = pipeline_layout;
  pipeline_info.renderPass = render_pass_.GetRenderPass();
  pipeline_info.subpass = 0;
  pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
  pipeline_info.basePipelineIndex = -1;

  VK_CHECK(vkCreateGraphicsPipelines(context_->logical_device, VK_NULL_HANDLE, 1, &pipeline_info,
                                     nullptr, &pipeline));
  vkDestroyShaderModule(context_->logical_device, fragment_shader_module, nullptr);
  vkDestroyShaderModule(context_->logical_device, vertex_shader_module, nullptr);
}

}  // namespace vulkan
}  // namespace core