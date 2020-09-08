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



uint64_t CAMERA_ID = 0;


constexpr uint32_t prereserved_size = 256;
constexpr uint32_t alloc_num = 1000;
constexpr float G = 6.67428e-11;

template <typename T, uint32_t S = prereserved_size> using vec = pooled_static_vector<T, entity<T>, S>;
template <typename T, uint32_t S = prereserved_size> using dic = dictionary<T, entity<T>, S>;

using std_clock_t = std::chrono::steady_clock;
using sys_clock_t = std::chrono::system_clock;
using base_time = std::chrono::milliseconds;

constexpr inline uint8_t TicksPerSecond = 40;
constexpr inline base_time HeartBeat = base_time(base_time(std::chrono::seconds(1)).count() / TicksPerSecond);

inline base_time elapsed(std_clock_t::time_point from, std_clock_t::time_point to)
{
    return std::chrono::duration_cast<base_time>(to - from);
}




class Planet : public entity<Planet>
{
public:
    void construct(float mass, glm::vec3 vel)
    {
        _mass = mass;
        _vel = vel;
    }

    void update(const base_time& diff)
    {
        auto self = get<transform>()->get<transform>();

        auto force = glm::vec3(0);
        for (auto planet : _planets->range())
        {
            if (planet == this)
            {
                continue;
            }

            auto other = planet->get<transform>()->get<transform>();
            auto delta = (other->position() - self->position()) * 1000.0f;
            auto d = glm::length(delta);

            // Rewrite to avoid overflows
            //force += delta * G * _mass * planet->_mass / pow(d, 3);
            // Mass would be divided further along
            //force += delta * (G * (_mass / d) * (planet->_mass / d)) / d;
            force += delta * (G * planet->_mass) / d / d / d;
        }

        float timestep = (float)diff.count() / 1000.0f;
        // _vel += force / _mass * timestep;
        _vel += force * timestep * 24.0f * 3600.0f;
    }

    void sync(const base_time& diff)
    {
        auto self = get<transform>()->get<transform>();
        float timestep = (float)diff.count() / 1000.0f;
        self->translate(_vel * timestep * 24.0f * 3600.0f / 1000.0f); // Keep units in KM
    }

    template <template <typename...> typename S, typename... components>
    constexpr inline void scheme_information(const S<components...>& scheme)
    {
        // Meshes require transforms
        scheme.template require<transform>();
        _planets = &scheme.template get<Planet>();
    }

private:
    float _mass;
    glm::vec3 _vel;

    dic<Planet>* _planets;
};



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

    auto ticket = store<entity<camera>>::get(CAMERA_ID);
    if (ticket && ticket->valid())
    {
        auto camera = ticket->get<::camera>();
        const float cameraSpeed = 1000.0f; // adjust accordingly
        if (key == GLFW_KEY_W && (action == GLFW_PRESS || action == GLFW_REPEAT))
        {
            camera->move(+cameraSpeed * camera->forward());
        }

        if (key == GLFW_KEY_S && (action == GLFW_PRESS || action == GLFW_REPEAT))
        {
            camera->move(-cameraSpeed * camera->forward());
        }

        if (key == GLFW_KEY_A && (action == GLFW_PRESS || action == GLFW_REPEAT))
        {
            camera->move(-glm::normalize(glm::cross(camera->forward(), camera->up())) * cameraSpeed);
        }

        if (key == GLFW_KEY_D && (action == GLFW_PRESS || action == GLFW_REPEAT))
        {
            camera->move(+glm::normalize(glm::cross(camera->forward(), camera->up())) * cameraSpeed);
        }
    }
}

float lastX = 400, lastY = 300;
float yaw = -90.0f, pitch = 0;
bool firstMouse = true;
static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    auto ticket = store<entity<camera>>::get(CAMERA_ID);
    if (ticket && ticket->valid())
    {
        // Move to mouse position
        //float z = 0;
        //glReadPixels(xpos, 600 - ypos, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &z);

        //float x = xpos / 800;
        //float y = (600 - ypos) / 600;
        //glm::vec4 screen = glm::vec4(x, y, 1.0f, 1.0f);

        //auto cam = reinterpret_cast<camera*>(ticket->get());
        //auto mvp = cam->mvp();
        //auto proj = glm::inverse(mvp) * (screen * 2.0f - 1.0f);
        //// proj = proj / proj.w;

        //auto dir = proj - glm::vec4(cam->obs(), 1.0);
        ////cam->look_towards(glm::normalize(glm::vec3(dir)));

        // Standard euler angles
        if (firstMouse)
        {
            lastX = xpos;
            lastY = ypos;
            firstMouse = false;
        }

        float xoffset = xpos - lastX;
        float yoffset = lastY - ypos; // reversed since y-coordinates range from bottom to top
        lastX = xpos;
        lastY = ypos;

        const float sensitivity = 0.1f;
        xoffset *= sensitivity;
        yoffset *= sensitivity;

        yaw += xoffset;
        pitch += yoffset;
        glm::vec3 direction;
        direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        direction.y = sin(glm::radians(pitch));
        direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

        auto camera = reinterpret_cast<::camera*>(ticket->get());
        camera->look_to(glm::normalize(direction));
    }
}


class quick_test
{
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
        glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        glfwSetCursorPosCallback(_window, cursor_position_callback);

        glfwMakeContextCurrent(_window);
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        {
            glfwTerminate();
            exit(EXIT_FAILURE);
        }
        
        glfwSwapInterval(1);

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
        auto executor = new ::executor();
        executor->start(12, true);

        auto overlap_scheme = overlap(store, obj_scheme, camera_scheme);
        auto updater = overlap_scheme.make_updater(true);

        executor->create_with_callback(camera_scheme, [](auto camera){
                CAMERA_ID = camera->id();
                std::cout << "HABEMUS CAMERA" << std::endl;
                return tao::tuple(camera);
            }, 
            camera_scheme.args<camera>(glm::vec3(1000, 1000, 1000))
        );

        geometry_provider::icosahedron(vertices, indices);

        for (int i = 0; i < 2; ++i)
            geometry_provider::subdivide(vertices, indices, true);

        /// normalize vectors to "inflate" the icosahedron into a sphere.
        for (int i = 0; i < vertices.size(); i++)
            vertices[i] = glm::normalize(vertices[i]);

        // SUN!
        executor->create_with_callback(obj_scheme,
            [](auto x, auto transform, auto mesh)
            {
                transform->local_scale({ 696340, 696340, 696340 });
                return tao::tuple(x, transform, mesh);
            },
            obj_scheme.args<Planet>(1.98892e30, glm::vec3(0)),
            obj_scheme.args<transform>(glm::vec3(0)),
            obj_scheme.args<mesh>((float*)vertices.data(), (std::size_t)(vertices.size() * sizeof(float) * 3), (int*)indices.data(), (std::size_t)(indices.size() * sizeof(int)), std::initializer_list<program*> { &_program })
        );

        // SUN2
        executor->create_with_callback(obj_scheme,
            [](auto x, auto transform, auto mesh)
            {
                transform->local_scale({ 696340, 696340, 696340 });
                return tao::tuple(x, transform, mesh);
            },
            obj_scheme.args<Planet>(1.892e27, glm::vec3(0, 0, 29.783 * 1000)),
            obj_scheme.args<transform>(glm::vec3(-76050000, 0, -76050000)),
            obj_scheme.args<mesh>((float*)vertices.data(), (std::size_t)(vertices.size() * sizeof(float) * 3), (int*)indices.data(), (std::size_t)(indices.size() * sizeof(int)), std::initializer_list<program*> { &_program })
        );

        // EARTH
        executor->create_with_callback(obj_scheme,
            [](auto x, auto transform, auto mesh)
            {
                transform->local_scale({ 696340, 696340, 696340 });
                return tao::tuple(x, transform, mesh);
            },
            obj_scheme.args<Planet>(5.9742e24, glm::vec3(0, 0, 29.783 * 1000)),
            obj_scheme.args<transform>(glm::vec3(-152.1e6, 0, 0)),
            obj_scheme.args<mesh>((float*)vertices.data(), (std::size_t)(vertices.size() * sizeof(float) * 3), (int*)indices.data(), (std::size_t)(indices.size() * sizeof(int)), std::initializer_list<program*> { &_program })
        );

        // PLANET X
        executor->create_with_callback(obj_scheme,
            [](auto x, auto transform, auto mesh)
            {
                transform->local_scale({ 696340, 696340, 696340 });
                return tao::tuple(x, transform, mesh);
            },
            obj_scheme.args<Planet>(5.9742e24, glm::vec3(-26335.7717, 0, 0)),
            obj_scheme.args<transform>(glm::vec3(-15210000, 0, 121680000)),
            obj_scheme.args<mesh>((float*)vertices.data(), (std::size_t)(vertices.size() * sizeof(float) * 3), (int*)indices.data(), (std::size_t)(indices.size() * sizeof(int)), std::initializer_list<program*> { &_program })
        );

        // VENUS
        executor->create_with_callback(obj_scheme,
            [](auto x, auto transform, auto mesh)
            {
                transform->local_scale({ 696340, 696340, 696340 });
                return tao::tuple(x, transform, mesh);
            },
            obj_scheme.args<Planet>(4.8685e24, glm::vec3(0, 0, -35.02 * 1000)),
            obj_scheme.args<transform>(glm::vec3(109968300, 0, 0)),
            obj_scheme.args<mesh>((float*)vertices.data(), (std::size_t)(vertices.size() * sizeof(float) * 3), (int*)indices.data(), (std::size_t)(indices.size() * sizeof(int)), std::initializer_list<program*> { &_program })
        );

        // Initial tasks execution
        executor->execute_tasks();

        // Initial clear
        for (int i = 0; i < 2; ++i)
        {
            int width, height;
            glfwGetFramebufferSize(_window, &width, &height);
            GL_SAFE(glViewport, 0, 0, width, height);
            GL_SAFE(glClearColor, 0.2f, 0.3f, 0.3f, 1.0f);
            GL_SAFE(glClearDepth, 1.0f);
            GL_SAFE(glClear, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glfwSwapBuffers(_window);
        }

        std_clock_t::time_point last_tick = std_clock_t::now();
        while (!glfwWindowShouldClose(_window))
        {
            auto now = std_clock_t::now();
            auto diff = elapsed(last_tick, now);

            GL_SAFE(glClearDepth, 1.0f);
            GL_SAFE(glClear, GL_DEPTH_BUFFER_BIT);

            //std::cout << "----------------" << std::endl;
            executor->update(updater, std::ref(diff));
            executor->execute_tasks();
            executor->sync(updater, std::ref(diff));
            last_tick = now;

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
    scheme_store<dic<Planet>, dic<transform>, dic<mesh>, dic<camera, 1>> store;
    decltype(scheme<Planet, transform, mesh>::make(store)) obj_scheme;
    decltype(scheme<camera>::make(store)) camera_scheme;
};


int main()
{
    static_assert(std::is_trivially_copyable_v<tao::tuple<int>>, "NOT TRIVIAL");

    quick_test test;
    test.run();

    return 0;
}
