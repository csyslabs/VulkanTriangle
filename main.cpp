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
    }

    void mainLoop() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
        }
    }

    void cleanup() {
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
            // ֧�ֱ���
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
};


int main() {
    VulkanHelloWorld helloWorld;
    helloWorld.Run();
    return 0;
}
