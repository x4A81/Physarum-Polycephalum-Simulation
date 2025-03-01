#include <SDL3/SDL.h>
#include <cmath>

#define NUM_AGENTS 150000
#define MOVE_SPEED 0.7f
#define WIDTH 800
#define HEIGHT 800
#define SENSOR_ANGLE (M_PI / 5.0f)
#define SENSOR_DISTANCE 8.0f
#define TURN_SPEED 0.4f
#define RANDOM_STRENGTH 0.1f
#define DECAY_RATE 0.98f

SDL_Renderer* renderer;
SDL_Window* window;
SDL_Texture* pheromoneTexture;

// Agent structure includes x position, y position, and angle
typedef struct t_agent {
    float x, y, angle;
} t_agent;

// Pheromone grid
alignas(32) float pheromones[WIDTH][HEIGHT] = {0};

// Agent array
alignas(32) t_agent agent[NUM_AGENTS];

// Initialises agents in a circle facing inwards
void init_agents() {
    const float center_x = WIDTH / 2.0f;
    const float center_y = HEIGHT / 2.0f;
    const float radius = 5;
    
    for (int current_agent = 0; current_agent < NUM_AGENTS; current_agent++) {
        float angle = static_cast<float>(current_agent) / NUM_AGENTS * 2.0f * M_PI;
        agent[current_agent].x = center_x + cos(angle) * radius;
        agent[current_agent].y = center_y + sin(angle) * radius;
        agent[current_agent].angle = atan2(center_y - agent[current_agent].y, 
                                         center_x - agent[current_agent].x);
    }
}

// Agents sense pheromones
float sense(float x, float y, float angle) {
    // Find the sensed pheromone
    int sense_x = static_cast<int>(x + cos(angle) * SENSOR_DISTANCE);
    int sense_y = static_cast<int>(y + sin(angle) * SENSOR_DISTANCE);
    sense_x = std::max(0, std::min(sense_x, WIDTH - 1));
    sense_y = std::max(0, std::min(sense_y, HEIGHT - 1));
    
    // Return the sensed pheromone
    return pheromones[sense_x][sense_y];
}

void update_agents() {
    for (int current_agent = 0; current_agent < NUM_AGENTS; current_agent++) {

        // Sense Left
        float left = sense(agent[current_agent].x, agent[current_agent].y,
             agent[current_agent].angle - SENSOR_ANGLE);
        
        // Sense Forward
        float forward = sense(agent[current_agent].x, agent[current_agent].y,
             agent[current_agent].angle);
    
        // Sense Right
        float right = sense(agent[current_agent].x, agent[current_agent].y,
             agent[current_agent].angle + SENSOR_ANGLE);

        // Add some random steering
        float random_steer = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * RANDOM_STRENGTH;

        // Turn towards the direction with the most pheromones
        if (forward > left && forward > right) {
            agent[current_agent].angle += random_steer;
        } else if (left > right) {
            agent[current_agent].angle -= TURN_SPEED + random_steer;
        } else if (right > left) {
            agent[current_agent].angle += TURN_SPEED + random_steer;
        } else {
            agent[current_agent].angle += random_steer;
        }
        
        // Move the agent
        float theta = agent[current_agent].angle;
        agent[current_agent].x += MOVE_SPEED * cos(theta);
        agent[current_agent].y += MOVE_SPEED * sin(theta);
        
        // Boundary conditions
        int turn_around = 0;
        if (agent[current_agent].x >= WIDTH) {
            agent[current_agent].x = WIDTH;
            turn_around = 1;
        }
        
        if (agent[current_agent].x <= 0) {
            agent[current_agent].x = 0;
            turn_around = 1;
        }

        if (agent[current_agent].y >= HEIGHT) {
            agent[current_agent].y = HEIGHT;
            turn_around = 1;
        }

        if (agent[current_agent].y <= 0) {
            agent[current_agent].y = 0;
            turn_around = 1;
        }

        if (turn_around) {
            agent[current_agent].angle += M_PI;
        }
        
        // Randomness
        agent[current_agent].angle += ((rand() % 3 - 1) * 0.1f);

        // Leave a pheromone trail
        int x_idx = static_cast<int>(agent[current_agent].x);
        int y_idx = static_cast<int>(agent[current_agent].y);

        x_idx = std::max(0, std::min(x_idx, WIDTH - 1));
        y_idx = std::max(0, std::min(y_idx, HEIGHT - 1));
        
        pheromones[x_idx][y_idx] = 0.9f;
    }
}

// Draws the pheromone grid to the screen
void update_pheromone_texture() {
    void* pixels;
    int pitch;
    SDL_LockTexture(pheromoneTexture, NULL, &pixels, &pitch);

    Uint32* dst = static_cast<Uint32*>(pixels);
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            float intensity = pheromones[x][y];
            
            // Convert intensity to RGB colors
            Uint8 r = static_cast<Uint8>(sin(intensity) * 255);
            Uint8 g = static_cast<Uint8>((intensity*intensity) * 255);
            Uint8 b = static_cast<Uint8>(sqrt(intensity) * 255);
            Uint8 a = static_cast<Uint8>(intensity * 255);
            
            // Pack colors into a 32-bit value
            dst[y * WIDTH + x] = (r << 24) | (g << 16) | (b << 8) | a;
            pheromones[x][y] *= DECAY_RATE;
        }
    }

    SDL_UnlockTexture(pheromoneTexture);
    SDL_RenderTexture(renderer, pheromoneTexture, NULL, NULL);
}

int main() {
    // Setup
    if (SDL_Init(SDL_INIT_VIDEO) == 0) {
        SDL_Log("Failed to initialize SDL: %s", SDL_GetError());
        return -1;
    }

    window = SDL_CreateWindow("Physarum Simulation", WIDTH, HEIGHT, SDL_WINDOW_RESIZABLE);
    if (!window) {
        SDL_Log("Failed to create window: %s", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    renderer = SDL_CreateRenderer(window, "opengl");
    if (!renderer) {
        SDL_Log("Failed to create renderer: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    pheromoneTexture = SDL_CreateTexture(renderer, 
        SDL_PIXELFORMAT_RGBA8888, 
        SDL_TEXTUREACCESS_STREAMING, 
        WIDTH, HEIGHT);
    if (!pheromoneTexture) {
        SDL_Log("Failed to create texture: %s", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    SDL_SetTextureBlendMode(pheromoneTexture, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    // Main loop
    init_agents();
    int running = 1;
    SDL_Event event;
    Uint32 lastTime = SDL_GetTicks();
    int frameCount = 0;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }

        // Update agents
        update_agents();

        // Clear the screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Draw the pheromones
        update_pheromone_texture();

        SDL_RenderPresent(renderer);
        
        // Calculate FPS
        frameCount++;
        Uint32 currentTime = SDL_GetTicks();
        if (currentTime - lastTime >= 1000) {
            float fps = frameCount / ((currentTime - lastTime) / 1000.0f);
            SDL_Log("FPS: %.2f", fps);
            frameCount = 0;
            lastTime = currentTime;
        }

    }
    
    // Cleanup
    SDL_DestroyTexture(pheromoneTexture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}