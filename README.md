# CORE: Compute and Open Rendering Engine

<p align="center">
  <img src="logo/core.png" alt="CORE Logo" style="width:50%;">
</p>

# Supporting OS

- MacOS
- Android arm64-v8a

# Unit Tests
```
./run.sh [-target macos|arm64-v8a] [-test_module <name>] [-test_filter <Suite.Test>]

for example: 
1. ./run.sh -target macos -test_module vulkan -test_filter ComputeSum.test
2. ./run.sh -target arm64-v8a -test_module vulkan -test_filter ComputeSum.test
```