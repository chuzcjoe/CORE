#include <gtest/gtest.h>

#include <iostream>

#define NS_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#include "Foundation/Foundation.hpp"
#include "Metal/Metal.hpp"
#include "QuartzCore/QuartzCore.hpp"

namespace core {
namespace test {

TEST(Metal, test) {
  MTL::Device* metalDevice = MTL::CreateSystemDefaultDevice();
  if (metalDevice == nullptr) {
    std::cout << "Metal is not supported on this system.\n";
  } else {
    std::cout << "Metal device name: " << metalDevice->name()->utf8String() << "\n";
  }
  EXPECT_EQ(metalDevice != nullptr, true);
}
}  // namespace test
}  // namespace core