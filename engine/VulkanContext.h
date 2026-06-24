// La classe VulkanContext — encapsule l'initialisation Vulkan 
#include <vulkan/vulkan.h>

class VulkanContext { 
public: 
    VkInstance       instance   = VK_NULL_HANDLE; 
    VkPhysicalDevice physDevice = VK_NULL_HANDLE; 
    VkDevice         device     = VK_NULL_HANDLE; 
    VkQueue          graphicsQ  = VK_NULL_HANDLE; 
    VkQueue          presentQ   = VK_NULL_HANDLE; 
    uint32_t         graphicsFamily = 0; 
    uint32_t         presentFamily  = 0; 
 
    bool init(NkWindow* window) { 
        createInstance(window); 
        setupDebugMessenger(); 
        createSurface(window); 
        pickPhysicalDevice(); 
        createLogicalDevice(); 
        return true; 
    } 
 
private: 
    void createInstance(NkWindow* window) { 
        VkApplicationInfo appInfo{}; 
        appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO; 
        appInfo.pApplicationName   = "MoteurRenduVK"; 
        appInfo.applicationVersion = VK_MAKE_VERSION(1,0,0); 
        appInfo.pEngineName        = "WandaRngine"; 
        appInfo.apiVersion         = VK_API_VERSION_1_2; 
 
        // Extensions requises par NKWindow 
        uint32_t extCount = 0; 
        const char** winExts = window->GetVulkanInstanceExtensions(&extCount); 
        std::vector<const char*> exts(winExts, winExts + extCount); 
 
        #ifdef DEBUG 
        exts.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME); 
        #endif 

               const char* layers[] = { "VK_LAYER_KHRONOS_validation" }; 
 
        VkInstanceCreateInfo createInfo{}; 
        createInfo.sType                   = 
        VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO; 
        createInfo.pApplicationInfo         = &appInfo; 
        createInfo.enabledExtensionCount    = (uint32_t)exts.size(); 
        createInfo.ppEnabledExtensionNames  = exts.data(); 
        #ifdef DEBUG 
        createInfo.enabledLayerCount        = 1; 
        createInfo.ppEnabledLayerNames      = layers; 
        #endif 
 
        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) 
            throw std::runtime_error("vkCreateInstance failed"); 
    } 
 
    void setupDebugMessenger() { 
        #ifndef DEBUG 
        return; 
        #endif 
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT) 
            vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"); 
        if (!func) return; 
 
        VkDebugUtilsMessengerCreateInfoEXT ci{}; 
        ci.sType           = 
        VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT; 
        ci.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | 
                             VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT; 
        ci.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT; 
        ci.pfnUserCallback = [](VkDebugUtilsMessageSeverityFlagBitsEXT sev, 
            VkDebugUtilsMessageTypeFlagsEXT, const 
            VkDebugUtilsMessengerCallbackDataEXT* d, 
            void*) -> VkBool32 { 
            printf("[VK Validation] %s\n", d->pMessage); 
            return VK_FALSE; 
        }; 
        func(instance, &ci, nullptr, &debugMessenger); 
    } 
}; 

// Selection du physical device
    void pickPhysicalDevice() { 
        uint32_t count = 0; 
        vkEnumeratePhysicalDevices(instance, &count, nullptr); 
        std::vector<VkPhysicalDevice> devices(count); 
        vkEnumeratePhysicalDevices(instance, &count, devices.data()); 
    
        // Choisir le GPU avec le score le plus élevé 
        int bestScore = -1; 
        for (auto& d : devices) { 
            int score = rateDevice(d); 
            if (score > bestScore) { bestScore = score; physDevice = d; } 
        } 
        if (physDevice == VK_NULL_HANDLE) 
            throw std::runtime_error("No suitable GPU found"); 
    } 

    int rateDevice(VkPhysicalDevice dev) { 
        VkPhysicalDeviceProperties props; 
        vkGetPhysicalDeviceProperties(dev, &props); 
        int score = 0; 
        if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) score += 1000; 
        score += props.limits.maxImageDimension2D; // résolution max 
        // Vérifier que les queue families requises existent 
        if (!findQueueFamilies(dev).isComplete()) return -1; 
        return score; 
    } 
    
    struct QueueFamilyIndices { 
        int graphics = -1, present = -1; 
        bool isComplete() const { return graphics >= 0 && present >= 0; } 
    }; 
    
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice dev) { 
        QueueFamilyIndices idx; 
        uint32_t count = 0; 
        vkGetPhysicalDeviceQueueFamilyProperties(dev, &count, nullptr); 
        std::vector<VkQueueFamilyProperties> families(count); 
        vkGetPhysicalDeviceQueueFamilyProperties(dev, &count, families.data()); 
        for (uint32_t i = 0; i < count; i++) { 
            if (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) idx.graphics = i; 
            VkBool32 presentSupport = false; 
            vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, surface, &presentSupport); 
            if (presentSupport) idx.present = i; 
        } 
        return idx; 
    } 