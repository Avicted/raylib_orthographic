// Defines ---------------------------------------------------
#define RLIGHTS_IMPLEMENTATION

// Includes --------------------------------------------------
#include "includes.h"
#include "raylib_includes.h"

// Variables -------------------------------------------------
i32 SCREEN_WIDTH = 640 * 2;
i32 SCREEN_HEIGHT = 360 * 2;

bool Debug = false;
const i64 MAP_SIZE = 512;
const i64 SQUARE_SIZE = 1;

u64 CPUMemory = 0L;

Font MainFont = {0};

// Ground ----------------------------------------------------
Texture2D GrassTexture = {0};
Texture2D GrassTexture02 = {0};
Texture2D GrassTexture03 = {0};
Texture2D GrassTexture04 = {0};

Mesh GroundMesh = {0};

Material *GroundMaterials = NULL;
Shader CustomShader = {0};

Camera3D MainCamera = {};
f64 Zoom = 0.5f;
Vector3 CameraStartPosition = (Vector3){30.0f, 30.0f, 30.0f};

// Types -----------------------------------------------------
typedef struct GroundTile
{
    // 3D position
    Matrix MatrixTransform;

    // Material
    usize MaterialIndex;
    Material Mat;
};

GroundTile *GroundTiles = NULL;

// Instanced rendering specific data ------------------------
Matrix *Transforms01 = NULL;
usize Transforms01Count = 0;
Material Mat01;

Matrix *Transforms02 = NULL;
usize Transforms02Count = 0;
Material Mat02;

Matrix *Transforms03 = NULL;
usize Transforms03Count = 0;
Material Mat03;

Matrix *Transforms04 = NULL;
usize Transforms04Count = 0;
Material Mat04;
// ----------------------------------------------------------

internal std::vector<GroundTile>
GetGroundTilesByMaterialIndex(GroundTile *groundTiles, usize count, usize targetIndex)
{
    std::vector<GroundTile> result;

    for (usize i = 0; i < count; ++i)
    {
        if (groundTiles[i].MaterialIndex == targetIndex)
        {
            result.push_back(groundTiles[i]);
        }
    }

    return result;
}

internal std::vector<Matrix>
GetMatricesByMaterialIndex(GroundTile *groundTiles, usize count, usize targetIndex)
{
    std::vector<Matrix> result;

    for (usize i = 0; i < count; ++i)
    {
        if (groundTiles[i].MaterialIndex == targetIndex)
        {
            result.push_back(groundTiles[i].MatrixTransform);
        }
    }

    return result;
}

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
        if (strcmp(argv[i], "RAYLIB_ORTHOGRAPHIC_DEBUG") == 0)
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

    if (GetMouseWheelMove() < 0)
    {
        // Zoom out
        MainCamera.fovy += 5.0f; // Increase fovy to zoom out

        // Move camera up slightly to give a zooming-out effect
        Vector3 forward = Vector3Normalize(Vector3Subtract(MainCamera.target, MainCamera.position));
        MainCamera.position = Vector3Add(MainCamera.position, Vector3Scale(forward, -2.0f));
    }
    else if (GetMouseWheelMove() > 0)
    {
        // Zoom in
        MainCamera.fovy -= 5.0f; // Decrease fovy to zoom in

        // Move camera down slightly to give a zooming-in effect
        Vector3 forward = Vector3Normalize(Vector3Subtract(MainCamera.target, MainCamera.position));
        MainCamera.position = Vector3Add(MainCamera.position, Vector3Scale(forward, 2.0f));
    }

    // Clamp the fovy value to prevent extreme zoom levels
    if (MainCamera.fovy < 10.0f)
    {
        MainCamera.fovy = 10.0f;
    }
    if (MainCamera.fovy > 1000.0f)
    {
        MainCamera.fovy = 1000.0f;
    }

    if (IsMouseButtonDown(MOUSE_RIGHT_BUTTON))
    {
        Vector2 mouseDelta = GetMouseDelta();
        f32 speedMultiplier = PI / 8;

        Vector3 forward = Vector3Normalize(Vector3Subtract(MainCamera.target, MainCamera.position));
        Vector3 right = Vector3CrossProduct(forward, MainCamera.up);

        // Move the camera based on the mouse movement delta
        MainCamera.position = Vector3Add(MainCamera.position, Vector3Scale(right, mouseDelta.x * speedMultiplier));          // Move right/left
        MainCamera.position = Vector3Add(MainCamera.position, Vector3Scale(MainCamera.up, -mouseDelta.y * speedMultiplier)); // Move up/down

        // Update the camera target to maintain focus
        MainCamera.target = Vector3Add(MainCamera.target, Vector3Scale(right, mouseDelta.x * speedMultiplier));
        MainCamera.target = Vector3Add(MainCamera.target, Vector3Scale(MainCamera.up, -mouseDelta.y * speedMultiplier));
    }
}

internal void
GameRender(f64 DeltaTime)
{
    Color bgColor = (Color){10, 10, 24, 255};
    ClearBackground(bgColor);

    BeginDrawing();

    Color Color1 = (Color){0, 255, 255, 255};
    Color Color2 = (Color){0, 100, 255, 255};
    DrawRectangleGradientV(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Color1, Color2);

    BeginMode3D(MainCamera);

    // Set the Camera3D far plane further away than the default 1000.0f -> 2000.0f
    {
        // Set the projection matrix based on the camera
        Matrix projection = MatrixPerspective(MainCamera.fovy * (PI / 180.0f),
                                              (float)GetScreenWidth() / (float)GetScreenHeight(),
                                              0.01f, 2000.0f);
        rlSetMatrixProjection(projection);
    }

    // Center of the world
    Vector3 CubePosition = Vector3{0.0f, SQUARE_SIZE, 0.0f};
    DrawCube(CubePosition, SQUARE_SIZE, SQUARE_SIZE, SQUARE_SIZE, MAGENTA);

    DrawMeshInstanced(GroundMesh, Mat01, Transforms01, Transforms01Count);
    DrawMeshInstanced(GroundMesh, Mat02, Transforms02, Transforms02Count);
    DrawMeshInstanced(GroundMesh, Mat03, Transforms03, Transforms03Count);
    DrawMeshInstanced(GroundMesh, Mat04, Transforms04, Transforms04Count);

    EndMode3D();

    // Draw UI -----------------------------------------------------------------------
    DrawTextEx(MainFont, TextFormat("FPS: %i", GetFPS()), {10, 10}, 16, 2, BLACK);
    DrawTextEx(MainFont, TextFormat("FPS: %i", GetFPS()), {13, 13}, 16, 2, WHITE);

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

    free(GroundTiles);
    CPUMemory -= ((MAP_SIZE * MAP_SIZE) * sizeof(GroundTile));

    // @Note(Victor): There should be no allocated memory left
    Assert(CPUMemory == 0);

    PrintMemoryUsage();

    printf("\n\tAll memory successfully deallocated.\n");
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

    MainCamera.position = CameraStartPosition;
    MainCamera.target = {0.0f, 0.0f, 0.0f};
    MainCamera.up = {0.0f, 1.0f, 0.0f};
    MainCamera.fovy = 45.0f; // Adjust if necessary
    MainCamera.projection = CAMERA_ORTHOGRAPHIC;

    // Raylib
    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "raylib_orthographic");

    SetTargetFPS(144);
    SetWindowState(FLAG_VSYNC_HINT);

    MainFont = LoadFontEx("./resources/fonts/SuperMarioBros2.ttf", 32, 0, 250);
    GrassTexture = LoadTexture("./resources/images/grass.png");
    GrassTexture02 = LoadTexture("./resources/images/grass_02.png");
    GrassTexture03 = LoadTexture("./resources/images/grass_03.png");
    GrassTexture04 = LoadTexture("./resources/images/grass_04.png");

    CustomShader = LoadShader("./shaders/lighting_instancing.vs", "./shaders/lighting.fs");

    // Get shader locations
    CustomShader.locs[SHADER_LOC_MATRIX_MVP] = GetShaderLocation(CustomShader, "mvp");
    CustomShader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(CustomShader, "viewPos");
    CustomShader.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocationAttrib(CustomShader, "instanceTransform");

    // Lighting
    {
        // Setting shader values
        i32 AmbientLoc = GetShaderLocation(CustomShader, "ambient");
        f64 AmbientValue[4] = {0.8, 0.8, 0.8, 0.1};
        SetShaderValue(CustomShader, AmbientLoc, &AmbientValue, SHADER_UNIFORM_VEC4);

        i32 ColorDiffuseLoc = GetShaderLocation(CustomShader, "colorDiffuse");
        f64 DiffuseValue[4] = {1.0, 1.0, 1.0, 1.0};
        SetShaderValue(CustomShader, ColorDiffuseLoc, &DiffuseValue, SHADER_UNIFORM_VEC4);

        // Like the sun shining on the earth
        CreateLight(LIGHT_DIRECTIONAL, {1000.0f, 1000.0f, 0.0f}, Vector3Zero(), WHITE, CustomShader);

        // We can also add a polight at the center of the world
        // CreateLight(LIGHT_POINT, {0.0f, 0.0f, 0.0f}, Vector3Zero(), WHITE, CustomShader);
    }

    // Create the ground tiles
    {
        GroundMaterials = (Material *)calloc((MAP_SIZE * MAP_SIZE), sizeof(Material));
        CPUMemory += (MAP_SIZE * MAP_SIZE) * sizeof(Material);

        GroundTiles = (GroundTile *)calloc((MAP_SIZE * MAP_SIZE), sizeof(GroundTile));
        CPUMemory += (MAP_SIZE * MAP_SIZE) * sizeof(GroundTile);

        for (usize i = 0; i < MAP_SIZE; ++i)
        {
            for (usize j = 0; j < MAP_SIZE; ++j)
            {
                const usize Id = i * MAP_SIZE + j;

                const f32 X = i - MAP_SIZE / 2.0f + 0.5f;
                const f32 Y = 0.0f;
                const f32 Z = j - MAP_SIZE / 2.0f + 0.5f;

                // Create a model matrix for each data poto position it
                GroundTiles[Id].MatrixTransform = MatrixIdentity();

                const f32 Scale = 0.032;
                GroundTiles[Id].MatrixTransform = MatrixMultiply(GroundTiles[Id].MatrixTransform, MatrixScale(SQUARE_SIZE * Scale, SQUARE_SIZE * Scale, SQUARE_SIZE * Scale));
                GroundTiles[Id].MatrixTransform = MatrixMultiply(GroundTiles[Id].MatrixTransform, MatrixTranslate(X * SQUARE_SIZE, Y * SQUARE_SIZE, Z * SQUARE_SIZE));

                // Assign Random Material
                {
                    GroundMaterials[Id] = LoadMaterialDefault();

                    const i32 RandomGrassId = GetRandomValue(0, 3);

                    // Assign different grass materials to the ground
                    if (RandomGrassId == 0)
                    {
                        GroundMaterials[Id].maps[MATERIAL_MAP_DIFFUSE].texture = GrassTexture;
                    }
                    else if (RandomGrassId == 1)
                    {
                        GroundMaterials[Id].maps[MATERIAL_MAP_DIFFUSE].texture = GrassTexture02;
                    }
                    else if (RandomGrassId == 2)
                    {
                        GroundMaterials[Id].maps[MATERIAL_MAP_DIFFUSE].texture = GrassTexture03;
                    }
                    else if (RandomGrassId == 3)
                    {
                        GroundMaterials[Id].maps[MATERIAL_MAP_DIFFUSE].texture = GrassTexture04;
                    }

                    GroundMaterials[Id].maps[MATERIAL_MAP_DIFFUSE].color = WHITE;
                    GroundMaterials[Id].maps[MATERIAL_MAP_SPECULAR].value = 0.0f;

                    f32 shininess = 0.0f;
                    SetShaderValue(GroundMaterials[Id].shader, GetShaderLocation(GroundMaterials[Id].shader, "shininess"), &shininess, SHADER_UNIFORM_FLOAT);
                    GroundMaterials[Id].shader = CustomShader;

                    GroundTiles[Id].Mat = GroundMaterials[Id];
                    GroundTiles[Id].MaterialIndex = RandomGrassId;
                }
            } // j
        } // i
    } // block

    // 01
    i64 MaterialTargetIndex = 0;
    std::vector<GroundTile> Tiles01 = GetGroundTilesByMaterialIndex(GroundTiles, (MAP_SIZE * MAP_SIZE), MaterialTargetIndex);
    Mat01 = Tiles01.front().Mat;
    std::vector<Matrix> TransformsVector01 = GetMatricesByMaterialIndex(GroundTiles, (MAP_SIZE * MAP_SIZE), MaterialTargetIndex);
    Transforms01 = TransformsVector01.data();
    Transforms01Count = TransformsVector01.size();

    // 02
    MaterialTargetIndex = 1;
    std::vector<GroundTile> Tiles02 = GetGroundTilesByMaterialIndex(GroundTiles, (MAP_SIZE * MAP_SIZE), MaterialTargetIndex);
    Mat02 = Tiles02.front().Mat;
    std::vector<Matrix> TransformsVector02 = GetMatricesByMaterialIndex(GroundTiles, (MAP_SIZE * MAP_SIZE), MaterialTargetIndex);
    Transforms02 = TransformsVector02.data();
    Transforms02Count = TransformsVector02.size();

    // 03
    MaterialTargetIndex = 2;
    std::vector<GroundTile> Tiles03 = GetGroundTilesByMaterialIndex(GroundTiles, (MAP_SIZE * MAP_SIZE), MaterialTargetIndex);
    Mat03 = Tiles03.front().Mat;
    std::vector<Matrix> TransformsVector03 = GetMatricesByMaterialIndex(GroundTiles, (MAP_SIZE * MAP_SIZE), MaterialTargetIndex);
    Transforms03 = TransformsVector03.data();
    Transforms03Count = TransformsVector03.size();

    // 04
    MaterialTargetIndex = 3;
    std::vector<GroundTile> Tiles04 = GetGroundTilesByMaterialIndex(GroundTiles, (MAP_SIZE * MAP_SIZE), MaterialTargetIndex);
    Mat04 = Tiles04.front().Mat;
    std::vector<Matrix> TransformsVector04 = GetMatricesByMaterialIndex(GroundTiles, (MAP_SIZE * MAP_SIZE), MaterialTargetIndex);
    Transforms04 = TransformsVector04.data();
    Transforms04Count = TransformsVector04.size();

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
