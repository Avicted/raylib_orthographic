// Defines -----------------------------------------------------------------------
#define RLIGHTS_IMPLEMENTATION

// Includes ----------------------------------------------------------------------
#include "includes.h"
#include "raylib_includes.h"

// Types -------------------------------------------------------------------------

// Variables ---------------------------------------------------------------------
i32 SCREEN_WIDTH = 640 * 2;
i32 SCREEN_HEIGHT = 360 * 2;

bool Debug = false;
bool IsPaused = false;
const i64 MAP_SIZE = 128;
const i64 SQUARE_SIZE = 32;

u64 CPUMemory = 0L;

Font MainFont = {0};

// Ground -----------------------------
Texture2D GrassTexture = {0};
Texture2D GrassTexture02 = {0};
Texture2D GrassTexture03 = {0};
Texture2D GrassTexture04 = {0};

Mesh GroundMesh = {0};

Material *GroundMaterials = NULL;

Camera3D MainCamera = {};
f64 Zoom = 0.5f;

internal void
ParseInputArgs(i32 argc, char **argv)
{
    if (argc == 1)
    {
        printf("\tNo input args OK!\n");
        printf("\tCurrent working directory: %s\n", GetWorkingDirectory());
        return;
    }

    for (i32 i = 1; i < argc; ++i)
    {
        if (strcmp(argv[i], "GALAXY_DEBUG") == 0)
        {
            printf("\tRunning in DEBUG mode !!!\n");
            Debug = true;
        }
    }
}

internal void
HandleWindowResize(void)
{
    if (IsWindowResized() && !IsWindowFullscreen())
    {
        SCREEN_WIDTH = GetScreenWidth();
        SCREEN_HEIGHT = GetScreenHeight();
    }

    // Check for alt + enter
    if (IsKeyPressed(KEY_ENTER) && (IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT)))
    {
        // See which display we are on right now
        i32 Display = GetCurrentMonitor();

        if (IsWindowFullscreen())
        {
            // If we are full screen, then go back to the windowed size
            SetWindowSize(SCREEN_WIDTH, SCREEN_HEIGHT);
        }
        else
        {
            // If we are not full screen, set the window size to match the monitor we are on
            SetWindowSize(GetMonitorWidth(Display), GetMonitorHeight(Display));
        }

        // Toggle the state
        ToggleFullscreen();

        SCREEN_WIDTH = GetScreenWidth();
        SCREEN_HEIGHT = GetScreenHeight();
    }
}

internal void
GameUpdate(f64 DeltaTime)
{
    HandleWindowResize();

    if (IsKeyPressed(KEY_ESCAPE))
    {
        CloseWindow();
    }

    if (IsKeyPressed(KEY_F11))
    {
        ToggleFullscreen();
    }

    // Mouse Scroll to change MainCamera.fovy
    if (GetMouseWheelMove() < 0)
    {
        MainCamera.fovy += 10.0f;
    }
    else if (GetMouseWheelMove() > 0)
    {
        MainCamera.fovy -= 10.0f;
    }

    // Right Mouse down to move
    if (IsMouseButtonDown(MOUSE_RIGHT_BUTTON))
    {
        // Get the mouse movement delta
        Vector2 mouseDelta = GetMouseDelta();

        // Adjust the speed multiplier for desired speed
        float speedMultiplier = PI / 2;

        // Move the camera based on the mouse movement delta
        MainCamera.position.x += mouseDelta.x * speedMultiplier;
        MainCamera.position.z += mouseDelta.y * speedMultiplier;

        // Move the camera target based on the mouse movement delta
        MainCamera.target.x += mouseDelta.x * speedMultiplier;
        MainCamera.target.z += mouseDelta.y * speedMultiplier;
    }
}

internal void
GameRender(f64 DeltaTime)
{
    BeginDrawing();
    Color bgColor = (Color){10, 10, 4, 255};
    ClearBackground(bgColor);

    BeginMode3D(MainCamera);

    // rlgl set the camera far plane in the distance

    // Draw a orthographic grid with tile borders
    DrawGrid(MAP_SIZE, SQUARE_SIZE);

    // Draw a cube at 0 0 0
    DrawCube((Vector3){0.0f, 16.0f, 0.0f}, SQUARE_SIZE, SQUARE_SIZE, SQUARE_SIZE, MAGENTA);

    for (i64 i = -MAP_SIZE / 2; i < MAP_SIZE / 2; ++i)
    {
        for (i64 j = -MAP_SIZE / 2; j < MAP_SIZE / 2; ++j)
        {
            const i64 SquareId = ((i + MAP_SIZE / 2) * MAP_SIZE) + (j + MAP_SIZE / 2);

            Matrix GroundMatrix = MatrixMultiply(MatrixTranslate(i * SQUARE_SIZE + SQUARE_SIZE / 2, 0, j * SQUARE_SIZE + SQUARE_SIZE / 2), MatrixRotateX(0));

            DrawMesh(GroundMesh, GroundMaterials[SquareId], GroundMatrix);
        }
    }

    EndMode3D();

    // Draw UI
    // Camera Position upper left corner
    // Draw the FPS with our font
    DrawTextEx(MainFont, TextFormat("FPS: %i", GetFPS()), {10, 10}, 16, 2, BLACK);
    DrawTextEx(MainFont, TextFormat("FPS: %i", GetFPS()), {13, 13}, 16, 2, WHITE);

    // Vector2 Pos = GetMousePosition();
    // DrawTextEx(MainFont, TextFormat("Mouse Pos: x: %f, y: %f", GetMouseDelta()), {10, 20}, 16, 2, WHITE);

    if (IsMouseButtonDown(MOUSE_RIGHT_BUTTON))
    {
        DrawTextEx(MainFont, "RIGHT MOUSE IS PRESSED!", {10, 32}, 16, 2, BLACK);
        DrawTextEx(MainFont, "RIGHT MOUSE IS PRESSED!", {13, 35}, 16, 2, WHITE);
    }

    DrawTextEx(MainFont, TextFormat("MainCamera.fovy: %f", MainCamera.fovy), {10, 64}, 16, 2, BLACK);
    DrawTextEx(MainFont, TextFormat("MainCamera.fovy: %f", MainCamera.fovy), {13, 67}, 16, 2, WHITE);

    EndDrawing();
}

internal void
PrintMemoryUsage(void)
{
    printf("\n\tMemory used in GigaBytes: %f\n", (f64)CPUMemory / (f64)Gigabytes(1));
    printf("\tMemory used in MegaBytes: %f\n", (f64)CPUMemory / (f64)Megabytes(1));
}

internal void
CleanupOurStuff(void)
{
    CloseWindow(); // Close window and OpenGL context
    printf("\n\tClosed window and OpenGL context\n");

    free(GroundMaterials);
    CPUMemory -= (MAP_SIZE * MAP_SIZE) * sizeof(Material);

    // @Note(Victor): There should be no allocated memory left
    Assert(CPUMemory == 0);
}

internal void
SigIntHandler(i32 Signal)
{
    printf("\tCaught SIGINT, exiting peacefully!\n");

    CleanupOurStuff();

    exit(0);
}

i32 main(i32 argc, char **argv)
{
    signal(SIGINT, SigIntHandler);

    ParseInputArgs(argc, argv);

    printf("\tHello from raylib_orthographic!\n\n");

    MainCamera.position = {350.0f, 400.0f, 256.0f};
    MainCamera.target = {0.0f, 0.0f, 0.0f};
    MainCamera.up = {0.0f, 1.0f, 0.0f};
    MainCamera.fovy = 400.0f; // Adjust if necessary
    MainCamera.projection = CAMERA_ORTHOGRAPHIC;

    // Raylib
    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "raylib_orthographic");

    SetTargetFPS(60);

    MainFont = LoadFontEx("./resources/fonts/SuperMarioBros2.ttf", 32, 0, 250);
    GrassTexture = LoadTexture("./resources/images/grass.png");
    GrassTexture02 = LoadTexture("./resources/images/grass_02.png");
    GrassTexture03 = LoadTexture("./resources/images/grass_03.png");
    GrassTexture04 = LoadTexture("./resources/images/grass_04.png");

    // Init the ground materials
    {
        GroundMaterials = (Material *)calloc((MAP_SIZE * MAP_SIZE), sizeof(Material));
        CPUMemory += (MAP_SIZE * MAP_SIZE) * sizeof(Material);

        for (usize i = 0; i < MAP_SIZE; ++i)
        {
            for (usize j = 0; j < MAP_SIZE; ++j)
            {
                const i64 SquareId = (i * MAP_SIZE) + j;
                printf("\tSetting the SquareId: %i\n", SquareId);

                GroundMaterials[SquareId] = LoadMaterialDefault();

                // Set one of the four Grass materials
                const int RandomGrassId = GetRandomValue(0, 3);

                if (RandomGrassId == 0)
                {
                    GroundMaterials[SquareId].maps[MATERIAL_MAP_DIFFUSE].texture = GrassTexture;
                }
                else if (RandomGrassId == 1)
                {
                    GroundMaterials[SquareId].maps[MATERIAL_MAP_DIFFUSE].texture = GrassTexture02;
                }
                else if (RandomGrassId == 2)
                {
                    GroundMaterials[SquareId].maps[MATERIAL_MAP_DIFFUSE].texture = GrassTexture03;
                }
                else if (RandomGrassId == 3)
                {
                    GroundMaterials[SquareId].maps[MATERIAL_MAP_DIFFUSE].texture = GrassTexture04;
                }
            }
        }
    }

    GroundMesh = GenMeshPlane(32, 32, 1, 1);

    printf("\n\tMemory usage before we start the game loop\n");
    PrintMemoryUsage();

    // Main loop
    while (!WindowShouldClose()) // Detect window close button or ESC key
    {
        f64 DeltaTime = GetFrameTime();
        GameUpdate(DeltaTime);
        GameRender(DeltaTime);
    }

    CleanupOurStuff();

    return (0);
}
