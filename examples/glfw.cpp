#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <stdio.h>

#include <cstdlib>

static void key_cb(GLFWwindow* w, int key, int scancode, int action, int mods) {
  printf("scancode: %d, mods: %d\n", scancode, mods);
  if ((key == GLFW_KEY_ESCAPE || key == GLFW_KEY_Q) && action == GLFW_PRESS)
    glfwSetWindowShouldClose(w, 1);
}

int main() {
  if (!glfwInit()) return EXIT_FAILURE;

  // *** No client API (no OpenGL/ES context) ***
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  GLFWwindow* win = glfwCreateWindow(800, 600, "GLFW (no GL)", nullptr, nullptr);
  if (!win) {
    glfwTerminate();
    return EXIT_FAILURE;
  }

  glfwSetKeyCallback(win, key_cb);

  // Event loop (no rendering; compositor shows the window)
  while (!glfwWindowShouldClose(win)) {
    // If you had Vulkan/Metal/DX rendering, it would go here.
    glfwPollEvents();  // or glfwWaitEventsTimeout(0.016)
  }

  glfwDestroyWindow(win);
  glfwTerminate();
  return EXIT_SUCCESS;
}
