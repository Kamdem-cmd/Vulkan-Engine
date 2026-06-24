#include <vulkan/vulkan.h>

 
// Trouver le bon type de mémoire GPU 
uint32_t findMemoryType(VkPhysicalDevice physDev, 
    uint32_t typeFilter, VkMemoryPropertyFlags props) { 
    VkPhysicalDeviceMemoryProperties memProps; 
    vkGetPhysicalDeviceMemoryProperties(physDev, &memProps); 
    for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) { 
        if ((typeFilter & (1 << i)) && 
            (memProps.memoryTypes[i].propertyFlags & props) == props) 
            return i; 
    } 
    throw std::runtime_error("findMemoryType: no suitable type"); 
} 
 
// Classe Buffer — encapsule VkBuffer + VkDeviceMemory 
struct Buffer { 
    VkBuffer       handle = VK_NULL_HANDLE; 
    VkDeviceMemory memory = VK_NULL_HANDLE; 
    VkDeviceSize   size   = 0; 
 
    void create(VkDevice dev, VkPhysicalDevice phys, 
        VkDeviceSize sz, VkBufferUsageFlags usage, VkMemoryPropertyFlags props) 
{ 
        size = sz; 
        VkBufferCreateInfo ci{}; 
        ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO; 
        ci.size  = sz; ci.usage = usage; 
        ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE; 
        vkCreateBuffer(dev, &ci, nullptr, &handle); 
 
        VkMemoryRequirements req; 
        vkGetBufferMemoryRequirements(dev, handle, &req); 
 
        VkMemoryAllocateInfo alloc{}; 
        alloc.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO; 
        alloc.allocationSize  = req.size; 
        alloc.memoryTypeIndex = findMemoryType(phys, req.memoryTypeBits, props); 
        vkAllocateMemory(dev, &alloc, nullptr, &memory); 
        vkBindBufferMemory(dev, handle, memory, 0); 
    } 
 
    // Upload via staging buffer 
    void upload(VkDevice dev, VkPhysicalDevice phys, 
        VkCommandPool pool, VkQueue q, 
        const void* data, VkDeviceSize sz) { 
            // 1. Staging buffer (HOST_VISIBLE pour accès CPU) 
            Buffer staging; 
            staging.create(dev, phys, sz, 
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT); 
            void* ptr; 
            vkMapMemory(dev, staging.memory, 0, sz, 0, &ptr); 
            memcpy(ptr, data, sz); 
            vkUnmapMemory(dev, staging.memory); 
            // 2. Copie GPU : staging → buffer DEVICE_LOCAL 
            VkCommandBuffer cb = beginOneTimeCommand(dev, pool); 
            VkBufferCopy copy{ 0, 0, sz }; 
            vkCmdCopyBuffer(cb, staging.handle, handle, 1, &copy); 
            endOneTimeCommand(dev, pool, q, cb); 
            staging.destroy(dev); 
        } 
        void destroy(VkDevice dev) { 
        if (handle) vkDestroyBuffer(dev, handle, nullptr); 
        if (memory) vkFreeMemory(dev, memory, nullptr); 
    } 
};


// Binding 0 : UBO caméra (accessible depuis vertex + fragment shader) 
VkDescriptorSetLayoutBinding uboBinding{}; 
uboBinding.binding            = 0; 
uboBinding.descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; 
uboBinding.descriptorCount    = 1; 
uboBinding.stageFlags         = VK_SHADER_STAGE_VERTEX_BIT | 
VK_SHADER_STAGE_FRAGMENT_BIT; 
 
VkDescriptorSetLayoutCreateInfo layoutCI{}; 
layoutCI.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO; 
layoutCI.bindingCount = 1; 
layoutCI.pBindings    = &uboBinding; 
vkCreateDescriptorSetLayout(device, &layoutCI, nullptr, &descSetLayout); 
 
// 2. Créer le pool d'allocation 
VkDescriptorPoolSize poolSize{}; 
poolSize.type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; 
poolSize.descriptorCount = MAX_FRAMES_IN_FLIGHT; 
 
VkDescriptorPoolCreateInfo poolCI{}; 
poolCI.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO; 
poolCI.maxSets       = MAX_FRAMES_IN_FLIGHT; 
poolCI.poolSizeCount = 1; 
poolCI.pPoolSizes    = &poolSize; 
vkCreateDescriptorPool(device, &poolCI, nullptr, &descPool); 
 
// 3. Allouer les descriptor sets (un par frame) 
std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descSetLayout); 
VkDescriptorSetAllocateInfo allocCI{}; 
allocCI.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO; 
allocCI.descriptorPool     = descPool; 
allocCI.descriptorSetCount = MAX_FRAMES_IN_FLIGHT; 
allocCI.pSetLayouts        = layouts.data(); 
vkAllocateDescriptorSets(device, &allocCI, descSets.data()); 
 
// 4. Écrire les ressources dans les descriptor sets 
for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) { 
    VkDescriptorBufferInfo bufInfo{ uboBuffers[i].handle, 0, sizeof(UBOCamera) 
}; 
    VkWriteDescriptorSet write{}; 
    write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET; 
    write.dstSet          = descSets[i]; 
    write.dstBinding      = 0; 
    write.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; 
    write.descriptorCount = 1; 
    write.pBufferInfo     = &bufInfo; 
    vkUpdateDescriptorSets(device, 1, &write, 0, nullptr); 
} 

// Push Constants — par objet, plus rapide qu'un UBO 
// Dans le shader GLSL (Vulkan) : 
// layout(push_constant) uniform PushConstants { 
//     mat4 model; 
// } pc; 
// Dans le command buffer : 
NkMat4x4 modelMatrix = transform.toMat4(); 
vkCmdPushConstants(cmdBuf, pipelineLayout, 
VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(NkMat4x4), modelMatrix.ptr()); 