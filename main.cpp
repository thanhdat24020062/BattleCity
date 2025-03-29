#include <iostream>
#include <SDL.h>
#include <SDL_image.h>
#include <vector>
#include <algorithm>
#include <SDL_mixer.h>

using namespace std;

const int SCREEN_WIDTH = 1100;
const int SCREEN_HEIGHT = 660;
const int TILE_SIZE = 35;
const int MAP_WIDTH = SCREEN_WIDTH / TILE_SIZE;
const int MAP_HEIGHT = SCREEN_HEIGHT / TILE_SIZE;

class Bullet {
public:
    int x, y;
    int dx, dy;
    SDL_Rect rect;
    bool active;

    Bullet(int startX, int startY, int dirX, int dirY) {
        x = startX;
        y = startY;
        dx = dirX * 2;
        dy = dirY * 2;
        active = true;
        rect = {x, y, 9, 9};
    }

    void move() {
        x += dx;
        y += dy;
        rect.x = x;
        rect.y = y;
        if (x < 0 || x > SCREEN_WIDTH || y < 0 || y > SCREEN_HEIGHT) {
            active = false;
        }
    }

    void render(SDL_Renderer* renderer) {
        if (active) {
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderFillRect(renderer, &rect);
        }
    }
};

class Wall {
public:
    int x, y;
    SDL_Rect rect;
    bool active;

    Wall(int startX, int startY) {
        x = startX;
        y = startY;
        active = true;
        rect = {x, y, TILE_SIZE, TILE_SIZE};
    }

    void render(SDL_Renderer* renderer, SDL_Texture* texture) {
        if (active) {
            SDL_RenderCopy(renderer, texture, NULL, &rect);
        }
    }
};

class PlayerTank {
public:
    int x, y;
    int dirX, dirY;
    SDL_Rect rect;
    vector<Bullet> bullets;

    PlayerTank(int startX, int startY) {
        x = startX;
        y = startY;
        rect = {x, y, TILE_SIZE, TILE_SIZE};
        dirX = 0;
        dirY = -1;
    }

    void move(int dx, int dy, const vector<Wall>& walls) {
        int newX = x + dx;
        int newY = y + dy;
        dirX = dx;
        dirY = dy;

        SDL_Rect newRect = {newX, newY, TILE_SIZE, TILE_SIZE};
        for (const auto& wall : walls) {
            if (wall.active && SDL_HasIntersection(&newRect, &wall.rect)) {
                return;
            }
        }

        if (newX >= TILE_SIZE && newX <= SCREEN_WIDTH - TILE_SIZE * 2 &&
            newY >= TILE_SIZE && newY <= SCREEN_HEIGHT - TILE_SIZE * 2) {
            x = newX;
            y = newY;
            rect.x = x;
            rect.y = y;
        }
    }

    void shoot() {
        bullets.emplace_back(x + TILE_SIZE/2 - 5, y + TILE_SIZE/2 - 5, dirX, dirY);
    }

    void updateBullets() {
        bullets.erase(remove_if(bullets.begin(), bullets.end(),
            [](Bullet& b) { return !b.active; }), bullets.end());
        for (auto& bullet : bullets) {
            bullet.move();
        }
    }

    void render(SDL_Renderer* renderer) {
        SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
        SDL_RenderFillRect(renderer, &rect);
        for (auto& bullet : bullets) {
            bullet.render(renderer);
        }
    }
};

class EnemyTank {
public:
    int x, y;
    int dirX, dirY;
    int moveDelay, shootDelay;
    SDL_Rect rect;
    bool active;
    vector<Bullet> bullets;

    EnemyTank(int startX, int startY) {
        moveDelay = 20;
        shootDelay = 70;
        x = startX;
        y = startY;
        rect = {x, y, TILE_SIZE, TILE_SIZE};
        dirX = 0;
        dirY = 1;
        active = true;
    }

    void move(const vector<Wall>& walls) {
        if (--moveDelay > 0) return;
        moveDelay = 15;

        int directions[4][2] = {{0,-5}, {0,5}, {-5,0}, {5,0}};
        int r = rand() % 4;
        dirX = directions[r][0];
        dirY = directions[r][1];

        int newX = x + dirX;
        int newY = y + dirY;

        SDL_Rect newRect = {newX, newY, TILE_SIZE, TILE_SIZE};
        for (const auto& wall : walls) {
            if (wall.active && SDL_HasIntersection(&newRect, &wall.rect)) {
                return;
            }
        }
        if (newX >= TILE_SIZE && newX <= SCREEN_WIDTH - TILE_SIZE * 2 &&
            newY >= TILE_SIZE && newY <= SCREEN_HEIGHT - TILE_SIZE * 2) {
            x = newX;
            y = newY;
            rect.x = x;
            rect.y = y;
        }
    }

    void shoot() {
        bullets.emplace_back(x + TILE_SIZE/5 - 5, y + TILE_SIZE/5 - 5, dirX, dirY);
    }

    void updateBullets() {
        bullets.erase(remove_if(bullets.begin(), bullets.end(),
            [](Bullet& b) { return !b.active; }), bullets.end());
        for (auto& bullet : bullets) {
            bullet.move();
        }
    }

    void render(SDL_Renderer* renderer) {
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderFillRect(renderer, &rect);
        for (auto& bullet : bullets) {
            bullet.render(renderer);
        }
    }
};

class Menu {
public:
    SDL_Texture* playTexture;
    SDL_Texture* exitTexture;
    SDL_Texture* backgroundTexture;
    Mix_Music* backgroundMusic;
    SDL_Rect playButton;
    SDL_Rect exitButton;
    bool showMenu;

    Menu(SDL_Renderer* renderer) {
        backgroundMusic = Mix_LoadMUS("nhacnen.wav");
        if (!backgroundMusic) {
            cerr << "Failed to load menu music: " << Mix_GetError() << endl;
        } else {
            Mix_VolumeMusic(64);
            Mix_PlayMusic(backgroundMusic, -1);
        }

        backgroundTexture = IMG_LoadTexture(renderer, "backgroundmenu.png");
        playTexture = IMG_LoadTexture(renderer, "play.png");
        exitTexture = IMG_LoadTexture(renderer, "exit.png");

        playButton = {SCREEN_WIDTH/2 - 100, 300, 200, 80};
        exitButton = {SCREEN_WIDTH/2 - 100, 400, 200, 80};
        showMenu = true;
    }

    void handleEvents(SDL_Event& e) {
        if (e.type == SDL_MOUSEBUTTONDOWN) {
            int x, y;
            SDL_GetMouseState(&x, &y);
            SDL_Point mousePos = {x, y};

            if (SDL_PointInRect(&mousePos, &playButton)) {
                showMenu = false;
            }
            if (SDL_PointInRect(&mousePos, &exitButton)) {
                exit(0);
            }
        }
    }

    void render(SDL_Renderer* renderer) {
        if (backgroundTexture) {
            SDL_RenderCopy(renderer, backgroundTexture, NULL, NULL);
        } else {
            SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
            SDL_RenderClear(renderer);
        }

        if (playTexture) SDL_RenderCopy(renderer, playTexture, NULL, &playButton);
        if (exitTexture) SDL_RenderCopy(renderer, exitTexture, NULL, &exitButton);

        SDL_RenderPresent(renderer);
    }

    ~Menu() {
        SDL_DestroyTexture(backgroundTexture);
        SDL_DestroyTexture(playTexture);
        SDL_DestroyTexture(exitTexture);
        if (backgroundMusic) {
            Mix_HaltMusic();
            Mix_FreeMusic(backgroundMusic);
        }
    }
};

class Game {
public:
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* wallTexture;
    SDL_Texture* winTexture;
    SDL_Texture* gameOverTexture;
    Mix_Chunk* playerShootSound;
    Mix_Chunk* enemyShootSound;
    Mix_Music* backgroundMusic;

    bool isGameOver;
    bool running;
    bool isVictory;
    vector<Wall> walls;
    PlayerTank player;
    int enemyNumber;
    vector<EnemyTank> enemies;
    Uint32 endTime;

    Game() : player(((MAP_WIDTH-1)/2)*TILE_SIZE, (MAP_HEIGHT-2)*TILE_SIZE) {
        running = true;
        isGameOver = false;
        isVictory = false;
        enemyNumber = 5;
        endTime = 0;

        SDL_Init(SDL_INIT_VIDEO);
        IMG_Init(IMG_INIT_PNG);

        window = SDL_CreateWindow("Battle City", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                 SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

        Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);
        backgroundMusic = Mix_LoadMUS("nhacnen.wav");
        Mix_PlayMusic(backgroundMusic, -1);

        playerShootSound = Mix_LoadWAV("shoot.wav");
        enemyShootSound = Mix_LoadWAV("shoot.wav");
        Mix_VolumeChunk(playerShootSound, 15);
        Mix_VolumeChunk(enemyShootSound, 15);

        wallTexture = IMG_LoadTexture(renderer, "wall.png");
        winTexture = IMG_LoadTexture(renderer, "win.png");
        gameOverTexture = IMG_LoadTexture(renderer, "gameover.png");

        generateWalls();
        spawnEnemies();
    }

    void generateWalls() {
        for (int i = 2; i < MAP_HEIGHT - 1; i += 3) {
            for (int j = 2; j < MAP_WIDTH - 1; j += 3) {
                walls.emplace_back(j * TILE_SIZE, i * TILE_SIZE);
            }
        }
    }

    void spawnEnemies() {
        enemies.clear();
        for (int i = 0; i < enemyNumber; i++) {
            bool valid = false;
            int x, y;
            while (!valid) {
                x = (rand() % (MAP_WIDTH - 4) + 2) * TILE_SIZE;
                y = (rand() % (MAP_HEIGHT - 4) + 2) * TILE_SIZE;
                valid = true;

                SDL_Rect tempRect = {x, y, TILE_SIZE, TILE_SIZE};
                for (const auto& wall : walls) {
                    if (wall.active && SDL_HasIntersection(&tempRect, &wall.rect)) {
                        valid = false;
                        break;
                    }
                }
                if (SDL_HasIntersection(&tempRect, &player.rect)) {
                    valid = false;
                }
            }
            enemies.emplace_back(x, y);
        }
    }

    void handleEvents() {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
            else if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_UP: player.move(0, -5, walls); break;
                    case SDLK_DOWN: player.move(0, 5, walls); break;
                    case SDLK_LEFT: player.move(-5, 0, walls); break;
                    case SDLK_RIGHT: player.move(5, 0, walls); break;
                    case SDLK_SPACE:
                        player.shoot();
                        Mix_PlayChannel(-1, playerShootSound, 0);
                        break;
                }
            }
        }
    }

    void update() {
        player.updateBullets();

        // Update enemies
        for (auto& enemy : enemies) {
            if (enemy.active) {
                enemy.move(walls);
                enemy.updateBullets();
                if (rand() % 100 < 2) {
                    enemy.shoot();
                    Mix_PlayChannel(-1, enemyShootSound, 0);
                }
            }
        }

        // Check collisions
        for (auto& bullet : player.bullets) {
            for (auto& wall : walls) {
                if (wall.active && SDL_HasIntersection(&bullet.rect, &wall.rect)) {
                    wall.active = false;
                    bullet.active = false;
                }
            }
            for (auto& enemy : enemies) {
                if (enemy.active && SDL_HasIntersection(&bullet.rect, &enemy.rect)) {
                    enemy.active = false;
                    bullet.active = false;
                }
            }
        }

        // Check player hit
        for (auto& enemy : enemies) {
            for (auto& bullet : enemy.bullets) {
                if (SDL_HasIntersection(&bullet.rect, &player.rect)) {
                    isGameOver = true;
                    running = false;
                    endTime = SDL_GetTicks();
                }
            }
        }

        // Check victory
        enemies.erase(remove_if(enemies.begin(), enemies.end(),
            [](EnemyTank& e) { return !e.active; }), enemies.end());

        if (enemies.empty()) {
            isVictory = true;
            running = false;
            endTime = SDL_GetTicks();
        }
    }

    void render() {
        SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);
        SDL_RenderClear(renderer);

        if (isVictory || isGameOver) {
            SDL_Texture* endTexture = isVictory ? winTexture : gameOverTexture;
            if (endTexture) {
                SDL_RenderCopy(renderer, endTexture, NULL, NULL);
            }
        } else {
            // Draw game board
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            for (int i = 1; i < MAP_HEIGHT - 1; i++) {
                for (int j = 1; j < MAP_WIDTH - 1; j++) {
                    SDL_Rect tile = {j * TILE_SIZE, i * TILE_SIZE, TILE_SIZE, TILE_SIZE};
                    SDL_RenderFillRect(renderer, &tile);
                }
            }

            // Draw walls
            for (auto& wall : walls) {
                wall.render(renderer, wallTexture);
            }

            // Draw player
            player.render(renderer);

            // Draw enemies
            for (auto& enemy : enemies) {
                if (enemy.active) enemy.render(renderer);
            }
        }

        SDL_RenderPresent(renderer);
    }

    void run() {
        while (running) {
            handleEvents();
            update();
            render();
            SDL_Delay(16);
        }

        // Show end screen for 3 seconds
        if (isVictory || isGameOver) {
            Uint32 startTime = SDL_GetTicks();
            while (SDL_GetTicks() - startTime < 3000) {
                render();
                SDL_Delay(16);
            }
        }
    }

    ~Game() {
        SDL_DestroyTexture(wallTexture);
        SDL_DestroyTexture(winTexture);
        SDL_DestroyTexture(gameOverTexture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        Mix_FreeChunk(playerShootSound);
        Mix_FreeChunk(enemyShootSound);
        Mix_FreeMusic(backgroundMusic);
        Mix_CloseAudio();
    }
};

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    IMG_Init(IMG_INIT_PNG);
    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);

    SDL_Window* menuWindow = SDL_CreateWindow("Menu", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                            SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer* menuRenderer = SDL_CreateRenderer(menuWindow, -1, SDL_RENDERER_ACCELERATED);

    Menu menu(menuRenderer);
    while (menu.showMenu) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) exit(0);
            menu.handleEvents(e);
        }
        menu.render(menuRenderer);
        SDL_Delay(16);
    }

    SDL_DestroyRenderer(menuRenderer);
    SDL_DestroyWindow(menuWindow);

    Game game;
    game.run();

    Mix_CloseAudio();
    IMG_Quit();
    SDL_Quit();
    return 0;
}
