#include "NKWindow/NkWindow.h"
#include "NKWindow/Core/NkMain.h"

// 1. Activer les extensions de plateforme avant d'inclure Vulkan
#if defined(_WIN32) || defined(NKENTSEU_PLATFORM_WINDOWS)
    #define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <vulkan/vulkan.h>

using namespace nkentseu;

// --- Fonctions de secours pour charger le Débogueur Vulkan ---
VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

// --- Callback de débogage appelé par les Validation Layers ---
static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {
    
    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        logger.Error("[Vulkan Layer] ", pCallbackData->pMessage);
    } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        logger.Warn("[Vulkan Layer] ", pCallbackData->pMessage);
    } else {
        logger.Info("[Vulkan Layer] ", pCallbackData->pMessage);
    }
    return VK_FALSE;
}

// =============================================================================
// Point d'entrée — nkmain
// =============================================================================
int nkmain(const NkEntryState& state) {

    // -------------------------------------------------------------------------
    // A. Configuration et Création de la Fenêtre
    // -------------------------------------------------------------------------
    NkWindowConfig cfg;
    cfg.title       = "Vulkan-Moteur - Etape 1";
    cfg.width       = 1280;
    cfg.height      = 720;
    cfg.centered    = true;
    cfg.resizable   = true;
    cfg.dropEnabled = true;

    NkWindow window(cfg);
    if (!window.IsOpen()) {
        logger.Error("[Sandbox] Window creation FAILED");
        NkContextShutdown();
        return -2;
    }

    auto surf = window.GetSurfaceDesc();

    //

    // -------------------------------------------------------------------------
    // B. Configuration de l'Instance Vulkan 1.2
    // -------------------------------------------------------------------------
    VkInstance instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "WandaGEVK";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Unkeny";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_2; // Version exigée par le cours

    // Gestion des extensions requises
    NkVector<const char*> extensions;
    extensions.PushBack(VK_KHR_SURFACE_EXTENSION_NAME);

    #if defined(NKENTSEU_PLATFORM_WINDOWS) || defined(_WIN32)
        extensions.PushBack(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
    #elif defined(NKENTSEU_PLATFORM_MACOS)
        extensions.PushBack(VK_EXT_METAL_SURFACE_EXTENSION_NAME);
    #endif

    // Ajout de l'extension de débogage
    extensions.PushBack(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    // Configuration de la couche de validation
    NkVector<const char*> layers;
    layers.PushBack("VK_LAYER_KHRONOS_validation");

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = (uint32_t)extensions.Size();
    createInfo.ppEnabledExtensionNames = extensions.Data();
    createInfo.enabledLayerCount = (uint32_t)layers.Size();
    createInfo.ppEnabledLayerNames = layers.Data();

    // Configuration du débogueur temporaire pour la création/destruction de l'instance
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debugCreateInfo.pfnUserCallback = VulkanDebugCallback;
    
    createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;

    // Création finale de l'instance
    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        logger.Error("[NkVulkan] Failed to create VkInstance!");
        window.Close();
        return -1;
    }
    logger.Infof("[NkVulkan] Instance Vulkan 1.2 créée avec succès !");

    // Création du messager de débogage permanent pour le reste de la vie de l'application
    if (CreateDebugUtilsMessengerEXT(instance, &debugCreateInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
        logger.Warn("[NkVulkan] Impossible d'attacher le messager de débogage permanent.");
    } else {
        logger.Infof("[NkVulkan] Messager de débogage (Validation Layers) activé.");
    }

    // -------------------------------------------------------------------------
    // Étape 2 : Sélection du Périphérique Physique (GPU)
    // -------------------------------------------------------------------------
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        logger.Error("[NkVulkan] Aucun GPU avec le support de Vulkan n'a été trouvé !");
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        vkDestroyInstance(instance, nullptr);
        window.Close();
        return -3;
    }

    logger.Infof("[NkVulkan] Nombre de GPU(s) détecté(s) : %d", deviceCount);

    NkVector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.Data());

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    int graphicsQueueFamilyIndex = -1;

    // Parcours de tous les composants graphiques pour trouver le meilleur candidat
    for (uint32_t i = 0; i < devices.Size(); ++i) {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(devices[i], &deviceProperties);

        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceFeatures(devices[i], &deviceFeatures);

        logger.Infof("--- Inspection GPU #%d ---", i);
        logger.Infof("  Nom : %s", deviceProperties.deviceName);
        
        const char* typeStr = "Inconnu";
        if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) typeStr = "GPU Dédié (Discrete)";
        if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) typeStr = "GPU Intégré (Integrated)";
        if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU) typeStr = "CPU";
        logger.Infof("  Type : %s", typeStr);

        uint32_t major = VK_VERSION_MAJOR(deviceProperties.apiVersion);
        uint32_t minor = VK_VERSION_MINOR(deviceProperties.apiVersion);
        logger.Infof("  Version API supportée : %d.%d", major, minor);

        // Vérification des familles de files d'attente pour ce GPU
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &queueFamilyCount, nullptr);

        NkVector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &queueFamilyCount, queueFamilies.Data());

        int currentGraphicsIndex = -1;
        for (uint32_t j = 0; j < queueFamilies.Size(); ++j) {
            // On cherche une file qui supporte les opérations graphiques
            if (queueFamilies[j].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                currentGraphicsIndex = j;
                break;
            }
        }

        // Critères de sélection requis : Vulkan 1.2+ et support des commandes graphiques
        if (major > 1 || (major == 1 && minor >= 2)) {
            if (currentGraphicsIndex != -1) {
                // Priorité absolue au GPU dédié (carte NVIDIA/AMD) pour de meilleures performances
                if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && physicalDevice == VK_NULL_HANDLE) {
                    physicalDevice = devices[i];
                    graphicsQueueFamilyIndex = currentGraphicsIndex;
                    logger.Infof("  -> [Choix Optimal] Ce GPU Dédié est sélectionné.");
                }
            }
        }
    }

    // Solution de repli si aucun GPU dédié n'est disponible (ex: puce intégrée Intel/AMD sur PC portable)
    if (physicalDevice == VK_NULL_HANDLE && devices.Size() > 0) {
        for (uint32_t i = 0; i < devices.Size(); ++i) {
            uint32_t queueFamilyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &queueFamilyCount, nullptr);
            NkVector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &queueFamilyCount, queueFamilies.Data());

            for (uint32_t j = 0; j < queueFamilies.Size(); ++j) {
                if (queueFamilies[j].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                    physicalDevice = devices[i];
                    graphicsQueueFamilyIndex = j;
                    break;
                }
            }
            if (physicalDevice != VK_NULL_HANDLE) break;
        }

        if (physicalDevice != VK_NULL_HANDLE) {
            VkPhysicalDeviceProperties fallbackProps;
            vkGetPhysicalDeviceProperties(physicalDevice, &fallbackProps);
            logger.Warnf("[NkVulkan] Aucun GPU dédié trouvé. Rabattement sur : %s", fallbackProps.deviceName);
        }
    }

    if (physicalDevice == VK_NULL_HANDLE || graphicsQueueFamilyIndex == -1) {
        logger.Error("[NkVulkan] Impossible de trouver un GPU compatible avec les fonctionnalités requises !");
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        vkDestroyInstance(instance, nullptr);
        window.Close();
        return -4;
    }

    logger.Infof("[NkVulkan] GPU validé avec succès ! Index de la famille de file d'attente graphique : %d", graphicsQueueFamilyIndex); 

    // -------------------------------------------------------------------------
    // Étape 4 : Création de la Surface Vulkan native via NkSurfaceDesc
    // -------------------------------------------------------------------------
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    NkSurfaceDesc surfDesc = window.GetSurfaceDesc();

    if (!surfDesc.IsValid()) {
        logger.Error("[NkVulkan] Le descripteur de surface natif (NkSurfaceDesc) est invalide !");
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        vkDestroyInstance(instance, nullptr);
        window.Close();
        return -5;
    }

    #if defined(NKENTSEU_PLATFORM_WINDOWS) || defined(_WIN32)
        VkWin32SurfaceCreateInfoKHR surfaceCreateInfo{};
        surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        surfaceCreateInfo.hwnd = surfDesc.hwnd;
        surfaceCreateInfo.hinstance = surfDesc.hinstance;

        if (vkCreateWin32SurfaceKHR(instance, &surfaceCreateInfo, nullptr, &surface) != VK_SUCCESS) {
            logger.Error("[NkVulkan] Échec de la création de la surface Vulkan Win32 !");
            DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
            vkDestroyInstance(instance, nullptr);
            window.Close();
            return -5;
        }
    #elif defined(NKENTSEU_WINDOWING_WAYLAND)
        VkWaylandSurfaceCreateInfoKHR surfaceCreateInfo{};
        surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
        surfaceCreateInfo.display = surfDesc.display;
        surfaceCreateInfo.surface = surfDesc.surface;

        if (vkCreateWaylandSurfaceKHR(instance, &surfaceCreateInfo, nullptr, &surface) != VK_SUCCESS) {
            logger.Error("[NkVulkan] Échec de la création de la surface Vulkan Wayland !");
            DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
            vkDestroyInstance(instance, nullptr);
            window.Close();
            return -5;
        }
    #elif defined(NKENTSEU_WINDOWING_XLIB)
        VkXlibSurfaceCreateInfoKHR surfaceCreateInfo{};
        surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
        surfaceCreateInfo.dpy = surfDesc.display;
        surfaceCreateInfo.window = surfDesc.window;

        if (vkCreateXlibSurfaceKHR(instance, &surfaceCreateInfo, nullptr, &surface) != VK_SUCCESS) {
            logger.Error("[NkVulkan] Échec de la création de la surface Vulkan Xlib !");
            DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
            vkDestroyInstance(instance, nullptr);
            window.Close();
            return -5;
        }
    #elif defined(NKENTSEU_WINDOWING_XCB)
        VkXcbSurfaceCreateInfoKHR surfaceCreateInfo{};
        surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
        surfaceCreateInfo.connection = surfDesc.connection;
        surfaceCreateInfo.window = surfDesc.window;

        if (vkCreateXcbSurfaceKHR(instance, &surfaceCreateInfo, nullptr, &surface) != VK_SUCCESS) {
            logger.Error("[NkVulkan] Échec de la création de la surface Vulkan XCB !");
            DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
            vkDestroyInstance(instance, nullptr);
            window.Close();
            return -5;
        }
    #else
        #error "Plateforme non supportée pour la création de surface Vulkan explicite."
    #endif

    logger.Infof("[NkVulkan] Surface de rendu native (VkSurfaceKHR) créée avec succès !");

    // Vérification cruciale : Est-ce que la file d'attente graphique supporte la Présentation ?
    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, graphicsQueueFamilyIndex, surface, &presentSupport);
    
    if (!presentSupport) {
        logger.Error("[NkVulkan] Le GPU ou la file d'attente sélectionnée ne supporte pas la Présentation sur cette surface !");
        vkDestroySurfaceKHR(instance, surface, nullptr);
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        vkDestroyInstance(instance, nullptr);
        window.Close();
        return -6;
    }
    logger.Infof("[NkVulkan] La file d'attente supporte la Présentation sur la surface.");

    // -------------------------------------------------------------------------
    // Étape 3 : Création définitive du Périphérique Logique (VkDevice)
    // -------------------------------------------------------------------------
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE; // Souvent identique à la file graphique, mais distincte mathématiquement

    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = graphicsQueueFamilyIndex;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    // Déclaration des fonctionnalités physiques requises (ex: l'anisotropie requise pour le PBR)
    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = VK_TRUE; 

    // Liste des extensions requises au niveau du Device (Obligatoire pour la Swapchain)
    NkVector<const char*> deviceExtensions;
    deviceExtensions.PushBack(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    // Vérification matérielle que le GPU supporte bien l'extension swapchain demandée
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);
    NkVector<VkExtensionProperties> availableDeviceExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableDeviceExtensions.Data());

    bool swapchainSupported = false;
    for (const auto& ext : availableDeviceExtensions) {
        if (strcmp(ext.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) {
            swapchainSupported = true;
            break;
        }
    }

    if (!swapchainSupported) {
        logger.Error("[NkVulkan] Le GPU sélectionné ne supporte pas l'extension Swapchain (VK_KHR_SWAPCHAIN_EXTENSION_NAME) !");
        vkDestroySurfaceKHR(instance, surface, nullptr);
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        vkDestroyInstance(instance, nullptr);
        window.Close();
        return -6;
    }

    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
    deviceCreateInfo.enabledExtensionCount = (uint32_t)deviceExtensions.Size();
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.Data();

    // Compatibilité couches de validation (optionnel/déprécié sur les versions récentes de Vulkan mais conservé pour la rétrocompatibilité)
    deviceCreateInfo.enabledLayerCount = (uint32_t)layers.Size();
    deviceCreateInfo.ppEnabledLayerNames = layers.Data();

    if (vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device) != VK_SUCCESS) {
        logger.Error("[NkVulkan] Échec de la création du périphérique logique (VkDevice) !");
        vkDestroySurfaceKHR(instance, surface, nullptr);
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        vkDestroyInstance(instance, nullptr);
        window.Close();
        return -7;
    }
    logger.Infof("[NkVulkan] Périphérique logique (VkDevice) créé avec succès !");

    // Récupération des handles des files d'attente (Queues)
    vkGetDeviceQueue(device, graphicsQueueFamilyIndex, 0, &graphicsQueue);
    vkGetDeviceQueue(device, graphicsQueueFamilyIndex, 0, &presentQueue);

    // -------------------------------------------------------------------------
    // Étape 5 : Configuration et Création de la Swapchain
    // -------------------------------------------------------------------------
    
    // 1. Récupération des capacités de la surface
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities);

    // 2. Sélection du format de surface (Idéalement B8G8R8A8_SRGB avec espace de couleur SRGB_NONLINEAR)
    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
    NkVector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, surfaceFormats.Data());

    VkSurfaceFormatKHR chosenFormat = surfaceFormats[0]; // Choix par défaut
    for (const auto& availableFormat : surfaceFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && 
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            chosenFormat = availableFormat;
            break;
        }
    }

    // 3. Sélection du Mode de Présentation (Recherche de MAILBOX comme exigé par le cours)
    uint32_t presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
    NkVector<VkPresentModeKHR> presentModes(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes.Data());

    VkPresentModeKHR chosenPresentMode = VK_PRESENT_MODE_FIFO_KHR; // Mode standard garanti par Vulkan
    for (const auto& availableMode : presentModes) {
        if (availableMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            chosenPresentMode = availableMode;
            logger.Infof("[NkVulkan] Mode de présentation optimal détecté et sélectionné : MAILBOX");
            break;
        }
    }
    if (chosenPresentMode == VK_PRESENT_MODE_FIFO_KHR) {
        logger.Infof("[NkVulkan] Mode MAILBOX non disponible. Repli sur le mode garanti : FIFO (V-Sync)");
    }

    // 4. Détermination de la taille de la Swapchain (Extent)
    VkExtent2D swapchainExtent = surfaceCapabilities.currentExtent;
    if (surfaceCapabilities.currentExtent.width == UINT32_MAX) {
        // Si la plateforme nous laisse le choix, on prend les dimensions de notre NkWindow
        swapchainExtent.width = std::max(surfaceCapabilities.minImageExtent.width, std::min(surfaceCapabilities.maxImageExtent.width, (uint32_t)cfg.width));
        swapchainExtent.height = std::max(surfaceCapabilities.minImageExtent.height, std::min(surfaceCapabilities.maxImageExtent.height, (uint32_t)cfg.height));
    }

    // 5. Nombre d'images dans la Swapchain (Triple Buffering si possible)
    uint32_t imageCount = surfaceCapabilities.minImageCount + 1;
    if (surfaceCapabilities.maxImageCount > 0 && imageCount > surfaceCapabilities.maxImageCount) {
        imageCount = surfaceCapabilities.maxImageCount;
    }

    // 6. Structure de création de la Swapchain
    VkSwapchainCreateInfoKHR swapchainCreateInfo{};
    swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCreateInfo.surface = surface;
    swapchainCreateInfo.minImageCount = imageCount;
    swapchainCreateInfo.imageFormat = chosenFormat.format;
    swapchainCreateInfo.imageColorSpace = chosenFormat.colorSpace;
    swapchainCreateInfo.imageExtent = swapchainExtent;
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // On va dessiner directement dessus

    // Vu que nos index graphiques et de présentation sont les mêmes (graphicsQueueFamilyIndex)
    swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainCreateInfo.queueFamilyIndexCount = 0;
    swapchainCreateInfo.pQueueFamilyIndices = nullptr;

    swapchainCreateInfo.preTransform = surfaceCapabilities.currentTransform;
    swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainCreateInfo.presentMode = chosenPresentMode;
    swapchainCreateInfo.clipped = VK_TRUE;
    swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    if (vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &swapchain) != VK_SUCCESS) {
        logger.Error("[NkVulkan] Échec de la création de la Swapchain !");
        vkDestroyDevice(device, nullptr);
        vkDestroySurfaceKHR(instance, surface, nullptr);
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        vkDestroyInstance(instance, nullptr);
        window.Close();
        return -8;
    }
    logger.Infof("[NkVulkan] Swapchain créée avec succès (%d images, %dx%d) !", imageCount, swapchainExtent.width, swapchainExtent.height);

    // 7. Récupération des handles des images créées par la Swapchain
    uint32_t actualImageCount = 0;
    vkGetSwapchainImagesKHR(device, swapchain, &actualImageCount, nullptr);
    NkVector<VkImage> swapchainImages(actualImageCount);
    vkGetSwapchainImagesKHR(device, swapchain, &actualImageCount, swapchainImages.Data());

    // -------------------------------------------------------------------------
    // Étape 6 : Création des Image Views pour la Swapchain
    // -------------------------------------------------------------------------
    NkVector<VkImageView> swapchainImageViews(swapchainImages.Size());

    for (size_t i = 0; i < swapchainImages.Size(); i++) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = swapchainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D; // On traite des images classiques en 2D
        createInfo.format = chosenFormat.format;     // On réutilise le format sélectionné à l'étape précédente

        // Configuration du swizzling des composants (mappage des canaux de couleur par défaut)
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        // Définition de l'utilisation de l'image (Ici : cible de couleur, pas de mipmapping, 1 seule couche)
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device, &createInfo, nullptr, &swapchainImageViews[i]) != VK_SUCCESS) {
            logger.Error("[NkVulkan] Échec de la création de la VkImageView #%d !", i);
            
            // Nettoyage d'urgence des views déjà créées avant l'échec
            for (size_t j = 0; j < i; j++) {
                vkDestroyImageView(device, swapchainImageViews[j], nullptr);
            }
            vkDestroySwapchainKHR(device, swapchain, nullptr);
            vkDestroyDevice(device, nullptr);
            vkDestroySurfaceKHR(instance, surface, nullptr);
            DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
            vkDestroyInstance(instance, nullptr);
            window.Close();
            return -9;
        }
    }
    logger.Infof("[NkVulkan] %d Image Views créées avec succès !", swapchainImageViews.Size());

    // -------------------------------------------------------------------------
    // C. Boucle principale (temporaire)
    // -------------------------------------------------------------------------
    auto& eventSystem = NkEvents();
    bool running = true;
    NkChrono chrono;
    NkElapsedTime elapsed;

    while (running) {
        (void)chrono.Reset(); // Le cast (void) indique explicitement au compilateur que l'on ignore le retour

        while (NkEvent* event = eventSystem.PollEvent()) {
            if (auto wcl = event->As<NkWindowCloseEvent>()) {
                running = false;
                break;
            }
        }

        if (!running) break;

        elapsed = chrono.Elapsed();
        if (elapsed.milliseconds < 16)
            NkChrono::Sleep(16 - elapsed.milliseconds);
        else
            NkChrono::YieldThread();
    }

    // -------------------------------------------------------------------------
    // D. Nettoyage explicite (Ordre inversé)
    // -------------------------------------------------------------------------
    if (device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(device);
        
        // 1. Détruire d'abord les Image Views qui dépendent des images de la swapchain
        for (size_t i = 0; i < swapchainImageViews.Size(); i++) {
            if (swapchainImageViews[i] != VK_NULL_HANDLE) {
                vkDestroyImageView(device, swapchainImageViews[i], nullptr);
            }
        }
        logger.Infof("[NkVulkan] Image Views détruites.");

        // 2. Détruire la Swapchain qui englobe ces images
        if (swapchain != VK_NULL_HANDLE) {
            vkDestroySwapchainKHR(device, swapchain, nullptr);
            logger.Infof("[NkVulkan] Swapchain détruite.");
        }

        // 3. Détruire le périphérique logique
        vkDestroyDevice(device, nullptr);
        logger.Infof("[NkVulkan] Périphérique logique (VkDevice) détruit.");
    }

    // 4. Détruire la surface de rendu (liée à l'instance)
    if (surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(instance, surface, nullptr);
        logger.Infof("[NkVulkan] Surface de rendu (VkSurfaceKHR) détruite.");
    }

    // 5. Désactiver les Validation Layers permanents
    if (debugMessenger != VK_NULL_HANDLE) {
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        logger.Infof("[NkVulkan] Messager de débogage détruit.");
    }

    // 6. Détruire l'instance en tout dernier
    if (instance != VK_NULL_HANDLE) {
        vkDestroyInstance(instance, nullptr);
        logger.Infof("[NkVulkan] Instance Vulkan détruite proprement.");
    }

    // 7. Fermeture de l'objet de fenêtrage NKWindow
    window.Close();
    return 0;
}