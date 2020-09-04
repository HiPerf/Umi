#include "gl_safe.hpp"
#include "containers/pooled_static_vector.hpp"
#include "containers/dictionary.hpp"
#include "entity/entity.hpp"
#include "entity/scheme.hpp"
#include "fiber/exclusive_work_stealing.hpp"
#include "gx/camera/camera.hpp"
#include "gx/mesh/mesh.hpp"
#include "gx/shader/program.hpp"
#include "updater/executor.hpp"
#include "updater/updater.hpp"

#include <boost/fiber/mutex.hpp>
#include <boost/fiber/condition_variable.hpp>

#include <tao/tuple/tuple.hpp>

#include <glm/gtx/string_cast.hpp>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>


int xc = 5;

class X : public entity<X>
{
public:
    void construct()
    {}

    void update()
    {
        //std::cout << "XXX Update at " << std::this_thread::get_id() << std::endl;
    }
};

class Y : public entity<Y>
{
public:
    void construct()
    {}

    void update()
    {
        //std::cout << "YYY Update at " << std::this_thread::get_id() << std::endl;
    }
};


constexpr uint32_t prereserved_size = 256;
constexpr uint32_t alloc_num = 1000;


//float vertices[] = {
//    0.5f,  0.5f, 0.0f,  // top right
//    0.5f, -0.5f, 0.0f,  // bottom right
//    -0.5f, -0.5f, 0.0f,  // bottom left
//    -0.5f,  0.5f, 0.0f   // top left 
//};
//unsigned int indices[] = {  // note that we start from 0!
//    0, 1, 3,  // first Triangle
//    1, 2, 3   // second Triangle
//};

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}
 
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

uint64_t CAMERA_ID = 0;

static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    auto ticket = store<entity<camera>>::get(CAMERA_ID);
    if (ticket && ticket->valid())
    {
        float z = 0;
        glReadPixels(xpos, 600 - ypos, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &z);

        float x = xpos / 800;
        float y = (600 - ypos) / 600;
        glm::vec4 screen = glm::vec4(x, y, 1.0f, 1.0f);

        auto cam = reinterpret_cast<camera*>(ticket->get());
        auto mvp = cam->mvp();
        auto proj = glm::inverse(mvp) * (screen * 2.0f - 1.0f);
        // proj = proj / proj.w;

        auto dir = proj - glm::vec4(cam->obs(), 1.0);
        cam->look_towards(glm::normalize(glm::vec3(dir)));
    }
}


class quick_test
{
    template <typename T, uint32_t S=prereserved_size> using vec = pooled_static_vector<T, entity<T>, S>;
    template <typename T, uint32_t S=prereserved_size> using dic = dictionary<T, entity<T>, S>;

    std::vector<glm::vec3> vertices;
    std::vector<int> indices;

public:
    quick_test() :
        store(),
        obj_scheme(obj_scheme.make(store)),
        camera_scheme(camera_scheme.make(store))
    {
        glfwSetErrorCallback(error_callback);
        if (!glfwInit())
        {
            exit(EXIT_FAILURE);
        }
    
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    
        _window = glfwCreateWindow(800, 600, "Simple example", NULL, NULL);
        if (!_window)
        {
            glfwTerminate();
            exit(EXIT_FAILURE);
        }
    
        glfwSetKeyCallback(_window, key_callback);
        glfwSetCursorPosCallback(_window, cursor_position_callback);

        glfwMakeContextCurrent(_window);
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        {
            glfwTerminate();
            exit(EXIT_FAILURE);
        }
        
        glfwSwapInterval(0);

        glViewport(0, 0, 800, 600);
    
        // NOTE: OpenGL error checks have been omitted for brevity
    
        _program.create();
        _program.attach(GL_VERTEX_SHADER, "resources/sample.vs");
        _program.attach(GL_FRAGMENT_SHADER, "resources/sample.fs");
        _program.bind_attribute("vCol", 1);
        _program.bind_attribute("vPos", 2);
        _program.link();
        _program.attribute_pointer("vPos", 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*) 0);
        _program.uniform_pointer("transform", program::uniform_type::transform, (void*)nullptr);

        GL_SAFE(glEnable, GL_DEPTH_TEST);
        GL_SAFE(glDepthMask, GL_TRUE);
        GL_SAFE(glDepthFunc, GL_LESS);
        GL_SAFE(glDepthRange, 0.0f, 1.0f);
        GL_SAFE(glEnable, GL_CULL_FACE);
        GL_SAFE(glFrontFace, GL_CCW);
        GL_SAFE(glCullFace, GL_BACK);
    }

    void run()
    {
        auto executor = new ::executor(12, true);
        auto overlap_scheme = overlap(store, obj_scheme, camera_scheme);
        auto updater = overlap_scheme.make_updater(true);

        executor->create_with_callback(camera_scheme, [](auto camera){
                CAMERA_ID = camera->id();
                std::cout << "HABEMUS CAMERA" << std::endl;
                return tao::tuple(camera);
            }, 
            camera_scheme.args<camera>(glm::vec3(20, 20, 20))
        );

        geometry_provider::icosahedron(vertices, indices);

        for (int i = 0; i < 2; ++i)
            geometry_provider::subdivide(vertices, indices, true);

        /// normalize vectors to "inflate" the icosahedron into a sphere.
        for (int i = 0; i < vertices.size(); i++)
            vertices[i] = glm::normalize(vertices[i]);

        executor->create_with_callback(obj_scheme,
            [](auto x, auto y, auto transform, auto mesh)
            {
                transform->local_scale({10, 10, 10});
                return tao::tuple(x, y, transform, mesh);
            },
            obj_scheme.args<X>(),
            obj_scheme.args<Y>(),
            obj_scheme.args<transform>(-10.0f, -10.0f),
            obj_scheme.args<mesh>((float*)vertices.data(), (std::size_t)(vertices.size() * sizeof(float) * 3), (int*)indices.data(), (std::size_t)(indices.size() * sizeof(int)), std::initializer_list<program*> { &_program })
        );

        executor->create(obj_scheme,
            obj_scheme.args<X>(),
            obj_scheme.args<Y>(),
            obj_scheme.args<transform>(-2.0f, 0.0f),
            obj_scheme.args<mesh>((float*)vertices.data(), (std::size_t)(vertices.size() * sizeof(float) * 3), (int*)indices.data(), (std::size_t)(indices.size() * sizeof(int)), std::initializer_list<program*> { &_program })
        );

        auto id = executor->create_with_callback(obj_scheme,
            [](auto x, auto y, auto transform, auto mesh)
            {
                transform->local_scale({ 2, 2, 2 });
                return tao::tuple(x, y, transform, mesh);
            },
            obj_scheme.args<X>(),
            obj_scheme.args<Y>(),
            obj_scheme.args<transform>(-1.0f, -4.0f),
            obj_scheme.args<mesh>((float*)vertices.data(), (std::size_t)(vertices.size() * sizeof(float) * 3), (int*)indices.data(), (std::size_t)(indices.size() * sizeof(int)), std::initializer_list<program*> { &_program })
        );

        executor->execute_tasks();

        while (!glfwWindowShouldClose(_window))
        {
            float ratio;
            int width, height;

            glfwGetFramebufferSize(_window, &width, &height);
            ratio = width / (float)height;

            GL_SAFE(glViewport, 0, 0, width, height);
            GL_SAFE(glClearColor, 0.2f, 0.3f, 0.3f, 1.0f);
            GL_SAFE(glClearDepth, 1.0f);
            GL_SAFE(glClear, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            //std::cout << "----------------" << std::endl;
            executor->update(updater);
            executor->execute_tasks();
            executor->sync(updater);

            glfwSwapBuffers(_window);
            glfwPollEvents();
        }
 
        glfwDestroyWindow(_window);
        glfwTerminate();

        executor->stop();
        delete executor;
    }

private:
    // Rendering
    GLFWwindow* _window;
    program _program;

    // Prebuilt schemes
    scheme_store<dic<X>, dic<Y>, dic<transform>, dic<mesh>, dic<camera, 1>> store;
    decltype(scheme<X, Y, transform, mesh>::make(store)) obj_scheme;
    decltype(scheme<camera>::make(store)) camera_scheme;
};


int main()
{
    static_assert(std::is_trivially_copyable_v<tao::tuple<int>>, "NOT TRIVIAL");

    quick_test test;
    test.run();

    return 0;
}
