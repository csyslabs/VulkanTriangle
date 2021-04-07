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
        // Ӧ�ó�����Ϣ
        VkApplicationInfo appInfo{};            // �ṹ��
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Hello Triangle";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        // ʵ��������Ϣ
        VkInstanceCreateInfo createInfo = {};   // �ṹ��
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        // ��չ - �ʹ���ϵͳ����
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        createInfo.enabledExtensionCount = glfwExtensionCount;
        createInfo.ppEnabledExtensionNames = glfwExtensions;
        // ȫ��У���
        createInfo.enabledLayerCount = 0;

        // ����ʵ��
        // ʵ��������Ϣ
        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
            throw std::runtime_error("Failed to create instance!");
    }

    void createSurface() {
        if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
            throw std::runtime_error("Failed to create window surface!");
    }

    void pickPhysicalDevice() {
        // ��ȡ�豸����
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
        if (deviceCount == 0)
            throw std::runtime_error("Failed to find GPUs with Vulkan support!");

        // ��ȡ���п��õ��豸
        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        // ѡȡһ�����ʵ��豸
        for (const auto& device : devices) {
            // ��ѯ�����豸����
            VkPhysicalDeviceProperties deviceProperties;
            vkGetPhysicalDeviceProperties(device, &deviceProperties);
            // ѡ����
            bool integratedGraphicsCard = deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;
            // ��ѯ���ʵĶ�����
            std::optional<uint32_t> queueFamily = findQueueFamily(device);
            bool queueFamilySupported = queueFamily.has_value();
            // ��ѯ������֧��
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
            // ѡ����к��ʵĶ�������֧�ֽ������ļ���
            if (integratedGraphicsCard && queueFamilySupported && swapchainSupported) {
                physicalDevice = device;
                break;
            }
        }
        if (physicalDevice == VK_NULL_HANDLE)
            throw std::runtime_error("Failed to find a suitable GPU!");
    }

    // ��ȡһ��ͬʱ֧�� ͼ������ ��֧�����ǵ� ������� �Ķ�����
    // ע�⵽uint32_t���κ�ȡֵ���п�����һ����ʵ���ڵĶ���������
    // �������ʹ��std::optional�������ȡʧ�ܵ����
    std::optional<uint32_t> findQueueFamily(VkPhysicalDevice device) {
        // ��ȡ���еĶ�����
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
        // ѡȡ������
        for (int i = 0; i < queueFamilies.size(); i++) {
            // ֧�ֱ��棨չʾ��
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
            // ֧��ͼ������
            VkBool32 graphicsSupport = queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT;
            if (graphicsSupport && presentSupport)
                return i;
        }
        return std::optional<uint32_t>();
    }

    void createLogicalDevice() {
        uint32_t queueFamilyIndex = findQueueFamily(physicalDevice).value();

        // �����߼��豸����Ҫһ�����д�����Ϣ
        // �ṹ�壬�����˶���һ��������������Ҫ�����Ķ��������Լ����Ե����ȼ�
        VkDeviceQueueCreateInfo queueCreateInfo = {};                           
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
        queueCreateInfo.queueCount = 1;
        float queuePriority = 1.0f;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        // ����Ҫ�����豸����s
        // ͬ��Ҳ��ͨ���ṹ�崫��
        VkPhysicalDeviceFeatures deviceFeatures = {};
        VkDeviceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.queueCreateInfoCount = 1;
        createInfo.pQueueCreateInfos = &queueCreateInfo;
        createInfo.pEnabledFeatures = &deviceFeatures;
        const std::vector<const char*> extensionNames = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensionNames.size());
        createInfo.ppEnabledExtensionNames = extensionNames.data(); // ����֧�ֽ���������չ
        if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS)    // �����߼��豸
            throw std::runtime_error("failed to create logical device");
        // �ڴ����߼��豸��ͬʱ������ݴ���Ķ��д�����Ϣ������Ӧ����
        // ����ȡ��������еľ��
        vkGetDeviceQueue(device, queueFamilyIndex, 0, &graphicsAndPresentQueue);    //�߼��豸���������������������������ڴ洢���о���ı�����ָ��
    }

    void createSwapchain() {
        // ѡȡ�����ʽ
        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
        /*
        typedef struct VkSurfaceFormatKHR {
            VkFormat           format;      // ��ɫͨ��������
            VkColorSpaceKHR    colorSpace;  // ��ɫ�ռ�  �Ƿ�֧��SRGB��
        } VkSurfaceFormatKHR;
        */
        std::vector<VkSurfaceFormatKHR> formats(formatCount);
        // ��ѯ�Կ�֧�ֵı����ʽ
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats.data());
        VkSurfaceFormatKHR formatChosen = formats[0];
        for (const auto& availableFormat : formats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) { // RGBA��ɫ��ʽ && SRGB��ɫ�ռ� 
                formatChosen = availableFormat;
                break;
            }
        }

        // ѡȡչʾģʽ
        VkPresentModeKHR presentModeChosen = VK_PRESENT_MODE_FIFO_KHR;

        // ѡ�񽻻���Χ
        // ͨ�� VkSurfaceCapabilitiesKHR ��ȡͼ����
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
        // ���У� VkExtent2D��ʾ��ǰ���ڷֱ���
        VkSurfaceCapabilitiesKHR capabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &capabilities);
        VkExtent2D actualExtent = capabilities.currentExtent;
        // �еĴ�����������ܲ�̫һ����������������ָ����ͬ��ֵ����������£���ʾ��ǰ����ֱ��ʵĳ�ԱcurrentExtent�еĸߺͿ���uint32_t�����ֵ��������Ҫͨ��minImageExtent��maxImageExtent��ѡ��һ�����ʵĽ�����Χ��???
        if (capabilities.currentExtent.width == UINT32_MAX) {
            actualExtent = { WIDTH, HEIGHT };
            actualExtent.width = max(capabilities.minImageExtent.width, min(capabilities.maxImageExtent.width, actualExtent.width));
            actualExtent.height = max(capabilities.minImageExtent.height, min(capabilities.maxImageExtent.height, actualExtent.height));
        }

        // ����������
        // ����һ���ṹ��
        VkSwapchainCreateInfoKHR createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;
        createInfo.minImageCount = capabilities.minImageCount;
        createInfo.imageFormat = formatChosen.format;
        createInfo.imageColorSpace = formatChosen.colorSpace;
        createInfo.imageExtent = actualExtent;
        createInfo.imageArrayLayers = 1;                                // �����������άӦ��
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;    // ֱ����Ⱦ��ͼ���ϣ���Ϊ��ɫ����
        // ����ģʽָ����ͼ���ڲ�ͬ����֮�䴫�ݹ��򣬱���˵������ǵ�ͼ�ζ��и�չʾ���в���ͬһ�����У����Ǿ���Ҫͨ��ͼ�ζ����ڽ������е�ͼ������Ⱦ������չʾ�������ύ���ǣ������Ҫͼ���ڲ�ͬ�����й���
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;        // ָ����ɫ�Ĺ���ģʽ
        createInfo.preTransform = capabilities.currentTransform;        // ָ����������ͼ�εı任
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;  // Alphaͨ�������ڸ�����ϵͳ�е�����������л��
        createInfo.presentMode = presentModeChosen;                     
        createInfo.clipped = VK_TRUE;                                   // ָ������Ϊ�񣬼������Ǳ��ڵ��Ĵ���������ɫ
        createInfo.oldSwapchain = VK_NULL_HANDLE;                       // oldSwapChain: ����������������ʱ�����細��ߴ�ı䣩���ϵĽ����������ﴫ�ݸ��µĽ��������˴��������ã�����һ��������

        if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapchain)) // ����������
            throw std::runtime_error("failed to create swapchain!");

        uint32_t imageCount;
        vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
        swapchainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data());

        // ���潻����ѡȡ�ĸ�ʽ�ͷ�Χ
        swapchainImageFormat = formatChosen.format;
        swapchainExtent = actualExtent;
    }


private:
    GLFWwindow* window;

    // Vulkanʵ��
    VkInstance instance;

    // �������ʵ��
    VkSurfaceKHR surface;

    // �����豸����
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

    // �߼��豸����
    VkDevice device;

    VkQueue graphicsAndPresentQueue;

    // ������
    VkSwapchainKHR swapchain;

    // (ȡ��)��������ͼ��ľ������
    std::vector<VkImage> swapchainImages;

    // ������ѡȡ�ĸ�ʽ�ͷ�Χ
    VkFormat swapchainImageFormat;
    VkExtent2D swapchainExtent;
};


int main() {
    VulkanHelloWorld helloWorld;
    helloWorld.Run();
    return 0;
}
