#include <gtest/gtest.h>

#include <cstdint>
#include <stdexcept>

#include "VulkanBuffer.h"
#include "VulkanContext.h"

namespace core {
namespace test {

TEST(VulkanBufferMappingTest, UnmapsMemoryWhenCallbackThrows) {
  vulkan::VulkanContext context(true, vulkan::QueueFamilyType::Compute, VK_NULL_HANDLE);
  context.Init();
  vulkan::VulkanBuffer buffer(
      &context, sizeof(uint32_t), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  const PFN_vkCreateDebugUtilsMessengerEXT create_messenger =
      reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
          vkGetInstanceProcAddr(context.instance, "vkCreateDebugUtilsMessengerEXT"));
  const PFN_vkDestroyDebugUtilsMessengerEXT destroy_messenger =
      reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
          vkGetInstanceProcAddr(context.instance, "vkDestroyDebugUtilsMessengerEXT"));
  ASSERT_NE(create_messenger, nullptr);
  ASSERT_NE(destroy_messenger, nullptr);
  bool validation_error = false;
  const VkDebugUtilsMessengerCreateInfoEXT messenger_info{
      .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
      .pNext = nullptr,
      .flags = 0,
      .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
      .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT,
      .pfnUserCallback = [](VkDebugUtilsMessageSeverityFlagBitsEXT,
                            VkDebugUtilsMessageTypeFlagsEXT,
                            const VkDebugUtilsMessengerCallbackDataEXT*, void* user_data) -> VkBool32 {
        *static_cast<bool*>(user_data) = true;
        return VK_FALSE;
      },
      .pUserData = &validation_error,
  };
  VkDebugUtilsMessengerEXT messenger = VK_NULL_HANDLE;
  VK_CHECK(create_messenger(context.instance, &messenger_info, nullptr, &messenger));

  EXPECT_THROW(buffer.MapData([](void*) { throw std::logic_error("callback failed"); }),
               std::logic_error);
  bool callback_called = false;
  EXPECT_NO_THROW(buffer.MapData([&callback_called](void* data) {
    callback_called = true;
    EXPECT_NE(data, nullptr);
  }));
  EXPECT_TRUE(callback_called);
  EXPECT_FALSE(validation_error) << "Remapping after a callback exception must be valid";

  destroy_messenger(context.instance, messenger, nullptr);
}

}  // namespace test
}  // namespace core
