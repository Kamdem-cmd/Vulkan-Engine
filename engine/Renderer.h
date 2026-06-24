#include <vulkan/vulkan.h>

static const int MAX_FRAMES_IN_FLIGHT = 2;  // double buffering CPU-GPU 

// Synchronisation : une paire de semaphores + une fence par frame en vol 
VkSemaphore imageAvailableSem[MAX_FRAMES_IN_FLIGHT]; 
VkSemaphore renderFinishedSem[MAX_FRAMES_IN_FLIGHT]; 
VkFence     inFlightFence    [MAX_FRAMES_IN_FLIGHT]; 
 
int currentFrame = 0; 
 
void drawFrame() { 
    // 1. Attendre que le GPU ait fini avec cette frame 
    vkWaitForFences(device, 1, &inFlightFence[currentFrame], VK_TRUE, UINT64_MAX); 
 
    // 2. Acquérir la prochaine image de la swapchain 
    uint32_t imageIndex; 
    VkResult r = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, 
        imageAvailableSem[currentFrame], VK_NULL_HANDLE, &imageIndex); 
    if (r == VK_ERROR_OUT_OF_DATE_KHR) { recreateSwapchain(); return; } 
 
    vkResetFences(device, 1, &inFlightFence[currentFrame]); 
 
    // 3. Enregistrer et soumettre le command buffer 
    vkResetCommandBuffer(cmdBuffers[currentFrame], 0); 
    recordCommandBuffer(cmdBuffers[currentFrame], imageIndex); 
 
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT }; 
    VkSubmitInfo submit{}; 
    submit.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO; 
    submit.waitSemaphoreCount   = 1; 
    submit.pWaitSemaphores      = &imageAvailableSem[currentFrame]; 
    submit.pWaitDstStageMask    = waitStages; 
    submit.commandBufferCount   = 1; 
    submit.pCommandBuffers      = &cmdBuffers[currentFrame]; 
    submit.signalSemaphoreCount = 1; 
    submit.pSignalSemaphores    = &renderFinishedSem[currentFrame]; 
    vkQueueSubmit(graphicsQ, 1, &submit, inFlightFence[currentFrame]); 
 
    // 4. Présenter l'image à l'écran 
    VkPresentInfoKHR present{}; 
    present.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR; 
    present.waitSemaphoreCount = 1; 
    present.pWaitSemaphores    = &renderFinishedSem[currentFrame]; 
    present.swapchainCount     = 1; 
    present.pSwapchains        = &swapchain; 
    present.pImageIndices      = &imageIndex; 
    vkQueuePresentKHR(presentQ, &present); 
 
    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT; 
}

struct SwapchainSupport { 
    VkSurfaceCapabilitiesKHR        caps; 
    std::vector<VkSurfaceFormatKHR> formats; 
    std::vector<VkPresentModeKHR>   presentModes; 
}; 
 
SwapchainSupport querySwapchainSupport(VkPhysicalDevice dev) { 
    SwapchainSupport s; 
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(dev, surface, &s.caps); 
    uint32_t n; 
    vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surface, &n, nullptr); 
    s.formats.resize(n); 
    vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surface, &n, s.formats.data()); 
    vkGetPhysicalDeviceSurfacePresentModesKHR(dev, surface, &n, nullptr); 
    s.presentModes.resize(n); 
    vkGetPhysicalDeviceSurfacePresentModesKHR(dev, surface, &n, 
s.presentModes.data()); 
    return s; 
} 
 
VkSurfaceFormatKHR chooseFormat(const std::vector<VkSurfaceFormatKHR>& formats) 
{ 
    for (auto& f : formats) 
        if (f.format == VK_FORMAT_B8G8R8A8_SRGB && 
            f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) return f; 
    return formats[0]; // fallback 
} 
 
VkPresentModeKHR choosePresentMode(const std::vector<VkPresentModeKHR>& modes) { 
    for (auto& m : modes) 
        if (m == VK_PRESENT_MODE_MAILBOX_KHR) return m; // triple buffering 
    return VK_PRESENT_MODE_FIFO_KHR; // vsync — toujours supporté 
} 
 
VkExtent2D chooseExtent(const VkSurfaceCapabilitiesKHR& caps, NkWindow* win) { 
    if (caps.currentExtent.width != UINT32_MAX) 
        return caps.currentExtent; // le OS impose la taille 
    // Sinon, utiliser la taille de la fenêtre 
    VkExtent2D ext = { (uint32_t)win->GetWidth(), (uint32_t)win->GetHeight() }; 
    ext.width  = std::clamp(ext.width,  caps.minImageExtent.width,  
caps.maxImageExtent.width); 
    ext.height = std::clamp(ext.height, caps.minImageExtent.height, 
caps.maxImageExtent.height); 
    return ext; 
} 

void createRenderPass() { 
    // Décrire l'attachement couleur 
    VkAttachmentDescription colorAttach{}; 
    colorAttach.format         = swapchainFormat; 
    colorAttach.samples        = VK_SAMPLE_COUNT_1_BIT; 
    colorAttach.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;   // effacer au début 
    colorAttach.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;  // garder à la fin 
    colorAttach.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE; 
    colorAttach.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; 
    colorAttach.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED; 
    colorAttach.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // prêt pour présentation 
 
    // Référence dans le subpass 
    VkAttachmentReference colorRef{}; 
    colorRef.attachment = 0; 
    colorRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; 
 
    VkSubpassDescription subpass{}; 
    subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS; 
    subpass.colorAttachmentCount = 1; 
    subpass.pColorAttachments    = &colorRef; 
 
    // Dépendance : attendre que la swapchain soit prête avant de dessiner 
    VkSubpassDependency dep{}; 
    dep.srcSubpass    = VK_SUBPASS_EXTERNAL; 
    dep.dstSubpass    = 0; 
    dep.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; 
    dep.srcAccessMask = 0; 
    dep.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; 
    dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; 
 
    VkRenderPassCreateInfo ci{}; 
    ci.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO; 
    ci.attachmentCount = 1; 
    ci.pAttachments    = &colorAttach; 
    ci.subpassCount    = 1; 
    ci.pSubpasses      = &subpass; 
    ci.dependencyCount = 1; 
    ci.pDependencies   = &dep; 
 
    vkCreateRenderPass(device, &ci, nullptr, &renderPass); 
}

 
 
void createFramebuffers() { 
    framebuffers.resize(swapchainImageViews.size()); 
    for (size_t i = 0; i < swapchainImageViews.size(); i++) { 
        VkFramebufferCreateInfo ci{}; 
        ci.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO; 
        ci.renderPass      = renderPass; 
        ci.attachmentCount = 1; 
        ci.pAttachments    = &swapchainImageViews[i]; 
        ci.width           = swapchainExtent.width; 
        ci.height          = swapchainExtent.height; 
        ci.layers          = 1; 
        vkCreateFramebuffer(device, &ci, nullptr, &framebuffers[i]); 
    } 
} 
