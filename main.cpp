#include "stdafx.h"


const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

class VulkanHelloWorld {

public:

    void Run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:

    void initWindow() {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        window = glfwCreateWindow(WIDTH, HEIGHT, "VulkanHelloWorld", nullptr, nullptr);
    }

    void initVulkan() {
        createInstance();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapchain();
    }

    void mainLoop() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
        }
    }

    void cleanup() {
        vkDestroySwapchainKHR(device, swapchain, nullptr);
        vkDestroyDevice(device, nullptr);
        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyInstance(instance, nullptr);

        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void createInstance() {
        // 应用程序信息
        VkApplicationInfo appInfo{};            // 结构体
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Hello Triangle";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        // 实例创建信息
        VkInstanceCreateInfo createInfo = {};   // 结构体
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        // 扩展 - 和窗体系统交互
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        createInfo.enabledExtensionCount = glfwExtensionCount;
        createInfo.ppEnabledExtensionNames = glfwExtensions;
        // 全局校验层
        createInfo.enabledLayerCount = 0;

        // 创建实例
        // 实例创建信息
        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
            throw std::runtime_error("Failed to create instance!");
    }

    void createSurface() {
        if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
            throw std::runtime_error("Failed to create window surface!");
    }

    void pickPhysicalDevice() {
        // 获取设备数量
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
        if (deviceCount == 0)
            throw std::runtime_error("Failed to find GPUs with Vulkan support!");

        // 获取所有可用的设备
        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        // 选取一个合适的设备
        for (const auto& device : devices) {
            // 查询物理设备属性
            VkPhysicalDeviceProperties deviceProperties;
            vkGetPhysicalDeviceProperties(device, &deviceProperties);
            // 选择集显
            bool integratedGraphicsCard = deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;
            // 查询合适的队列族
            std::optional<uint32_t> queueFamily = findQueueFamily(device);
            bool queueFamilySupported = queueFamily.has_value();
            // 查询交换链支持
            std::string swapchainExtensionName = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
            uint32_t extensionCount;
            vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
            std::vector<VkExtensionProperties> availableExtensions(extensionCount);
            vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
            bool swapchainExtensionSupported = false;
            for (auto properties : availableExtensions)
                if (properties.extensionName == swapchainExtensionName)
                    swapchainExtensionSupported = true;
            bool swapchainSupported = false;
            if (swapchainExtensionSupported) {
                uint32_t formatCount;
                uint32_t presentModeCount;
                vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
                vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
                swapchainSupported = formatCount && presentModeCount;
            }
            // 选择具有合适的队列族且支持交换链的集显
            if (integratedGraphicsCard && queueFamilySupported && swapchainSupported) {
                physicalDevice = device;
                break;
            }
        }
        if (physicalDevice == VK_NULL_HANDLE)
            throw std::runtime_error("Failed to find a suitable GPU!");
    }

    // 获取一个同时支持 图形命令 且支持我们的 窗体表面 的队列族
    // 注意到uint32_t的任何取值都有可能是一个真实存在的队列族索引
    // 因此我们使用std::optional来处理获取失败的情况
    std::optional<uint32_t> findQueueFamily(VkPhysicalDevice device) {
        // 获取所有的队列族
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
        // 选取队列族
        for (int i = 0; i < queueFamilies.size(); i++) {
            // 支持表面（展示）
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
            // 支持图形命令
            VkBool32 graphicsSupport = queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT;
            if (graphicsSupport && presentSupport)
                return i;
        }
        return std::optional<uint32_t>();
    }

    void createLogicalDevice() {
        uint32_t queueFamilyIndex = findQueueFamily(physicalDevice).value();

        // 创建逻辑设备，需要一个队列创建信息
        // 结构体，描述了对于一个队列族我们需要创建的队列数量以及各自的优先级
        VkDeviceQueueCreateInfo queueCreateInfo = {};                           
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
        queueCreateInfo.queueCount = 1;
        float queuePriority = 1.0f;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        // 还需要物理设备特性s
        // 同样也是通过结构体传入
        VkPhysicalDeviceFeatures deviceFeatures = {};
        VkDeviceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.queueCreateInfoCount = 1;
        createInfo.pQueueCreateInfos = &queueCreateInfo;
        createInfo.pEnabledFeatures = &deviceFeatures;
        const std::vector<const char*> extensionNames = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensionNames.size());
        createInfo.ppEnabledExtensionNames = extensionNames.data(); // 启用支持交换链的拓展
        if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS)    // 创建逻辑设备
            throw std::runtime_error("failed to create logical device");
        // 在创建逻辑设备的同时，会根据传入的队列创建信息创建相应队列
        // 我们取回这个队列的句柄
        vkGetDeviceQueue(device, queueFamilyIndex, 0, &graphicsAndPresentQueue);    //逻辑设备、队列族索引、队列索引、用于存储队列句柄的变量的指针
    }

    void createSwapchain() {
        // 选取表面格式
        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
        /*
        typedef struct VkSurfaceFormatKHR {
            VkFormat           format;      // 颜色通道和类型
            VkColorSpaceKHR    colorSpace;  // 颜色空间  是否支持SRGB的
        } VkSurfaceFormatKHR;
        */
        std::vector<VkSurfaceFormatKHR> formats(formatCount);
        // 查询显卡支持的表面格式
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats.data());
        VkSurfaceFormatKHR formatChosen = formats[0];
        for (const auto& availableFormat : formats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) { // RGBA颜色格式 && SRGB颜色空间 
                formatChosen = availableFormat;
                break;
            }
        }

        // 选取展示模式
        VkPresentModeKHR presentModeChosen = VK_PRESENT_MODE_FIFO_KHR;

        // 选择交换范围
        // 通过 VkSurfaceCapabilitiesKHR 获取图像宽高
        /*
         * typedef struct VkSurfaceCapabilitiesKHR {
                uint32_t                         minImageCount;
                uint32_t                         maxImageCount;
                VkExtent2D                       currentExtent;
                VkExtent2D                       minImageExtent;
                VkExtent2D                       maxImageExtent;
                uint32_t                         maxImageArrayLayers;
                VkSurfaceTransformFlagsKHR       supportedTransforms;
                VkSurfaceTransformFlagBitsKHR    currentTransform;
                VkCompositeAlphaFlagsKHR         supportedCompositeAlpha;
                VkImageUsageFlags                supportedUsageFlags;
        } VkSurfaceCapabilitiesKHR;
        */
        // 其中， VkExtent2D表示当前窗口分辨率
        VkSurfaceCapabilitiesKHR capabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &capabilities);
        VkExtent2D actualExtent = capabilities.currentExtent;
        // 有的窗体管理器可能不太一样，它们允许我们指定不同的值。这种情况下，表示当前窗体分辨率的成员currentExtent中的高和宽都是uint32_t的最大值，我们需要通过minImageExtent和maxImageExtent来选择一个合适的交换范围。???
        if (capabilities.currentExtent.width == UINT32_MAX) {
            actualExtent = { WIDTH, HEIGHT };
            actualExtent.width = max(capabilities.minImageExtent.width, min(capabilities.maxImageExtent.width, actualExtent.width));
            actualExtent.height = max(capabilities.minImageExtent.height, min(capabilities.maxImageExtent.height, actualExtent.height));
        }

        // 创建交换链
        // 定义一个结构体
        VkSwapchainCreateInfoKHR createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;
        createInfo.minImageCount = capabilities.minImageCount;
        createInfo.imageFormat = formatChosen.format;
        createInfo.imageColorSpace = formatChosen.colorSpace;
        createInfo.imageExtent = actualExtent;
        createInfo.imageArrayLayers = 1;                                // 如果不创建三维应用
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;    // 直接渲染到图像上，作为颜色附着
        // 共享模式指定了图像在不同队列之间传递规则，比如说如果我们的图形队列跟展示队列不是同一个队列，我们就需要通过图形队列在交换链中的图像上渲染，再在展示队列上提交它们，这就需要图像在不同队列中共享
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;        // 指定颜色的共享模式
        createInfo.preTransform = capabilities.currentTransform;        // 指定交换链中图形的变换
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;  // Alpha通道不用于跟窗体系统中的其他窗体进行混合
        createInfo.presentMode = presentModeChosen;                     
        createInfo.clipped = VK_TRUE;                                   // 指定剪裁为否，即不考虑被遮挡的窗体像素颜色
        createInfo.oldSwapchain = VK_NULL_HANDLE;                       // oldSwapChain: 当交换链被废弃的时候（例如窗体尺寸改变），老的交换链在这里传递给新的交换链。此处不做设置，仅有一个交换链

        if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapchain)) // 创建交换链
            throw std::runtime_error("failed to create swapchain!");

        uint32_t imageCount;
        vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
        swapchainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data());

        // 保存交换链选取的格式和范围
        swapchainImageFormat = formatChosen.format;
        swapchainExtent = actualExtent;
    }


private:
    GLFWwindow* window;

    // Vulkan实例
    VkInstance instance;

    // 窗体表面实例
    VkSurfaceKHR surface;

    // 物理设备对象
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

    // 逻辑设备对象
    VkDevice device;

    VkQueue graphicsAndPresentQueue;

    // 交换链
    VkSwapchainKHR swapchain;

    // (取回)交换链中图像的句柄数组
    std::vector<VkImage> swapchainImages;

    // 交换链选取的格式和范围
    VkFormat swapchainImageFormat;
    VkExtent2D swapchainExtent;
};


int main() {
    VulkanHelloWorld helloWorld;
    helloWorld.Run();
    return 0;
}
