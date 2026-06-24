 #include <vulkan/vulkan.h>

 
class PipelineBuilder { 
public: 
    std::vector<VkPipelineShaderStageCreateInfo> stages; 
    VkPipelineVertexInputStateCreateInfo   vertexInput{}; 
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{}; 
    VkPipelineRasterizationStateCreateInfo rasterizer{}; 
    VkPipelineMultisampleStateCreateInfo   multisampling{}; 
    VkPipelineColorBlendAttachmentState    colorBlendAttach{}; 
    VkPipelineDepthStencilStateCreateInfo  depthStencil{}; 
 
    static PipelineBuilder defaults() { 
        PipelineBuilder pb; 
        // Triangles par défaut 
        pb.inputAssembly.sType    = 
VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO; 
        pb.inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; 
        // Rasterizer solide 
        pb.rasterizer.sType       = 
VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO; 
        pb.rasterizer.polygonMode = VK_POLYGON_MODE_FILL; 
        pb.rasterizer.cullMode    = VK_CULL_MODE_BACK_BIT; 
        pb.rasterizer.frontFace   = VK_FRONT_FACE_COUNTER_CLOCKWISE; 
        pb.rasterizer.lineWidth   = 1.0f; 
        // MSAA désactivé 
        pb.multisampling.sType    = 
VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO; 
        pb.multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT; 
        // Pas de blending 
        pb.colorBlendAttach.colorWriteMask = 
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | 
            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT; 
        return pb; 
    } 
 
    VkPipeline build(VkDevice device, VkRenderPass rp, VkPipelineLayout layout) 
{ 
        // Viewport & scissor dynamiques 
        VkDynamicState dynStates[] = { VK_DYNAMIC_STATE_VIEWPORT, 
VK_DYNAMIC_STATE_SCISSOR }; 
        VkPipelineDynamicStateCreateInfo dynState{}; 
        dynState.sType             = 
VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO; 
        dynState.dynamicStateCount = 2; 
        dynState.pDynamicStates    = dynStates; 
 
        VkPipelineViewportStateCreateInfo vpState{}; 
        vpState.sType         = 
VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO; 
        vpState.viewportCount = 1; 
        vpState.scissorCount  = 1; 
 
        VkPipelineColorBlendStateCreateInfo colorBlend{}; 
        colorBlend.sType           = 
VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO; 
        colorBlend.attachmentCount = 1; 
        colorBlend.pAttachments    = &colorBlendAttach; 
 
        VkGraphicsPipelineCreateInfo ci{}; 
        ci.sType               = 
VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO; 
        ci.stageCount          = (uint32_t)stages.size(); 
        ci.pStages             = stages.data(); 
        ci.pVertexInputState   = &vertexInput; 
        ci.pInputAssemblyState = &inputAssembly; 
        ci.pViewportState      = &vpState; 
        ci.pRasterizationState = &rasterizer; 
        ci.pMultisampleState   = &multisampling; 
        ci.pColorBlendState    = &colorBlend; 
        ci.pDynamicState       = &dynState; 
        ci.layout              = layout; 
        ci.renderPass          = rp; 
        ci.subpass             = 0; 
 
        VkPipeline pipeline;
        
        vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &ci, nullptr, 
        &pipeline); 
        return pipeline; 
    } 
}; 