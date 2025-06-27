#include <SDL2/SDL.h>
#include <SDL2/SDL_gamecontroller.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_video.h>
#include <iostream>
#include <thread>
#include <string>

int showResolutionSelection()
{
    const int optionsCount = 2;
    const int resolutions[optionsCount][2] = {
        {1280, 720},
        {1920, 1080}
    };

    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        std::cerr << "SDL_Init error: " << SDL_GetError() << std::endl;
        return -1;
    }

    if (TTF_Init() < 0)
    {
        std::cerr << "TTF_Init error: " << TTF_GetError() << std::endl;
        SDL_Quit();
        return -1;
    }

    TTF_Font* menuFont = TTF_OpenFont("DejaVuSans-Bold.ttf", 28);
    if (!menuFont)
    {
        std::cerr << "Failed to load font: " << TTF_GetError() << std::endl;
        TTF_Quit();
        SDL_Quit();
        return -1;
    }

    SDL_Window* menuWindow = SDL_CreateWindow("Select Resolution",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        600, 300, SDL_WINDOW_SHOWN);

    if (!menuWindow)
    {
        std::cerr << "Failed to create menu window: " << SDL_GetError() << std::endl;
        TTF_CloseFont(menuFont);
        TTF_Quit();
        SDL_Quit();
        return -1;
    }

    SDL_Renderer* menuRenderer = SDL_CreateRenderer(menuWindow, -1, SDL_RENDERER_ACCELERATED);
    if (!menuRenderer)
    {
        std::cerr << "Failed to create menu renderer: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(menuWindow);
        TTF_CloseFont(menuFont);
        TTF_Quit();
        SDL_Quit();
        return -1;
    }

    bool selecting = true;
    int selectedIndex = -1;

    while (selecting)
    {
        SDL_Event e;
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT)
            {
                selectedIndex = -1;
                selecting = false;
            }
            else if (e.type == SDL_KEYDOWN)
            {
                if (e.key.keysym.sym == SDLK_1) { selectedIndex = 0; selecting = false; }
                else if (e.key.keysym.sym == SDLK_2) { selectedIndex = 1; selecting = false; }
                else if (e.key.keysym.sym == SDLK_3) { selectedIndex = 2; selecting = false; }
                else if (e.key.keysym.sym == SDLK_ESCAPE)
                {
                    selectedIndex = -1;
                    selecting = false;
                }
            }
        }

        SDL_SetRenderDrawColor(menuRenderer, 0, 0, 0, 255);
        SDL_RenderClear(menuRenderer);

        for (int i = 0; i < optionsCount; i++)
        {
            std::string text = std::to_string(i + 1) + ". " + std::to_string(resolutions[i][0]) + "x" + std::to_string(resolutions[i][1]);

            SDL_Color white = { 255, 255, 255, 255 };
            SDL_Surface* surface = TTF_RenderText_Solid(menuFont, text.c_str(), white);
            SDL_Texture* texture = SDL_CreateTextureFromSurface(menuRenderer, surface);

            int texW = 0;
            int texH = 0;
            SDL_QueryTexture(texture, NULL, NULL, &texW, &texH);

            SDL_Rect dstRect = { 50, 50 + i * 60, texW, texH };
            SDL_RenderCopy(menuRenderer, texture, NULL, &dstRect);

            SDL_DestroyTexture(texture);
            SDL_FreeSurface(surface);
        }

        SDL_RenderPresent(menuRenderer);
    }

    SDL_DestroyRenderer(menuRenderer);
    SDL_DestroyWindow(menuWindow);
    TTF_CloseFont(menuFont);
    TTF_Quit();
    SDL_Quit();

    if (selectedIndex == -1)
        return -1;
    else
        return selectedIndex;
}

SDL_Window* window = nullptr;
int windowWidth = 1920;
int windowHeight = 1080;
SDL_Renderer* renderer = nullptr;
TTF_Font* font = nullptr;

static float speedX = 400.0f;
static float speedY = 300.0f;

bool isRunning = true;

class Paddle
{
private:
    SDL_Rect rect;
    float speed = 1000.0f;
    int score = 0;

public:
    Paddle(int x, int y)
        : rect{x, y, 20, windowHeight / 5} {}

    void draw(SDL_Renderer* renderer)
    {
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderFillRect(renderer, &rect);
    }

    void moveUp(float deltaTime)
    {
        if (rect.y > 0)
        {
            rect.y -= static_cast<int>(speed * deltaTime);
        }
    }

    void moveDown(float deltaTime)
    {
        if (rect.y + rect.h < windowHeight)
        {
            rect.y += static_cast<int>(speed * deltaTime);
        }
    }

    SDL_Rect getRect() const { return rect; }
    int getScore() const { return score; }
    float getPosX() const { return rect.x; }
    float getPosY() const { return rect.y; }

    friend class Ball;
};

class Ball 
{
private:
    SDL_Rect rect;
    float posX, posY;

    bool waitingForReset = false;
    Uint32 resetStartTime = 0;
    const Uint32 resetDelay = 1000;

public:
    Ball(int x, int y, int w, int h)
        : rect{x, y, w, h}, posX(x), posY(y) {}

    void draw(SDL_Renderer* renderer)
    {
        int centerX = rect.x + rect.w / 2;
        int centerY = rect.y + rect.h / 2;
        int radius = rect.w / 2;

        filledCircleRGBA(renderer, centerX, centerY, radius, 255, 255, 255, 255);
    }

    void delay(int milliseconds)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
    }

    void resetWithDelay()
    {
        waitingForReset = true;
        resetStartTime = SDL_GetTicks();
    }

    void doResetIfReady()
    {
        if (waitingForReset && SDL_GetTicks() - resetStartTime >= resetDelay)
        {
            posX = windowWidth / 2 - rect.w / 2;
            posY = windowHeight / 2 - rect.h / 2;
            speedX = (rand() % 2 == 0 ? -1 : 1) * std::abs(speedX);
            speedY = (rand() % 2 == 0 ? -1 : 1) * std::abs(speedY);
            rect.x = static_cast<int>(posX);
            rect.y = static_cast<int>(posY);
            waitingForReset = false;
        }
    }

    void update(float deltaTime, Paddle* player, Paddle* ai)
    {
        if (waitingForReset)
            return;

        posX += speedX * deltaTime;
        posY += speedY * deltaTime;

        rect.x = static_cast<int>(posX);
        rect.y = static_cast<int>(posY);

        SDL_Rect playerRect = player->getRect(); 
        SDL_Rect aiRect = ai->getRect();

        if (SDL_HasIntersection(&rect, &playerRect))
        {
            speedX = std::abs(speedX);
            posX = playerRect.x + playerRect.w;
        }
        else if (SDL_HasIntersection(&rect, &aiRect))
        {
            speedX = -std::abs(speedX);
            posX = aiRect.x - rect.w;
        }

        if (posY <= 0.0f)
        {
            posY = 0.0f;
            speedY = std::abs(speedY);
        }        
        if (posY + rect.h >= windowHeight)
        {
            posY = windowHeight - rect.h;
            speedY = -std::abs(speedY);
        }

        if (rect.x + rect.w > windowWidth)
        {
            player->score++;
            speedX *= 1.1f;
            speedY *= 1.1f;
            resetWithDelay();
        }
        if (rect.x < 0)
        {
            ai->score++;
            speedX *= 1.1f;
            speedY *= 1.1f;
            resetWithDelay();
        }
    }

    void setPosX(float pos) { posX = pos; }
    void setPoxY(float pos) { posY = pos; }
    float getPosX() const { return posX; }
    float getPosY() const { return posY; }
    SDL_Rect getRect() const { return rect; }
};

Paddle* playerPaddle = nullptr;
Paddle* aiPaddle = nullptr;
Ball* ball = nullptr;

bool initialize()
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        return false;
    }

    if (TTF_Init() < 0)
    {
        std::cerr << "Failed to initialize TTF: " << TTF_GetError() << std::endl;
        return false;
    }

    font = TTF_OpenFont("DejaVuSans-Bold.ttf", 64);

    window = SDL_CreateWindow("Pong Game",
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              windowWidth, windowHeight,
                              SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS);
    if (!window)
    {
        std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
        return false;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (!renderer)
    {
        std::cerr << "Failed to create renderer: " << SDL_GetError() << std::endl;
        return false;
    }

    playerPaddle = new Paddle(20, windowHeight / 2 - 50); 
    if(!playerPaddle)
    {
        std::cerr << "Failed to create player paddle." << std::endl;
        return false;
    }

    aiPaddle = new Paddle(windowWidth - 40, windowHeight / 2 - 50);
    if(!aiPaddle)
    {
        std::cerr << "Failed to create AI paddle." << std::endl;
        return false;
    }

    ball = new Ball(windowWidth / 2 - 10, windowHeight / 2 - 10, 20, 20);
    if(!ball)
    {
        std::cerr << "Failed to create ball." << std::endl;
        return false;
    }

    return true;
}

void cleanup()
{
    delete playerPaddle;
    delete aiPaddle;
    delete ball;

    if (font)
    {
        TTF_CloseFont(font);
        font = nullptr;
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
}

void renderText(const std::string& text, int x, int y)
{
    SDL_Surface* surface = TTF_RenderText_Solid(font, text.c_str(), {255, 255, 255, 255});
    if (surface == nullptr) 
    {
        return;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (texture == nullptr)
    {
        SDL_FreeSurface(surface);
        return;
    }

    SDL_Rect dstRect = { x, y, surface->w, surface->h };
    SDL_RenderCopy(renderer, texture, nullptr, &dstRect);

    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface); 
}

int main()
{
    int choice = showResolutionSelection();
    if (choice == -1)
    {
        return 0; 
    }

    const int resolutions[2][2] =
    {
        {1280, 720},
        {1920, 1080}
    };

    windowWidth = resolutions[choice][0];
    windowHeight = resolutions[choice][1];

    if (!initialize())
    {
        cleanup();
        return -1;
    } 
   
    Uint32 currentTime = SDL_GetTicks();
    Uint32 lastTime = currentTime;
    float deltaTime = 0.0f;

    const Uint8* keyboardState = SDL_GetKeyboardState(NULL);

    SDL_Event e;
    while (isRunning)
    {
        currentTime = SDL_GetTicks();
        deltaTime = (currentTime - lastTime) / 1000.0f; 
        lastTime = currentTime;
   
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT || e.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
            {
                isRunning = false;
            }
        }

        std::string playerScore = std::to_string(playerPaddle->getScore());
        std::string aiScore = std::to_string(aiPaddle->getScore());

        // AI paddle movement logic
        if (ball->getPosY() < aiPaddle->getRect().y)
        {
            aiPaddle->moveUp(deltaTime);
        }
        else if (ball->getPosY() + ball->getRect().h > aiPaddle->getRect().y + aiPaddle->getRect().h)
        {
            aiPaddle->moveDown(deltaTime);
        }

        SDL_PumpEvents();

        if (keyboardState[SDL_SCANCODE_UP])
        {
            playerPaddle->moveUp(deltaTime);
        }
        
        if (keyboardState[SDL_SCANCODE_DOWN])
        {
            playerPaddle->moveDown(deltaTime);
        }    

        ball->update(deltaTime, playerPaddle, aiPaddle);
        ball->doResetIfReady();

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        renderText(playerScore, windowWidth / 4, 20);
        renderText(aiScore, windowWidth - (windowWidth / 4) - 40, 20);

        playerPaddle->draw(renderer);
        aiPaddle->draw(renderer);
        ball->draw(renderer);

        SDL_RenderPresent(renderer);    
    }
    
    cleanup();

    return 0;
}
