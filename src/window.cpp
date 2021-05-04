#include "window.h"
#include "log.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <fmt/format.h>

#include <cstdlib>
#include <cstdint>

namespace alc {

window::window(const std::string& name, int width, int height)
    : d_native_window(nullptr)
    , d_data({name, width, height, true, true, true})
{
    if (GLFW_TRUE != glfwInit()) {
		log::fatal("Failed to initialise GLFW\n");
		std::exit(-1);
	}

	d_native_window = glfwCreateWindow(
		width, height, name.c_str(), nullptr, nullptr
	);

	if (!d_native_window) {
		log::fatal("Failed to create window\n");
		std::exit(-2);
	}

	glfwMakeContextCurrent(d_native_window);
    glfwSetWindowUserPointer(d_native_window, &d_data);

	// Initialise GLAD
	if (0 == gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		log::fatal("Failed to initialise GLAD\n");
		std::exit(-3);
	}

    
	int versionMajor;
	int versionMinor;
	glGetIntegerv(GL_MAJOR_VERSION, &versionMajor);
	glGetIntegerv(GL_MINOR_VERSION, &versionMinor);
	log::info("OpenGL version: {}.{}", versionMajor, versionMinor);

	// Set OpenGL error callback
	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback([](GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void*) {
		
		switch (severity) {
			case GL_DEBUG_SEVERITY_NOTIFICATION: return;
			case GL_DEBUG_SEVERITY_LOW: {
				log::info("{}, {}, {}, {}, {}", source, type, id, length, message);
			} break;
			case GL_DEBUG_SEVERITY_MEDIUM: {
				log::warn("{}, {}, {}, {}, {}", source, type, id, length, message);
			} break;
			case GL_DEBUG_SEVERITY_HIGH: {
				log::error("{}, {}, {}, {}, {}", source, type, id, length, message);
			} break;
		}
	}, nullptr);

	// Set GLFW callbacks
	glfwSetWindowSizeCallback(d_native_window, [](GLFWwindow* window, int width, int height)
	{
		glViewport(0, 0, width, height);
		window_data* data = (window_data*)glfwGetWindowUserPointer(window);
		if (!data->focused) return;
		auto event = alc::make_event<alc::window_resize_event>(width, height);
		data->width = width;
		data->height = height;
		data->callback(event);
	});

	glfwSetWindowCloseCallback(d_native_window, [](GLFWwindow* window)
	{
		window_data* data = (window_data*)glfwGetWindowUserPointer(window);
		if (!data->focused) return;
		auto event = alc::make_event<alc::window_closed_event>();
		data->running = false;
		data->callback(event);
	});

	glfwSetKeyCallback(d_native_window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
		window_data* data = (window_data*)glfwGetWindowUserPointer(window);
		if (!data->focused) return;
		switch (action)
		{
			case GLFW_PRESS: {
				auto event = alc::make_event<alc::keyboard_pressed_event>(key, scancode, mods);
				data->callback(event);
			} break;
			case GLFW_RELEASE: {
				auto event = alc::make_event<alc::keyboard_released_event>(key, scancode, mods);
				data->callback(event);
			} break;
			case GLFW_REPEAT: {
				auto event = alc::make_event<alc::keyboard_held_event>(key, scancode, mods);
				data->callback(event);
			} break;
		}
	});

	glfwSetMouseButtonCallback(d_native_window, [](GLFWwindow* window, int button, int action, int mods) {
		window_data* data = (window_data*)glfwGetWindowUserPointer(window);
		if (!data->focused) return;
		switch (action)
		{
		case GLFW_PRESS: {
			auto event = alc::make_event<alc::mouse_pressed_event>(button, action, mods);
			data->callback(event);
		} break;
		case GLFW_RELEASE: {
			auto event = alc::make_event<alc::mouse_released_event>(button, action, mods);
			data->callback(event);
		} break;
		}
	});

	glfwSetCursorPosCallback(d_native_window, [](GLFWwindow* window, double x_pos, double y_pos) {
		window_data* data = (window_data*)glfwGetWindowUserPointer(window);
		if (!data->focused) return;
		auto event = alc::make_event<alc::mouse_moved_event>(x_pos, y_pos);
		data->callback(event);
	});

	glfwSetScrollCallback(d_native_window, [](GLFWwindow* window, double x_offset, double y_offset) {
		window_data* data = (window_data*)glfwGetWindowUserPointer(window);
		if (!data->focused) return;
		auto event = alc::make_event<alc::mouse_scrolled_event>(x_offset, y_offset);
		data->callback(event);
	});

	glfwSetWindowFocusCallback(d_native_window, [](GLFWwindow* window, int focused) {
		window_data* data = (window_data*)glfwGetWindowUserPointer(window);
		if (focused) {
			auto event = alc::make_event<alc::window_got_focus_event>();
			data->focused = true;
			data->callback(event);
		}
		else {
			auto event = alc::make_event<alc::window_lost_focus_event>();
			data->focused = false;
			data->callback(event);
		}
	});

	glfwSetWindowMaximizeCallback(d_native_window, [](GLFWwindow* window, int maximized) {
		window_data* data = (window_data*)glfwGetWindowUserPointer(window);
		if (maximized) {
			auto event = alc::make_event<alc::window_maximise_event>();
			data->callback(event);
		}
		else {
			auto event = alc::make_event<alc::window_minimise_event>();
			data->callback(event);
		}
	});

	glfwSetCharCallback(d_native_window, [](GLFWwindow* window, std::uint32_t key) {
		window_data* data = (window_data*)glfwGetWindowUserPointer(window);
		if (!data->focused) return;
		auto event = alc::make_event<alc::keyboard_typed_event>(key);
		data->callback(event);
	});
}

window::~window()
{
    glfwDestroyWindow(d_native_window);
	glfwTerminate();
}

void window::on_update(double dt)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glClearColor(0.0, 0.0, 0.0, 1.0);

    glfwSwapBuffers(d_native_window);
    glfwPollEvents();
}

bool window::is_running() const
{
    return d_data.running;
}

glm::vec2 window::get_mouse_pos() const
{
    double x, y;
    glfwGetCursorPos(d_native_window, &x, &y);
    return {x, y};
}

void window::set_name(const std::string& name)
{
    glfwSetWindowTitle(d_native_window, name.c_str());
}

void window::set_callback(const callback_t& callback)
{
    d_data.callback = callback;
}

}