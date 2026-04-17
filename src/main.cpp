#include "config.h"
#include "NetworkClient.h"

// -------------------- ARM CONSTANTS --------------------
float L1 = 0.4f;
float L2 = 0.4f;

constexpr float PI = 3.14159265f;
constexpr float SERVO_CENTER = 150.0f;

// Servo limits
bool limitEnabled = false;
constexpr float SERVO_MIN_DEG = 10.0f;
constexpr float SERVO_MAX_DEG = 290.0f;

// -------------------- STATE --------------------
float theta1 = glm::radians(90.0f); // Start UP
float theta2 = 0.0f;
float baseRotation = 0.0f;

float targetX = 0.0f;
float targetY = 0.0f;
float lastMouseX = 0.0f;

// -------------------- SHADERS --------------------
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
uniform mat4 uMVP;
void main()
{
    gl_Position = uMVP * vec4(aPos, 1.0);
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;
void main()
{
    FragColor = vec4(0.8, 0.8, 0.9, 1.0);
}
)";

// -------------------- CALLBACKS --------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void key_callback(GLFWwindow* window,
                  int key,
                  int scancode,
                  int action,
                  int mods)
{
    if (key == GLFW_KEY_L && action == GLFW_PRESS)
    {
        limitEnabled = !limitEnabled;
        std::cout << (limitEnabled ? "Limits ON (10°-290°)\n" : "Limits OFF (0°-300°)\n");
    }
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    bool ctrlPressed =
        glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;

    bool altPressed =
        glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS;

    bool shiftPressed =
        glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;

    float precision = shiftPressed ? 0.25f : 1.0f;

    // ---------------- CTRL = IK ----------------
    if (ctrlPressed)
    {
        int width, height;
        glfwGetWindowSize(window, &width, &height);

        targetX = (xpos / width) * 2.0f - 1.0f;
        targetY = -((ypos / height) * 2.0f - 1.0f);

        float dist = sqrt(targetX * targetX + targetY * targetY);
        float maxReach = L1 + L2 - 0.0001f;

        if (dist > maxReach)
        {
            targetX *= maxReach / dist;
            targetY *= maxReach / dist;
        }

        float cosTheta2 =
            (targetX*targetX + targetY*targetY - L1*L1 - L2*L2)
            / (2.0f * L1 * L2);

        cosTheta2 = glm::clamp(cosTheta2, -1.0f, 1.0f);
        float newTheta2 = acos(cosTheta2);

        float k1 = L1 + L2 * cos(newTheta2);
        float k2 = L2 * sin(newTheta2);

        float newTheta1 =
            atan2(targetY, targetX) - atan2(k2, k1);

        // Apply smoothing
        constexpr float SMOOTH_FACTOR = 0.15f;
        theta1 += SMOOTH_FACTOR * precision * (newTheta1 - theta1);
        theta2 += SMOOTH_FACTOR * precision * (newTheta2 - theta2);
    }

    // ---------------- ALT = Base rotation ----------------
    if (altPressed)
    {
        float deltaX = xpos - lastMouseX;
        baseRotation += 0.005f * precision * deltaX;
    }

    lastMouseX = (float)xpos;
}

// -------------------- MAIN --------------------
int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window =
        glfwCreateWindow(800, 600, "IK Arm 3D", NULL, NULL);

    if (!window) return -1;

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        return -1;

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetKeyCallback(window, key_callback);

    glEnable(GL_DEPTH_TEST);

    NetworkClient net;
    net.connectToServer("192.168.137.65", 5000); // Using IP from first version

    float vertices[] =
    {
        0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f
    };

    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER,
                 sizeof(vertices),
                 vertices,
                 GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT,
                          GL_FALSE,
                          3*sizeof(float),
                          (void*)0);
    glEnableVertexAttribArray(0);

    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    int mvpLoc = glGetUniformLocation(shaderProgram, "uMVP");

    while (!glfwWindowShouldClose(window))
    {
        glClearColor(0.08f, 0.08f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);

        glm::mat4 projection =
            glm::perspective(glm::radians(45.0f),
                             800.0f/600.0f,
                             0.1f,
                             100.0f);

        glm::mat4 view =
            glm::translate(glm::mat4(1.0f),
                           glm::vec3(0.0f, 0.0f, -3.0f));

        // LINK 1
        glm::mat4 model1 = glm::mat4(1.0f);
        model1 = glm::rotate(model1, baseRotation, glm::vec3(0,0,1));
        model1 = glm::rotate(model1, theta1, glm::vec3(0,0,1));
        model1 = glm::scale(model1, glm::vec3(L1,1,1));

        glm::mat4 mvp1 = projection * view * model1;
        glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvp1));
        glDrawArrays(GL_LINES, 0, 2);

        // LINK 2
        glm::mat4 model2 = glm::mat4(1.0f);
        model2 = glm::rotate(model2, baseRotation, glm::vec3(0,0,1));
        model2 = glm::rotate(model2, theta1, glm::vec3(0,0,1));
        model2 = glm::translate(model2, glm::vec3(L1,0,0));
        model2 = glm::rotate(model2, theta2, glm::vec3(0,0,1));
        model2 = glm::scale(model2, glm::vec3(L2,1,1));

        glm::mat4 mvp2 = projection * view * model2;
        glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvp2));
        glDrawArrays(GL_LINES, 0, 2);

        // ---------------- SEND ONLY WHILE HOLDING SPACE ----------------
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        {
            static float lastDeg1 = -1.0f;
            static float lastDeg2 = -1.0f;

            // Convert radians → 0–300° system
            float deg1 = theta1 * 180.0f / PI + SERVO_CENTER;
            float deg2 = theta2 * 180.0f / PI + SERVO_CENTER;

            // Always clamp to physical 0–300 range
            deg1 = glm::clamp(deg1, 0.0f, 300.0f);
            deg2 = glm::clamp(deg2, 0.0f, 300.0f);

            if (limitEnabled)
            {
                deg1 = glm::clamp(deg1, SERVO_MIN_DEG, SERVO_MAX_DEG);
                deg2 = glm::clamp(deg2, SERVO_MIN_DEG, SERVO_MAX_DEG);

                // Convert back to radians so VISUAL matches motor
                theta1 = glm::radians(deg1 - SERVO_CENTER);
                theta2 = glm::radians(deg2 - SERVO_CENTER);
            }

            // Only send if angle actually changed (prevents spam)
            if (fabs(deg1 - lastDeg1) > 0.5f ||
                fabs(deg2 - lastDeg2) > 0.5f)
            {
                net.sendAngles(deg1, deg2);
                lastDeg1 = deg1;
                lastDeg2 = deg2;
            }
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    net.closeConnection();
    glfwTerminate();
    return 0;
}