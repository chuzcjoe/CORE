#define VULKAN_HPP_NO_EXCEPTIONS
#include <assert.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <vector>

#include "Status.h"

// #if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
// #	include <vulkan/vulkan_raii.hpp>
// #else
// import vulkan_hpp;
// #endif

#include <vulkan/vulkan_raii.hpp>

#define GLFW_INCLUDE_VULKAN  // REQUIRED only for GLFW CreateWindowSurface.
#include <GLFW/glfw3.h>

constexpr uint32_t WIDTH = 800;
constexpr uint32_t HEIGHT = 600;
constexpr int MAX_FRAMES_IN_FLIGHT = 2;

const std::vector<char const *> validationLayers = {"VK_LAYER_KHRONOS_validation"};

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif

core::Status BackendError(vk::Result result) {
  return core::Status(core::ErrorCode::kBackend, vk::to_string(result),
                      "Vulkan", static_cast<std::int64_t>(result));
}

class HelloTriangleApplication {
 public:
  ~HelloTriangleApplication() { cleanup(); }

  core::Status run() {
    {
      const core::Status status = initWindow();
      if (!status.ok()) {
        return status;
      }
    }
    {
      const core::Status status = initVulkan();
      if (!status.ok()) {
        return status;
      }
    }
    {
      const core::Status status = mainLoop();
      if (!status.ok()) {
        return status;
      }
    }
    return core::Status();
  }

 private:
  GLFWwindow *window = nullptr;
  vk::raii::Context context;
  vk::raii::Instance instance = nullptr;
  vk::raii::DebugUtilsMessengerEXT debugMessenger = nullptr;
  vk::raii::SurfaceKHR surface = nullptr;
  vk::raii::PhysicalDevice physicalDevice = nullptr;
  vk::raii::Device device = nullptr;
  uint32_t queueIndex = std::numeric_limits<uint32_t>::max();
  vk::raii::Queue queue = nullptr;
  vk::raii::SwapchainKHR swapChain = nullptr;
  std::vector<vk::Image> swapChainImages;
  vk::SurfaceFormatKHR swapChainSurfaceFormat;
  vk::Extent2D swapChainExtent;
  std::vector<vk::raii::ImageView> swapChainImageViews;

  vk::raii::PipelineLayout pipelineLayout = nullptr;
  vk::raii::Pipeline graphicsPipeline = nullptr;

  vk::raii::CommandPool commandPool = nullptr;
  std::vector<vk::raii::CommandBuffer> commandBuffers;

  std::vector<vk::raii::Semaphore> presentCompleteSemaphores;
  std::vector<vk::raii::Semaphore> renderFinishedSemaphores;
  std::vector<vk::raii::Fence> inFlightFences;
  uint32_t frameIndex = 0;

#ifdef __APPLE__
  std::vector<const char *> requiredDeviceExtension = {vk::KHRSwapchainExtensionName};
#else
  std::vector<const char *> requiredDeviceExtension = {
      vk::KHRSwapchainExtensionName, vk::KHRSpirv14ExtensionName,
      vk::KHRSynchronization2ExtensionName, vk::KHRCreateRenderpass2ExtensionName};
#endif

  core::Status initWindow() {
    if (glfwInit() != GLFW_TRUE) {
      return core::Status(core::ErrorCode::kBackend,
                          "GLFW initialization failed");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    if (window == nullptr) {
      return core::Status(core::ErrorCode::kBackend, "Window creation failed");
    }

    return core::Status();
  }

  core::Status initVulkan() {
    {
      const core::Status status = createInstance();
      if (!status.ok()) {
        return status;
      }
    }
    {
      const core::Status status = setupDebugMessenger();
      if (!status.ok()) {
        return status;
      }
    }
    {
      const core::Status status = createSurface();
      if (!status.ok()) {
        return status;
      }
    }
    {
      const core::Status status = pickPhysicalDevice();
      if (!status.ok()) {
        return status;
      }
    }
    {
      const core::Status status = createLogicalDevice();
      if (!status.ok()) {
        return status;
      }
    }
    {
      const core::Status status = createSwapChain();
      if (!status.ok()) {
        return status;
      }
    }
    {
      const core::Status status = createImageViews();
      if (!status.ok()) {
        return status;
      }
    }
    {
      const core::Status status = createGraphicsPipeline();
      if (!status.ok()) {
        return status;
      }
    }
    {
      const core::Status status = createCommandPool();
      if (!status.ok()) {
        return status;
      }
    }
    {
      const core::Status status = createCommandBuffers();
      if (!status.ok()) {
        return status;
      }
    }
    {
      const core::Status status = createSyncObjects();
      if (!status.ok()) {
        return status;
      }
    }

    return core::Status();
  }

  core::Status mainLoop() {
    while (!glfwWindowShouldClose(window)) {
      glfwPollEvents();
      {
        const core::Status status = drawFrame();
        if (!status.ok()) {
          return status;
        }
      }
    }

    {
      const vk::Result result = device.waitIdle();
      if (result != vk::Result::eSuccess) {
        return BackendError(result);
      }
    }

    return core::Status();
  }

  void cleanup() {
    if (*device) {
      static_cast<void>(device.waitIdle());
    }
    // Release GPU resources while the surface and window are still alive.
    inFlightFences.clear();
    renderFinishedSemaphores.clear();
    presentCompleteSemaphores.clear();
    commandBuffers.clear();
    commandPool.clear();
    graphicsPipeline.clear();
    pipelineLayout.clear();
    swapChainImageViews.clear();
    swapChain.clear();
    device.clear();
    surface.clear();
    glfwDestroyWindow(window);

    glfwTerminate();
  }

  core::Status createInstance() {
    uint32_t supportedVersion = 0;
    const auto version = context.enumerateInstanceVersion();
    supportedVersion = version.result == vk::Result::eSuccess
                           ? version.value
                           : VK_API_VERSION_1_0;

    uint32_t desiredVersion = VK_API_VERSION_1_3;
    uint32_t apiVersion = std::min(supportedVersion, desiredVersion);
    if (apiVersion == 0) {
      apiVersion = VK_API_VERSION_1_0;
    }

    vk::ApplicationInfo appInfo{.pApplicationName = "Hello Triangle",
                                .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
                                .pEngineName = "No Engine",
                                .engineVersion = VK_MAKE_VERSION(1, 0, 0),
                                .apiVersion = apiVersion};

    // Get the required layers
    std::vector<char const *> requiredLayers;
    if (enableValidationLayers) {
      requiredLayers.assign(validationLayers.begin(), validationLayers.end());
    }

    // Check if the required layers are supported by the Vulkan implementation.

    auto layerProperties_result = context.enumerateInstanceLayerProperties();
    if (layerProperties_result.result != vk::Result::eSuccess) {
      return BackendError(layerProperties_result.result);
    }
    auto layerProperties = std::move(layerProperties_result.value);
    for (auto const &requiredLayer : requiredLayers) {
      if (std::ranges::none_of(layerProperties, [requiredLayer](auto const &layerProperty) {
            return strcmp(layerProperty.layerName, requiredLayer) == 0;
          })) {
        return core::Status(
            core::ErrorCode::kBackend,
            "Required layer not supported: " + std::string(requiredLayer));
      }
    }

    // Get the required extensions.
    auto requiredExtensions = getRequiredExtensions();

    // Check if the required extensions are supported by the Vulkan implementation.

    auto extensionProperties_result =
        context.enumerateInstanceExtensionProperties();
    if (extensionProperties_result.result != vk::Result::eSuccess) {
      return BackendError(extensionProperties_result.result);
    }
    auto extensionProperties = std::move(extensionProperties_result.value);
    for (auto const &requiredExtension : requiredExtensions) {
      if (std::ranges::none_of(
              extensionProperties, [requiredExtension](auto const &extensionProperty) {
                return strcmp(extensionProperty.extensionName, requiredExtension) == 0;
              })) {
        return core::Status(core::ErrorCode::kBackend,
                            "Required extension not supported: " +
                                std::string(requiredExtension));
      }
    }

    vk::InstanceCreateInfo createInfo{
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = static_cast<uint32_t>(requiredLayers.size()),
        .ppEnabledLayerNames = requiredLayers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size()),
        .ppEnabledExtensionNames = requiredExtensions.data()};
#ifdef __APPLE__
    bool hasPortability = std::ranges::any_of(requiredExtensions, [](auto const *ext) {
      return std::strcmp(ext, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME) == 0;
    });
    if (!hasPortability) {
      return core::Status(
          core::ErrorCode::kBackend,
          "VK_KHR_portability_enumeration must be enabled on macOS");
    }
    createInfo.flags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
#endif

    auto instance_result = context.createInstance(createInfo);
    if (instance_result.result != vk::Result::eSuccess) {
      return BackendError(instance_result.result);
    }
    instance = std::move(instance_result.value);

    return core::Status();
  }

  core::Status setupDebugMessenger() {
    if (!enableValidationLayers) {
      return core::Status();
    }

    vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
    vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
        vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
        vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
    vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT{
        .messageSeverity = severityFlags,
        .messageType = messageTypeFlags,
        .pfnUserCallback = &debugCallback};

    auto debugMessenger_result =
        instance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT);
    if (debugMessenger_result.result != vk::Result::eSuccess) {
      return BackendError(debugMessenger_result.result);
    }
    debugMessenger = std::move(debugMessenger_result.value);

    return core::Status();
  }

  core::Status createSurface() {
    VkSurfaceKHR _surface;
    if (glfwCreateWindowSurface(*instance, window, nullptr, &_surface) != 0) {
      return core::Status(core::ErrorCode::kBackend,
                          "failed to create window surface!");
    }
    surface = vk::raii::SurfaceKHR(instance, _surface);

    return core::Status();
  }

  core::Status pickPhysicalDevice() {
    auto devices_result = instance.enumeratePhysicalDevices();
    if (devices_result.result != vk::Result::eSuccess) {
      return BackendError(devices_result.result);
    }
    auto devices = std::move(devices_result.value);
    for (const auto& candidate : devices) {
      const auto families = candidate.getQueueFamilyProperties();
      const bool graphics =
          std::ranges::any_of(families, [](const auto& family) {
            return static_cast<bool>(family.queueFlags &
                                     vk::QueueFlagBits::eGraphics);
          });
      auto extensions = candidate.enumerateDeviceExtensionProperties();
      if (extensions.result != vk::Result::eSuccess) {
        return BackendError(extensions.result);
      }
      const bool supported = std::ranges::all_of(
          requiredDeviceExtension, [&](const char* required) {
            return std::ranges::any_of(
                extensions.value, [&](const auto& available) {
                  return std::strcmp(available.extensionName, required) == 0;
                });
          });
#ifdef __APPLE__
      const bool version_supported =
          candidate.getProperties().apiVersion >= VK_API_VERSION_1_0;
      const bool features_supported = true;
#else
      const bool version_supported =
          candidate.getProperties().apiVersion >= VK_API_VERSION_1_3;
      const auto features = candidate.getFeatures2<
          vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan11Features,
          vk::PhysicalDeviceVulkan13Features,
          vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();
      const bool features_supported =
          features.get<vk::PhysicalDeviceVulkan11Features>()
              .shaderDrawParameters &&
          features.get<vk::PhysicalDeviceVulkan13Features>().synchronization2 &&
          features.get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
          features.get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>()
              .extendedDynamicState;
#endif
      if (graphics && supported && version_supported && features_supported) {
        physicalDevice = candidate;
        return core::Status();
      }
    }
    return core::Status(core::ErrorCode::kUnsupported,
                        "No suitable Vulkan GPU");
  }

  core::Status createLogicalDevice() {
    std::vector<vk::QueueFamilyProperties> queueFamilyProperties =
        physicalDevice.getQueueFamilyProperties();

    // get the first index into queueFamilyProperties which supports both graphics and present
    for (uint32_t qfpIndex = 0; qfpIndex < queueFamilyProperties.size(); qfpIndex++) {
      const auto support =
          physicalDevice.getSurfaceSupportKHR(qfpIndex, *surface);
      if (support.result != vk::Result::eSuccess) {
        return BackendError(support.result);
      }
      if ((queueFamilyProperties[qfpIndex].queueFlags &
           vk::QueueFlagBits::eGraphics) &&
          support.value) {
        // found a queue family that supports both graphics and present
        queueIndex = qfpIndex;
        break;
      }
    }
    if (queueIndex == std::numeric_limits<uint32_t>::max()) {
      return core::Status(
          core::ErrorCode::kBackend,
          "Could not find a queue for graphics and present -> terminating");
    }

    // query for Vulkan 1.3 features
    vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan11Features,
                       vk::PhysicalDeviceVulkan13Features,
                       vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>
        featureChain = {
            {},                              // vk::PhysicalDeviceFeatures2
            {.shaderDrawParameters = true},  // vk::PhysicalDeviceVulkan11Features
            {.synchronization2 = true,
             .dynamicRendering = true},     // vk::PhysicalDeviceVulkan13Features
            {.extendedDynamicState = true}  // vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT
        };

    // create a Device
    float queuePriority = 0.5f;
    vk::DeviceQueueCreateInfo deviceQueueCreateInfo{
        .queueFamilyIndex = queueIndex, .queueCount = 1, .pQueuePriorities = &queuePriority};
    vk::DeviceCreateInfo deviceCreateInfo{
        .pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &deviceQueueCreateInfo,
        .enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtension.size()),
        .ppEnabledExtensionNames = requiredDeviceExtension.data()};

    auto device_result = physicalDevice.createDevice(deviceCreateInfo);
    if (device_result.result != vk::Result::eSuccess) {
      return BackendError(device_result.result);
    }
    device = std::move(device_result.value);
    queue = device.getQueue(queueIndex, 0);

    return core::Status();
  }

  core::Status createSwapChain() {
    auto surfaceCapabilities_result =
        physicalDevice.getSurfaceCapabilitiesKHR(*surface);
    if (surfaceCapabilities_result.result != vk::Result::eSuccess) {
      return BackendError(surfaceCapabilities_result.result);
    }
    auto surfaceCapabilities = std::move(surfaceCapabilities_result.value);
    swapChainExtent = chooseSwapExtent(surfaceCapabilities);
    auto formats = physicalDevice.getSurfaceFormatsKHR(*surface);
    if (formats.result != vk::Result::eSuccess) {
      return BackendError(formats.result);
    }
    if (formats.value.empty()) {
      return core::Status(core::ErrorCode::kUnsupported, "No surface formats");
    }
    swapChainSurfaceFormat = chooseSwapSurfaceFormat(formats.value);
    auto modes = physicalDevice.getSurfacePresentModesKHR(*surface);
    if (modes.result != vk::Result::eSuccess) {
      return BackendError(modes.result);
    }
    vk::SwapchainCreateInfoKHR swapChainCreateInfo{
        .surface = *surface,
        .minImageCount = chooseSwapMinImageCount(surfaceCapabilities),
        .imageFormat = swapChainSurfaceFormat.format,
        .imageColorSpace = swapChainSurfaceFormat.colorSpace,
        .imageExtent = swapChainExtent,
        .imageArrayLayers = 1,
        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
        .imageSharingMode = vk::SharingMode::eExclusive,
        .preTransform = surfaceCapabilities.currentTransform,
        .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode = chooseSwapPresentMode(modes.value),
        .clipped = true};

    auto swapChain_result = device.createSwapchainKHR(swapChainCreateInfo);
    if (swapChain_result.result != vk::Result::eSuccess) {
      return BackendError(swapChain_result.result);
    }
    swapChain = std::move(swapChain_result.value);

    auto swapChainImages_result = swapChain.getImages();
    if (swapChainImages_result.result != vk::Result::eSuccess) {
      return BackendError(swapChainImages_result.result);
    }
    swapChainImages = std::move(swapChainImages_result.value);

    return core::Status();
  }

  core::Status createImageViews() {
    assert(swapChainImageViews.empty());

    vk::ImageViewCreateInfo imageViewCreateInfo{
        .viewType = vk::ImageViewType::e2D,
        .format = swapChainSurfaceFormat.format,
        .subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}};
    for (auto &image : swapChainImages) {
      imageViewCreateInfo.image = image;
      auto view_result = device.createImageView(imageViewCreateInfo);
      if (view_result.result != vk::Result::eSuccess) {
        return BackendError(view_result.result);
      }
      swapChainImageViews.push_back(std::move(view_result.value));
    }

    return core::Status();
  }

  core::Status createGraphicsPipeline() {
    auto code = readFile("./vulkan_tutorial/shaders/slang.spv");
    if (!code.ok()) {
      return code.status();
    }
    auto shader = createShaderModule(*code);
    if (!shader.ok()) {
      return shader.status();
    }
    vk::raii::ShaderModule shaderModule = std::move(*shader);

    vk::PipelineShaderStageCreateInfo vertShaderStageInfo{
        .stage = vk::ShaderStageFlagBits::eVertex, .module = shaderModule, .pName = "vertMain"};
    vk::PipelineShaderStageCreateInfo fragShaderStageInfo{
        .stage = vk::ShaderStageFlagBits::eFragment, .module = shaderModule, .pName = "fragMain"};
    vk::PipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
    vk::PipelineInputAssemblyStateCreateInfo inputAssembly{
        .topology = vk::PrimitiveTopology::eTriangleList};
    vk::PipelineViewportStateCreateInfo viewportState{.viewportCount = 1, .scissorCount = 1};

    vk::PipelineRasterizationStateCreateInfo rasterizer{.depthClampEnable = vk::False,
                                                        .rasterizerDiscardEnable = vk::False,
                                                        .polygonMode = vk::PolygonMode::eFill,
                                                        .cullMode = vk::CullModeFlagBits::eBack,
                                                        .frontFace = vk::FrontFace::eClockwise,
                                                        .depthBiasEnable = vk::False,
                                                        .depthBiasSlopeFactor = 1.0f,
                                                        .lineWidth = 1.0f};

    vk::PipelineMultisampleStateCreateInfo multisampling{
        .rasterizationSamples = vk::SampleCountFlagBits::e1, .sampleShadingEnable = vk::False};

    vk::PipelineColorBlendAttachmentState colorBlendAttachment{
        .blendEnable = vk::False,
        .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                          vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA};

    vk::PipelineColorBlendStateCreateInfo colorBlending{.logicOpEnable = vk::False,
                                                        .logicOp = vk::LogicOp::eCopy,
                                                        .attachmentCount = 1,
                                                        .pAttachments = &colorBlendAttachment};

    std::vector dynamicStates = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
    vk::PipelineDynamicStateCreateInfo dynamicState{
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data()};

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{.setLayoutCount = 0,
                                                    .pushConstantRangeCount = 0};

    auto pipelineLayout_result =
        device.createPipelineLayout(pipelineLayoutInfo);
    if (pipelineLayout_result.result != vk::Result::eSuccess) {
      return BackendError(pipelineLayout_result.result);
    }
    pipelineLayout = std::move(pipelineLayout_result.value);

    vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfo>
        pipelineCreateInfoChain = {
            {.stageCount = 2,
             .pStages = shaderStages,
             .pVertexInputState = &vertexInputInfo,
             .pInputAssemblyState = &inputAssembly,
             .pViewportState = &viewportState,
             .pRasterizationState = &rasterizer,
             .pMultisampleState = &multisampling,
             .pColorBlendState = &colorBlending,
             .pDynamicState = &dynamicState,
             .layout = pipelineLayout,
             .renderPass = nullptr},
            {.colorAttachmentCount = 1, .pColorAttachmentFormats = &swapChainSurfaceFormat.format}};

    auto graphicsPipeline_result = device.createGraphicsPipeline(
        nullptr, pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>());
    if (graphicsPipeline_result.result != vk::Result::eSuccess) {
      return BackendError(graphicsPipeline_result.result);
    }
    graphicsPipeline = std::move(graphicsPipeline_result.value);

    return core::Status();
  }

  core::Status createCommandPool() {
    vk::CommandPoolCreateInfo poolInfo{.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
                                       .queueFamilyIndex = queueIndex};

    auto commandPool_result = device.createCommandPool(poolInfo);
    if (commandPool_result.result != vk::Result::eSuccess) {
      return BackendError(commandPool_result.result);
    }
    commandPool = std::move(commandPool_result.value);

    return core::Status();
  }

  core::Status createCommandBuffers() {
    vk::CommandBufferAllocateInfo allocInfo{.commandPool = commandPool,
                                            .level = vk::CommandBufferLevel::ePrimary,
                                            .commandBufferCount = MAX_FRAMES_IN_FLIGHT};

    auto commandBuffers_result = device.allocateCommandBuffers(allocInfo);
    if (commandBuffers_result.result != vk::Result::eSuccess) {
      return BackendError(commandBuffers_result.result);
    }
    commandBuffers = std::move(commandBuffers_result.value);

    return core::Status();
  }

  core::Status recordCommandBuffer(uint32_t imageIndex) {
    auto &commandBuffer = commandBuffers[frameIndex];
    {
      const vk::Result result = commandBuffer.begin({});
      if (result != vk::Result::eSuccess) {
        return BackendError(result);
      }
    }
    // Before starting rendering, transition the swapchain image to COLOR_ATTACHMENT_OPTIMAL
    transition_image_layout(imageIndex, vk::ImageLayout::eUndefined,
                            vk::ImageLayout::eColorAttachmentOptimal,
                            {},  // srcAccessMask (no need to wait for previous operations)
                            vk::AccessFlagBits2::eColorAttachmentWrite,          // dstAccessMask
                            vk::PipelineStageFlagBits2::eColorAttachmentOutput,  // srcStage
                            vk::PipelineStageFlagBits2::eColorAttachmentOutput   // dstStage
    );
    vk::ClearValue clearColor = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
    vk::RenderingAttachmentInfo attachmentInfo = {
        .imageView = swapChainImageViews[imageIndex],
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = clearColor};
    vk::RenderingInfo renderingInfo = {.renderArea = {.offset = {0, 0}, .extent = swapChainExtent},
                                       .layerCount = 1,
                                       .colorAttachmentCount = 1,
                                       .pColorAttachments = &attachmentInfo};
    commandBuffer.beginRendering(renderingInfo);
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *graphicsPipeline);
    commandBuffer.setViewport(0,
                              vk::Viewport{0.0f, 0.0f, static_cast<float>(swapChainExtent.width),
                                           static_cast<float>(swapChainExtent.height), 0.0f, 1.0f});
    commandBuffer.setScissor(0, vk::Rect2D{vk::Offset2D{0, 0}, swapChainExtent});
    commandBuffer.draw(3, 1, 0, 0);
    commandBuffer.endRendering();
    // After rendering, transition the swapchain image to PRESENT_SRC
    transition_image_layout(imageIndex, vk::ImageLayout::eColorAttachmentOptimal,
                            vk::ImageLayout::ePresentSrcKHR,
                            vk::AccessFlagBits2::eColorAttachmentWrite,          // srcAccessMask
                            {},                                                  // dstAccessMask
                            vk::PipelineStageFlagBits2::eColorAttachmentOutput,  // srcStage
                            vk::PipelineStageFlagBits2::eBottomOfPipe            // dstStage
    );
    {
      const vk::Result result = commandBuffer.end();
      if (result != vk::Result::eSuccess) {
        return BackendError(result);
      }
    }

    return core::Status();
  }

  void transition_image_layout(uint32_t imageIndex, vk::ImageLayout old_layout,
                               vk::ImageLayout new_layout, vk::AccessFlags2 src_access_mask,
                               vk::AccessFlags2 dst_access_mask,
                               vk::PipelineStageFlags2 src_stage_mask,
                               vk::PipelineStageFlags2 dst_stage_mask) {
    vk::ImageMemoryBarrier2 barrier = {
        .srcStageMask = src_stage_mask,
        .srcAccessMask = src_access_mask,
        .dstStageMask = dst_stage_mask,
        .dstAccessMask = dst_access_mask,
        .oldLayout = old_layout,
        .newLayout = new_layout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = swapChainImages[imageIndex],
        .subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eColor,
                             .baseMipLevel = 0,
                             .levelCount = 1,
                             .baseArrayLayer = 0,
                             .layerCount = 1}};
    vk::DependencyInfo dependency_info = {
        .dependencyFlags = {}, .imageMemoryBarrierCount = 1, .pImageMemoryBarriers = &barrier};
    commandBuffers[frameIndex].pipelineBarrier2(dependency_info);
  }

  core::Status createSyncObjects() {
    assert(presentCompleteSemaphores.empty() && renderFinishedSemaphores.empty() &&
           inFlightFences.empty());

    for (size_t i = 0; i < swapChainImages.size(); i++) {
      auto semaphore_result = device.createSemaphore(vk::SemaphoreCreateInfo());
      if (semaphore_result.result != vk::Result::eSuccess) {
        return BackendError(semaphore_result.result);
      }
      renderFinishedSemaphores.push_back(std::move(semaphore_result.value));
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
      auto semaphore_result = device.createSemaphore(vk::SemaphoreCreateInfo());
      if (semaphore_result.result != vk::Result::eSuccess) {
        return BackendError(semaphore_result.result);
      }
      presentCompleteSemaphores.push_back(std::move(semaphore_result.value));
      auto fence_result = device.createFence(
          vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled});
      if (fence_result.result != vk::Result::eSuccess) {
        return BackendError(fence_result.result);
      }
      inFlightFences.push_back(std::move(fence_result.value));
    }

    return core::Status();
  }

  core::Status drawFrame() {
    // Note: inFlightFences, presentCompleteSemaphores, and commandBuffers are indexed by
    // frameIndex,
    //       while renderFinishedSemaphores is indexed by imageIndex
    auto fenceResult = device.waitForFences(*inFlightFences[frameIndex], vk::True, UINT64_MAX);
    if (fenceResult != vk::Result::eSuccess) {
      return core::Status(core::ErrorCode::kBackend,
                          "failed to wait for fence!");
    }
    {
      const vk::Result result = device.resetFences(*inFlightFences[frameIndex]);
      if (result != vk::Result::eSuccess) {
        return BackendError(result);
      }
    }

    auto [result, imageIndex] =
        swapChain.acquireNextImage(UINT64_MAX, *presentCompleteSemaphores[frameIndex], nullptr);

    if (result != vk::Result::eSuccess &&
        result != vk::Result::eSuboptimalKHR) {
      return BackendError(result);
    }
    const vk::Result reset_result = commandBuffers[frameIndex].reset();
    if (reset_result != vk::Result::eSuccess) {
      return BackendError(reset_result);
    }
    {
      const core::Status status = recordCommandBuffer(imageIndex);
      if (!status.ok()) {
        return status;
      }
    }

    vk::PipelineStageFlags waitDestinationStageMask(
        vk::PipelineStageFlagBits::eColorAttachmentOutput);
    const vk::SubmitInfo submitInfo{.waitSemaphoreCount = 1,
                                    .pWaitSemaphores = &*presentCompleteSemaphores[frameIndex],
                                    .pWaitDstStageMask = &waitDestinationStageMask,
                                    .commandBufferCount = 1,
                                    .pCommandBuffers = &*commandBuffers[frameIndex],
                                    .signalSemaphoreCount = 1,
                                    .pSignalSemaphores = &*renderFinishedSemaphores[imageIndex]};
    {
      const vk::Result result =
          queue.submit(submitInfo, *inFlightFences[frameIndex]);
      if (result != vk::Result::eSuccess) {
        return BackendError(result);
      }
    }

    const vk::PresentInfoKHR presentInfoKHR{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &*renderFinishedSemaphores[imageIndex],
        .swapchainCount = 1,
        .pSwapchains = &*swapChain,
        .pImageIndices = &imageIndex};
    result = queue.presentKHR(presentInfoKHR);
    switch (result) {
      case vk::Result::eSuccess:
        break;
      case vk::Result::eSuboptimalKHR:
        std::cout << "vk::Queue::presentKHR returned vk::Result::eSuboptimalKHR !\n";
        break;
      default:
        return BackendError(result);
    }
    frameIndex = (frameIndex + 1) % MAX_FRAMES_IN_FLIGHT;

    return core::Status();
  }

  [[nodiscard]] core::Result<vk::raii::ShaderModule> createShaderModule(
      const std::vector<char>& code) const {
    vk::ShaderModuleCreateInfo createInfo{.codeSize = code.size() * sizeof(char),
                                          .pCode = reinterpret_cast<const uint32_t *>(code.data())};
    auto result = device.createShaderModule(createInfo);
    if (result.result != vk::Result::eSuccess) {
      return core::Result<vk::raii::ShaderModule>(BackendError(result.result));
    }
    return core::Result<vk::raii::ShaderModule>(std::move(result.value));
  }

  static uint32_t chooseSwapMinImageCount(vk::SurfaceCapabilitiesKHR const &surfaceCapabilities) {
    auto minImageCount = std::max(3u, surfaceCapabilities.minImageCount);
    if ((0 < surfaceCapabilities.maxImageCount) &&
        (surfaceCapabilities.maxImageCount < minImageCount)) {
      minImageCount = surfaceCapabilities.maxImageCount;
    }
    return minImageCount;
  }

  static vk::SurfaceFormatKHR chooseSwapSurfaceFormat(
      const std::vector<vk::SurfaceFormatKHR> &availableFormats) {
    assert(!availableFormats.empty());
    const auto formatIt = std::ranges::find_if(availableFormats, [](const auto &format) {
      return format.format == vk::Format::eB8G8R8A8Srgb &&
             format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
    });
    return formatIt != availableFormats.end() ? *formatIt : availableFormats[0];
  }

  static vk::PresentModeKHR chooseSwapPresentMode(
      const std::vector<vk::PresentModeKHR> &availablePresentModes) {
    assert(std::ranges::any_of(availablePresentModes, [](auto presentMode) {
      return presentMode == vk::PresentModeKHR::eFifo;
    }));
    return std::ranges::any_of(
               availablePresentModes,
               [](const vk::PresentModeKHR value) { return vk::PresentModeKHR::eMailbox == value; })
               ? vk::PresentModeKHR::eMailbox
               : vk::PresentModeKHR::eFifo;
  }

  vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR &capabilities) {
    if (capabilities.currentExtent.width != 0xFFFFFFFF) {
      return capabilities.currentExtent;
    }
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    return {std::clamp<uint32_t>(width, capabilities.minImageExtent.width,
                                 capabilities.maxImageExtent.width),
            std::clamp<uint32_t>(height, capabilities.minImageExtent.height,
                                 capabilities.maxImageExtent.height)};
  }

  std::vector<const char *> getRequiredExtensions() {
    uint32_t glfwExtensionCount = 0;
    auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    if (enableValidationLayers) {
      extensions.push_back(vk::EXTDebugUtilsExtensionName);
    }

#ifdef __APPLE__
    // required by MoltenVK / portability
    extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
#endif

    return extensions;
  }

  static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(
      vk::DebugUtilsMessageSeverityFlagBitsEXT severity, vk::DebugUtilsMessageTypeFlagsEXT type,
      const vk::DebugUtilsMessengerCallbackDataEXT *pCallbackData, void *) {
    if (severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eError ||
        severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning) {
      std::cerr << "validation layer: type " << to_string(type)
                << " msg: " << pCallbackData->pMessage << std::endl;
    }

    return vk::False;
  }

  static core::Result<std::vector<char>> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
      return core::Result<std::vector<char>>(
          core::Status(core::ErrorCode::kIo, "failed to open file!"));
    }
    const std::streampos end = file.tellg();
    if (end <= 0 ||
        static_cast<std::uintmax_t>(end) >
            static_cast<std::uintmax_t>(
                std::numeric_limits<std::streamsize>::max()) ||
        (end % 4) != 0) {
      return core::Result<std::vector<char>>(
          core::Status(core::ErrorCode::kIo, "Invalid shader file size"));
    }
    std::vector<char> buffer(static_cast<size_t>(end));
    file.seekg(0, std::ios::beg);
    file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
    if (!file) {
      return core::Result<std::vector<char>>(
          core::Status(core::ErrorCode::kIo, "Failed to read shader file"));
    }
    file.close();
    return core::Result<std::vector<char>>(std::move(buffer));
  }
};

int main() {
  HelloTriangleApplication app;
  const core::Status status = app.run();
  if (!status.ok()) {
    std::cerr << status.message() << '\n';
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
