#define GL_SILENCE_DEPRECATION

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#define GL_GLEXT_PROTOTYPES 1
#define PI 3.14159265
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_mixer.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include <math.h>

// Instatiations
SDL_Window* displayWindow;
ShaderProgram program;

bool gameIsRunning = true;

// Dimensions of paddles
float paddle_w = 0.1f;
float paddle_h = 1.0f;
// Dimensions for (bounce) walls
float bwall_w = 10.0f;
float bwall_h = 0.0f;
// Dimensions for (empty) walls
float ewall_w = 0.0f;
float ewall_h = 7.5f;
// Dimensions for ball
float ball_w = 0.10f;
float ball_h = 0.10f;


// Matrices
glm::mat4 viewMatrix, paddle1Matrix, paddle2Matrix, ballMatrix, projectionMatrix, lineMatrix;

// Paddles
glm::vec3 paddle1_position = glm::vec3(-4.3, 0, 0);
// Don’t go anywhere (yet).
glm::vec3 paddle1_movement = glm::vec3(0, 0, 0);

glm::vec3 paddle2_position = glm::vec3(4.3, 0, 0);
// Don’t go anywhere (yet).
glm::vec3 paddle2_movement = glm::vec3(0, 0, 0);

// Ball
glm::vec3 ball_position = glm::vec3(0, 0, 0);
// Don’t go anywhere (yet).
glm::vec3 ball_movement = glm::vec3(0, 0, 0);

// Invisible walls
glm::vec3 top_wall = glm::vec3(0, 3.75, 0);
glm::vec3 bot_wall = glm::vec3(0, -3.75, 0);
glm::vec3 left_wall = glm::vec3(-5.0, 0, 0);
glm::vec3 right_wall = glm::vec3(5.0, -0, 0);


float ball_speed = 8.5f; // Speed of ball 
float MAX_BOUNCE_ANGLE = 60.0f;
float paddle_speed = 7.5f; // Speed of paddle

// Ticks
float lastTicks = 0.0f;

// Music Variables
Mix_Music* music;
Mix_Chunk* bounce;

// Box - Box Collision Detection
bool collision(glm::vec3& a, glm::vec3& b, float w1, float w2, float h1, float h2) {
    float xdist = fabs(a.x - b.x) - ((w1 + w2) / 2.0f);
    float ydist = fabs(a.y - b.y) - ((h1 + h2) / 2.0f);

    return (xdist < 0 && ydist < 0); // Colliding
       
}

// Calculate difference between y-paddle and y-ball and normalize it to find
// the angle that it bounces. The edge of the paddles should reflect ball back 30 degrees relative
// to the paddle
void paddleBounce(glm::vec3& ball_position, glm::vec3& paddle_position, glm::vec3& ball_movement) {
    float noramlizedxdist = (ball_position.y - paddle_position.y) / (paddle_h / 2);
    float bounceAngle = noramlizedxdist * MAX_BOUNCE_ANGLE;
    ball_movement.y = sin((bounceAngle * PI) / 180);
    ball_movement.x = (ball_movement.x > 0) ? -cos((bounceAngle * PI) / 180) : cos((bounceAngle * PI) / 180);
}

void endGame() {
    ball_movement = glm::vec3(0, 0, 0);
}

void startAgain() {
    ball_position = glm::vec3(0, 0, 0);
    paddle1_position = glm::vec3(-4.3, 0, 0);
    paddle2_position = glm::vec3(4.3, 0, 0);
    ball_movement = glm::vec3(0, 0, 0);
}

void Initialize() {
    SDL_Renderer* renderer = NULL;
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    displayWindow = SDL_CreateWindow("Pong", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(0, 0, 640, 480); // Tell camera is draw from (0,0) to (640,480)

    program.Load("shaders/vertex.glsl", "shaders/fragment.glsl");

    // Start Audio
    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096);
    music = Mix_LoadMUS("dooblydoo.mp3");
    Mix_PlayMusic(music, -1);
    Mix_VolumeMusic(MIX_MAX_VOLUME / 4);

    bounce = Mix_LoadWAV("bounce.wav");

    // Initialize indentity matrices and projection matrix
    viewMatrix = glm::mat4(1.0f);
    paddle1Matrix = glm::mat4(1.0f);
    paddle2Matrix = glm::mat4(1.0f);
    ballMatrix = glm::mat4(1.0f);
    lineMatrix = glm::mat4(1.0f);
    projectionMatrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    program.SetProjectionMatrix(projectionMatrix);
    program.SetViewMatrix(viewMatrix);
    program.SetColor(0.0f, 0.0f, 0.0f, 1.0f);

    glUseProgram(program.programID);

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

}

void ProcessInput() {

    // Reset paddle movements to 0 (nothing is pressed)
    paddle1_movement = glm::vec3(0); // {W, S}
    paddle2_movement = glm::vec3(0); // {UP, DOWN}

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_QUIT:
        case SDL_WINDOWEVENT_CLOSE:
            gameIsRunning = false;
            break;

        case SDL_KEYDOWN:
            switch (event.key.keysym.sym) {
            case SDLK_SPACE:
                ball_movement = glm::vec3(1, 1, 0);
                break;
            case SDLK_r:
                startAgain();
                break;
            }
            break; // SDL_KEYDOWN
        }
    }

    // Getting the keyboard state
    const Uint8* keys = SDL_GetKeyboardState(NULL);
    if (keys[SDL_SCANCODE_W] && !(collision(paddle1_position, top_wall, paddle_w, bwall_w, paddle_h, bwall_h))) {
        paddle1_movement.y = 1.0f;
    }
    else if (keys[SDL_SCANCODE_S] && !(collision(paddle1_position, bot_wall, paddle_w, bwall_w, paddle_h, bwall_h))) {
        paddle1_movement.y = -1.0f;
    }
    if (keys[SDL_SCANCODE_UP] && !(collision(paddle2_position, top_wall, paddle_w, bwall_w, paddle_h, bwall_h))) {
        paddle2_movement.y = 1.0f;
    }
    else if (keys[SDL_SCANCODE_DOWN] && !(collision(paddle2_position, bot_wall, paddle_w, bwall_w, paddle_h, bwall_h))) {
        paddle2_movement.y = -1.0f;
    }

    

}


void Update() {

    // Calculating deltatime
    float ticks = (float)SDL_GetTicks() / 1000.0f;
    float deltaTime = ticks - lastTicks;
    lastTicks = ticks;

    // paddle1 movement
    paddle1_position += paddle1_movement * paddle_speed * deltaTime;
    paddle1Matrix = glm::mat4(1.0f);
    paddle1Matrix = glm::translate(paddle1Matrix, paddle1_position);

    // paddle2 movement
    paddle2_position += paddle2_movement * paddle_speed * deltaTime;
    paddle2Matrix = glm::mat4(1.0f);
    paddle2Matrix = glm::translate(paddle2Matrix, paddle2_position);

    // ---- Collision movements ----

    // When ball collides with side walls, game ends
    if (collision(ball_position, left_wall, ball_w, ewall_w, ball_h, ewall_h) || collision(ball_position, right_wall, ball_w, ewall_w, ball_h, ewall_h)) {
        endGame();
    }

    // When ball collides with paddle1
    if (collision(ball_position, paddle1_position, ball_w, paddle_w, ball_h, paddle_h)) {
        Mix_PlayChannel(-1, bounce, 0);
        paddleBounce(ball_position, paddle1_position, ball_movement);
    }

    // When ball collides with paddle2
    if (collision(ball_position, paddle2_position, ball_w, paddle_w, ball_h, paddle_h)) {
        Mix_PlayChannel(-1, bounce, 0);
        paddleBounce(ball_position, paddle2_position, ball_movement);
    }

    // When ball collides with wall, reflects away from the wall with the same angle (negate y)
    if (collision(ball_position, top_wall, ball_w, bwall_w, ball_h, bwall_h) || collision(ball_position, bot_wall, ball_w, bwall_w, ball_h, bwall_h)) {
        Mix_PlayChannel(-1, bounce, 0);
        ball_movement.y = -ball_movement.y;
    }


    // ball movement
    ball_position += ball_movement * ball_speed * deltaTime;
    ballMatrix = glm::mat4(1.0f);
    ballMatrix = glm::translate(ballMatrix, ball_position);
}

// Rendering
void Render() {
    glClear(GL_COLOR_BUFFER_BIT);

    // Vertices for paddle
    float paddleVertices[] = { -0.05, -0.50, 0.05, -0.50, 0.05, 0.50, -0.05, -0.50, 0.05, 0.50, -0.05, 0.50 };
    glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, paddleVertices);
    glEnableVertexAttribArray(program.positionAttribute);

    // Draw paddle1
    program.SetModelMatrix(paddle1Matrix);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // Draw paddle2
    program.SetModelMatrix(paddle2Matrix);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // Vertices for ball
    float ballVertices[] = { -0.05, -0.05, 0.05, -0.05, 0.05, 0.05, -0.05, -0.05, 0.05, 0.05, -0.05, 0.05 };
    glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, ballVertices);
    glEnableVertexAttribArray(program.positionAttribute);

    // Draw ball
    program.SetModelMatrix(ballMatrix);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // Vertices for line
    float y = -3.75;
    float lineVertices[200];
    for (int i = 0; i < 200; i++) {
        if (i % 2 == 0) {
            lineVertices[i] = 0;
         }
        else {
            lineVertices[i] = y;
            y += 0.075;
        }
    }
    glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, lineVertices);
    glEnableVertexAttribArray(program.positionAttribute);

    // Draw lines
    program.SetModelMatrix(lineMatrix);
    glDrawArrays(GL_LINES, 0, 100);


    glDisableVertexAttribArray(program.positionAttribute);

    SDL_GL_SwapWindow(displayWindow);
}

void Shutdown() {
    SDL_Quit();
}

int main(int argc, char* argv[]) {
    Initialize();

    while (gameIsRunning) {
        ProcessInput();
        Update();
        Render();
    }

    Shutdown();
    return 0;
}