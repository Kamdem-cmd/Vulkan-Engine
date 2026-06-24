#include <vulkan/vulkan.h>

// Créer une VkImage + la mémoire associée 
void createImage(uint32_t w, uint32_t h, uint32_t mips, 
    VkFormat fmt, VkImageTiling tiling, VkImageUsageFlags usage, 
    VkMemoryPropertyFlags props, 
    VkImage& image, VkDeviceMemory& memory) { 
 
    VkImageCreateInfo ci{}; 
    ci.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO; 
    ci.imageType     = VK_IMAGE_TYPE_2D; 
    ci.format        = fmt; 
    ci.extent        = { w, h, 1 }; 
    ci.mipLevels     = mips; 
    ci.arrayLayers   = 1; 
    ci.samples       = VK_SAMPLE_COUNT_1_BIT; 
    ci.tiling        = tiling; 
    ci.usage         = usage; 
    ci.sharingMode   = VK_SHARING_MODE_EXCLUSIVE; 
    ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; 
    vkCreateImage(device, &ci, nullptr, &image); 
 
    VkMemoryRequirements req; 
    vkGetImageMemoryRequirements(device, image, &req); 
    VkMemoryAllocateInfo alloc{}; 
    alloc.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO; 
    alloc.allocationSize  = req.size; 
    alloc.memoryTypeIndex = findMemoryType(physDevice, req.memoryTypeBits, 
props); 
    vkAllocateMemory(device, &alloc, nullptr, &memory); 
    vkBindImageMemory(device, image, memory, 0); 
} 
 
// Transition de layout avec une pipeline barrier 
void transitionLayout(VkImage img, VkImageLayout from, VkImageLayout to, 
uint32_t mips) { 
    VkCommandBuffer cb = beginOneTimeCommand(device, cmdPool); 
    VkImageMemoryBarrier barrier{}; 
    barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER; 
    barrier.oldLayout           = from; 
    barrier.newLayout           = to; 
    barrier.image               = img; 
    barrier.subresourceRange    = { VK_IMAGE_ASPECT_COLOR_BIT, 0, mips, 0, 1 }; 
 
    VkPipelineStageFlags srcStage, dstStage; 
    if (from == VK_IMAGE_LAYOUT_UNDEFINED && to == 
VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) { 
        barrier.srcAccessMask = 0; barrier.dstAccessMask = 
VK_ACCESS_TRANSFER_WRITE_BIT; 
        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT; 
        dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT; 
    } else if (from == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && 
               to   == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) { 
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; 
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT; 
        srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT; 
dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT; 
} 
vkCmdPipelineBarrier(cb, srcStage, dstStage, 0, 0,nullptr, 0,nullptr, 
1,&barrier); 
endOneTimeCommand(device, cmdPool, graphicsQ, cb); 
} 