#include "./Game.h"
#include "../Logger/Logger.h"
#include "../ECS/ECS.h"
#include "../Components/TransformComponent.h"
#include "../Components/RigidBodyComponent.h"
#include "../Components/SpriteComponent.h"
#include "../Components/AnimationComponent.h"
#include "../Components/BoxColliderComponent.h"
#include "../Components/KeyboardControlledComponent.h"
#include "../Components/CameraFollowComponent.h"
#include "../Components/ProjectileEmitterComponent.h"
#include "../Components/HealthComponent.h"
#include "../Components/TextLabelComponent.h"
#include "../Systems/MovementSystem.h"
#include "../Systems/CameraMovementSystem.h"
#include "../Systems/RenderSystem.h"
#include "../Systems/AnimationSystem.h"
#include "../Systems/CollisionSystem.h"
#include "../Systems/RenderColliderSystem.h"
#include "../Systems/DamageSystem.h"
#include "../Systems/ProjectileEmitSystem.h"
#include "../Systems/KeyboardControlSystem.h"
#include "../Systems/ProjectileLifecycleSystem.h"
#include "../Systems/RenderTextSystem.h"
#include "../Systems/RenderHealthBarSystem.h"
#include "../Systems/RenderGUISystem.h"
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include "../../libs/glm/glm.hpp"
#include"../../libs/imgui/imgui.h"
#include "../../libs/imgui/imgui_sdl.h"
#include "../../libs/imgui/imgui_impl_sdl.h"
#include <fstream>

int Game::windowWidth;
int Game::windowHeight;
int Game::mapWidth;
int Game::mapHeight;

Game::Game() {
    isRunning = false;
    isDebug = false;
    registry = std::make_unique<Registry>();
    assetStore = std::make_unique<AssetStore>();
    eventBus = std::make_unique<EventBus>();
    Logger::Log("Game constructor called!");
}

Game::~Game() {
    Logger::Log("Game destructor called!");   
}

void Game::Initialize() {
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        Logger::Err("Error initializing SDL.");
        return;
    }
    if (TTF_Init() != 0) {
        Logger::Err("Error initializing SDL TTF.");
        return;
    }
    SDL_DisplayMode displayMode;
    SDL_GetCurrentDisplayMode(0, &displayMode);
    windowWidth = 900; // displayMode.w;
    windowHeight = 700; // displayMode.h;
    window = SDL_CreateWindow(
        "SpaceAvenger",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        windowWidth,
        windowHeight,
        NULL
    );
    if (!window) {
        Logger::Err("Error creating SDL window.");
        return;
    }
    renderer = SDL_CreateRenderer(window, -1, 0);
    if (!renderer) {
        Logger::Err("Error creating SDL renderer.");
        return;
    }

    // Initialize the ImGui context
    ImGui::CreateContext();
    ImGuiSDL::Initialize(renderer, windowWidth, windowHeight);

    // Initialize the camera view with the entire screen area
    camera.x = 0;
    camera.y = 0;
    camera.w = windowWidth;
    camera.h = windowHeight;

    // SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
    isRunning = true;
}

void Game::ProcessInput() {
    SDL_Event sdlEvent;
    while (SDL_PollEvent(&sdlEvent)) {
        // ImGui SDL input
        ImGui_ImplSDL2_ProcessEvent(&sdlEvent);
        ImGuiIO& io = ImGui::GetIO();
        int mouseX, mouseY;
        const int buttons = SDL_GetMouseState(&mouseX, &mouseY);
        io.MousePos = ImVec2(mouseX, mouseY);
        io.MouseDown[0] = buttons & SDL_BUTTON(SDL_BUTTON_LEFT);
        io.MouseDown[1] = buttons & SDL_BUTTON(SDL_BUTTON_RIGHT);

        // Handle core SDL events (close window, key pressed, etc.)
        switch (sdlEvent.type) {
            case SDL_QUIT:
                isRunning = false;
                break;
            case SDL_KEYDOWN:
                if (sdlEvent.key.keysym.sym == SDLK_ESCAPE) {
                    isRunning = false;
                }
                if (sdlEvent.key.keysym.sym == SDLK_F1) {
                    isDebug = !isDebug;
                }
                eventBus->EmitEvent<KeyPressedEvent>(sdlEvent.key.keysym.sym);
                break;
        }
    }
}

void Game::LoadLevel(int level) {
    // Add the sytems that need to be processed in our game
    registry->AddSystem<MovementSystem>();
    registry->AddSystem<RenderSystem>();
    registry->AddSystem<AnimationSystem>();
    registry->AddSystem<CollisionSystem>();
    registry->AddSystem<RenderColliderSystem>();
    registry->AddSystem<DamageSystem>();
    registry->AddSystem<KeyboardControlSystem>();
    registry->AddSystem<CameraMovementSystem>();
    registry->AddSystem<ProjectileEmitSystem>();
    registry->AddSystem<ProjectileLifecycleSystem>();
    registry->AddSystem<RenderTextSystem>();
    registry->AddSystem<RenderHealthBarSystem>();
    registry->AddSystem<RenderGUISystem>();

    // Adding assets to the asset store
    assetStore->AddTexture(renderer, "enemy1-image", "./assets/images/enemy.png");
    assetStore->AddTexture(renderer, "meteor1-image", "./assets/images/Meteor_01.png");
    assetStore->AddTexture(renderer, "avenger-image", "./assets/images/avengersprt.png");
    assetStore->AddTexture(renderer, "radar-image", "./assets/images/radar.png");
    assetStore->AddTexture(renderer, "tilemap-image", "./assets/tilemaps/space.png");
    assetStore->AddTexture(renderer, "bullet-image", "./assets/images/missile.png");
    assetStore->AddFont("charriot-font-20", "./assets/fonts/charriot.ttf", 20);
    assetStore->AddFont("pico8-font-5", "./assets/fonts/pico8.ttf", 5);
    assetStore->AddFont("pico8-font-10", "./assets/fonts/pico8.ttf", 10);

    // Load the tilemap
    int tileSize = 200;
    double tileScale = 0.5;
    int mapNumCols = 9;
    int mapNumRows = 7;

    std::fstream mapFile;
    mapFile.open("./assets/tilemaps/space.map");

    for (int y = 0; y < mapNumRows; y++) {
        for (int x = 0; x < mapNumCols; x++) {
            char ch;
            mapFile.get(ch);
            int srcRectY = std::atoi(&ch) * tileSize;
            mapFile.get(ch);
            int srcRectX = std::atoi(&ch) * tileSize;
            mapFile.ignore();

            Entity tile = registry->CreateEntity();
            tile.Group("tiles");
            tile.AddComponent<TransformComponent>(glm::vec2(x * (tileScale * tileSize), y * (tileScale * tileSize)), glm::vec2(tileScale, tileScale), 0.0);
            tile.AddComponent<SpriteComponent>("tilemap-image", tileSize, tileSize, 0, false, srcRectX, srcRectY);
        }
    }
    mapFile.close();
    mapWidth = mapNumCols * tileSize * tileScale;
    mapHeight = mapNumRows * tileSize * tileScale;

    // Create an entity
    Entity avenger = registry->CreateEntity();
    avenger.Tag("player");
    avenger.AddComponent<TransformComponent>(glm::vec2(400.0, 600.0), glm::vec2(0.33, 0.33), 0.0);
    avenger.AddComponent<RigidBodyComponent>(glm::vec2(0.0, 0.0));
    avenger.AddComponent<SpriteComponent>("avenger-image", 141, 517, 1);
    avenger.AddComponent<AnimationComponent>(8, 15, true);
    avenger.AddComponent<BoxColliderComponent>(141 / 3, 517 / 3, glm::vec2(0, 5));
    avenger.AddComponent<ProjectileEmitterComponent>(glm::vec2(150.0, 150.0), 0, 10000, 50, true);
    avenger.AddComponent<KeyboardControlledComponent>(glm::vec2(0, -150), glm::vec2(150, 0), glm::vec2(0, 150), glm::vec2(-150, 0));
    avenger.AddComponent<CameraFollowComponent>();
    avenger.AddComponent<HealthComponent>(100);
    
    Entity radar = registry->CreateEntity();
    radar.AddComponent<TransformComponent>(glm::vec2(windowWidth - 74, 10.0), glm::vec2(1.0, 1.0), 0.0);
    radar.AddComponent<RigidBodyComponent>(glm::vec2(0.0, 0.0));
    radar.AddComponent<SpriteComponent>("radar-image", 64, 64, 9, true);
    radar.AddComponent<AnimationComponent>(8, 5, true);
    
    Entity enemy1 = registry->CreateEntity();
    enemy1.Group("enemies");
    enemy1.AddComponent<TransformComponent>(glm::vec2(450.0, 100.0), glm::vec2(0.125, 0.125), 0.0);
    enemy1.AddComponent<RigidBodyComponent>(glm::vec2(100.0, 0.0));
    enemy1.AddComponent<SpriteComponent>("enemy1-image", 1000, 1000, 1);
    enemy1.AddComponent<BoxColliderComponent>(1000 / 8, 1000 / 8, glm::vec2(5, 7));
    enemy1.AddComponent<ProjectileEmitterComponent>(glm::vec2(0.0, 100.0), 3000, 5000, 20, false);
    enemy1.AddComponent<HealthComponent>(100);
    
    for (int i = 0; i < 5; i++) {
        srand(time(0));
        int n = (rand() % 5) * 150 + 50;
        Entity meteor1 = registry->CreateEntity();
        meteor1.Group("enemies");
        meteor1.AddComponent<TransformComponent>(glm::vec2(n, i * -300), glm::vec2(0.5, 0.5), 0.0);
        meteor1.AddComponent<RigidBodyComponent>(glm::vec2(0.0, 50.0));
        meteor1.AddComponent<SpriteComponent>("meteor1-image", 300, 300, 2);
        meteor1.AddComponent<BoxColliderComponent>(300 / 2, 300 / 2, glm::vec2(5, 5));
        meteor1.AddComponent<HealthComponent>(100);

    };

    for (int i = 0; i < 5; i++) {
        int n = (rand() % 5) * 150 + 50;
        Entity meteor1 = registry->CreateEntity();
        meteor1.Group("enemies");
        meteor1.AddComponent<TransformComponent>(glm::vec2(n,-200 - i * -300.0), glm::vec2(0.5, 0.5), 0.0);
        meteor1.AddComponent<RigidBodyComponent>(glm::vec2(0.0, 50.0));
        meteor1.AddComponent<SpriteComponent>("meteor1-image", 300, 300, 2);
        meteor1.AddComponent<BoxColliderComponent>(300 / 2, 300 / 2, glm::vec2(5, 5));
        meteor1.AddComponent<HealthComponent>(100);
    };

    Entity inviswallL = registry->CreateEntity();
    inviswallL.Group("obstacles");
    inviswallL.AddComponent<TransformComponent>(glm::vec2(0.0, 0.0), glm::vec2(1.0, 1.0), 0.0);
    inviswallL.AddComponent<BoxColliderComponent>(10, 700, glm::vec2(1, 1));
    
    Entity inviswallR = registry->CreateEntity();
    inviswallR.Group("obstacles");
    inviswallR.AddComponent<TransformComponent>(glm::vec2(900.0, 0.0), glm::vec2(1.0, 1.0), 0.0);
    inviswallR.AddComponent<BoxColliderComponent>(10, 700, glm::vec2(1, 1));
    
    Entity label = registry->CreateEntity();
    SDL_Color green = {0, 255, 0};
    label.AddComponent<TextLabelComponent>(glm::vec2(windowWidth / 2 - 40, 10), "AVENGER 1.0", "pico8-font-10", green, true);
}

void Game::Setup() {
    LoadLevel(1);
}

void Game::Update() {
    // If we are too fast, waste some time until we reach the MILLISECS_PER_FRAME
    int timeToWait = MILLISECS_PER_FRAME - (SDL_GetTicks() - millisecsPreviousFrame);
    if (timeToWait > 0 && timeToWait <= MILLISECS_PER_FRAME) {
        SDL_Delay(timeToWait);
    }

    // The difference in ticks since the last frame, converted to seconds
    double deltaTime = (SDL_GetTicks() - millisecsPreviousFrame) / 1000.0;

    // Store the "previous" frame time
    millisecsPreviousFrame = SDL_GetTicks();
   
    // Reset all event handlers for the current frame
    eventBus->Reset();

    // Perform the subscription of the events for all systems
    registry->GetSystem<MovementSystem>().SubscribeToEvents(eventBus);
    registry->GetSystem<DamageSystem>().SubscribeToEvents(eventBus);
    registry->GetSystem<KeyboardControlSystem>().SubscribeToEvents(eventBus);
    registry->GetSystem<ProjectileEmitSystem>().SubscribeToEvents(eventBus);

    // Update the registry to process the entities that are waiting to be created/deleted
    registry->Update();
    
    // Invoke all the systems that need to update 
    registry->GetSystem<MovementSystem>().Update(deltaTime);
    registry->GetSystem<AnimationSystem>().Update();
    registry->GetSystem<CollisionSystem>().Update(eventBus);
    registry->GetSystem<ProjectileEmitSystem>().Update(registry);
    registry->GetSystem<CameraMovementSystem>().Update(camera);
    registry->GetSystem<ProjectileLifecycleSystem>().Update();
}

void Game::Render() {
    SDL_SetRenderDrawColor(renderer, 21, 21, 21, 255);
    SDL_RenderClear(renderer);

    // Invoke all the systems that need to render 
    registry->GetSystem<RenderSystem>().Update(renderer, assetStore, camera);
    registry->GetSystem<RenderTextSystem>().Update(renderer, assetStore, camera);
    registry->GetSystem<RenderHealthBarSystem>().Update(renderer, assetStore, camera);
    if (isDebug) {
        registry->GetSystem<RenderColliderSystem>().Update(renderer, camera);
        registry->GetSystem<RenderGUISystem>().Update(registry, camera);
    }

    SDL_RenderPresent(renderer);
}

void Game::Run() {
    Setup();
    while (isRunning) {
        ProcessInput();
        Update();
        Render();
    }
}

void Game::Destroy() {
    ImGuiSDL::Deinitialize();
    ImGui::DestroyContext();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
