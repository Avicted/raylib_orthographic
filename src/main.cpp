// Defines ---------------------------------------------------
#define RLIGHTS_IMPLEMENTATION

// Includes --------------------------------------------------
#include "includes.h"
#include "raylib_includes.h"

// Variables -------------------------------------------------
i32 SCREEN_WIDTH = 640 * 2;
i32 SCREEN_HEIGHT = 360 * 2;

bool Debug = false;
const i64 MAP_SIZE = 256;
const i64 SQUARE_SIZE = 32;

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
const f64 CameraAngle = 0.0;
const f64 CameraAndgleCos = cos(CameraAngle * DEG2RAD);
const f64 CameraAndgleSin = sin(CameraAngle * DEG2RAD);
const Vector3 CameraStartPosition = (Vector3){90.0 * 2.0, 180.0 * 2, 90.0 * 2.0};

Camera3D DebugCamera = {};
const Vector3 DebugCameraStartPosition = (Vector3){90.0f * 2.0, 180.0f * 2.0, 90.0f * 2.0};

// Types -----------------------------------------------------
struct GroundTile
{
    i64 Id;

    // 3D position
    Matrix MatrixTransform;

    usize MaterialIndex;
    Material Mat;

    // Optional: Add a bounding box for frustum culling
    BoundingBox BoundingVolume;

    f32 width;
    f32 depth;
    f32 height; // Height of the tile (if applicable)
};

GroundTile *GroundTiles = NULL;

GroundTile *SelectedGroundTile = NULL;
// Display information about closest hit
RayCollision collision = {0};
char *hitObjectName = "None";
Ray ray = {0}; // Picking ray

struct Plane
{
    Vector3 normal; // Normal of the plane
    f32 distance;   // Distance from the origin to the plane
};

struct Frustum
{
    Plane planes[6]; // Define the 6 planes of the frustum
};

// Instanced rendering specific data ------------------------
Material Mat01;
Material Mat02;
Material Mat03;
Material Mat04;

// Lists to store the transforms of tiles in view for each material
std::vector<Matrix> TransformsInView01;
std::vector<Matrix> TransformsInView02;
std::vector<Matrix> TransformsInView03;
std::vector<Matrix> TransformsInView04;
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

internal Vector2
GetTileCoordsUnderMouse(RayCollision groundHitInfo, Camera3D camera)
{
    Vector2 tileCoords = {-1, -1}; // Initialize to out-of-bounds

    // World space collision point on the ground
    Vector3 hitPosition = collision.point;

    // Convert to local grid coordinates by adjusting for map offset and tile size
    f64 localX = hitPosition.x * CameraAndgleCos - hitPosition.z * CameraAndgleSin + (MAP_SIZE * SQUARE_SIZE) / 2.0f;
    f64 localZ = hitPosition.x * CameraAndgleSin + hitPosition.z * CameraAndgleCos + (MAP_SIZE * SQUARE_SIZE) / 2.0f;

    // Calculate tile indices (floor to get correct tile)
    tileCoords.x = floor(localX / SQUARE_SIZE);
    tileCoords.y = floor(localZ / SQUARE_SIZE);

    return tileCoords;
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

    // Set the hovered tile
    {
        // Reset the collision info
        hitObjectName = "None";
        ray = GetMouseRay(GetMousePosition(), MainCamera);

        // Initialize collision distance to a large value so the closest hit is recorded
        collision.distance = FLT_MAX;
        SelectedGroundTile = NULL;

        // Iterate through each GroundTile and check for ray collision
        // @Todo(Victor): Only iterate the visible tiles
        for (int i = 0; i < MAP_SIZE * MAP_SIZE; i++)
        {
            // Transform the bounding box of the current tile
            BoundingBox transformedBox;
            transformedBox.min = Vector3Transform(GroundTiles[i].BoundingVolume.min, GroundTiles[i].MatrixTransform);
            transformedBox.max = Vector3Transform(GroundTiles[i].BoundingVolume.max, GroundTiles[i].MatrixTransform);

            // Perform the ray collision check with the transformed bounding box
            RayCollision tileHitInfo = GetRayCollisionBox(ray, transformedBox);
            if (tileHitInfo.hit && tileHitInfo.distance < collision.distance)
            {
                // Update collision information and selected tile
                collision = tileHitInfo;
                hitObjectName = "Ground";
                SelectedGroundTile = &GroundTiles[i];
            }
        }

        if (Debug)
        {
            printf("Hit Tile ID: %lld, Distance: %f\n", (SelectedGroundTile != NULL) ? SelectedGroundTile->Id : -1, collision.distance);
        }

        // Check if a tile was hit
        if (SelectedGroundTile != NULL)
        {
            Vector2 tileCoords = GetTileCoordsUnderMouse(collision, MainCamera);
            i64 Id = static_cast<i64>(tileCoords.x) * MAP_SIZE + static_cast<i64>(tileCoords.y);

            if (tileCoords.x >= 0 && tileCoords.y >= 0 &&
                tileCoords.x < MAP_SIZE && tileCoords.y < MAP_SIZE)
            {
                SelectedGroundTile = &GroundTiles[Id];
            }
            else
            {
                SelectedGroundTile = NULL;
            }
        }
    }

    // Zoom out
    if (GetMouseWheelMove() < 0 && MainCamera.position.y <= 890.0f)
    {
        MainCamera.fovy += 5.0f;  // Increase fovy to zoom out
        DebugCamera.fovy += 5.0f; // Increase fovy to zoom out

        // Only move Y axis
        MainCamera.position = (Vector3){MainCamera.position.x, MainCamera.position.y + 80.0f, MainCamera.position.z};
        DebugCamera.position = (Vector3){DebugCamera.position.x, DebugCamera.position.y + 80.0f, DebugCamera.position.z};
    }
    // Zoom in
    else if (GetMouseWheelMove() > 0 && MainCamera.position.y >= 180.0f)
    {
        MainCamera.fovy -= 5.0f;  // Decrease fovy to zoom in
        DebugCamera.fovy -= 5.0f; // Decrease fovy to zoom in

        // Only move Y axis
        MainCamera.position = (Vector3){MainCamera.position.x, MainCamera.position.y - 80.0f, MainCamera.position.z};
        DebugCamera.position = (Vector3){DebugCamera.position.x, DebugCamera.position.y - 80.0f, DebugCamera.position.z};
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

    if (DebugCamera.fovy < 10.0f)
    {
        DebugCamera.fovy = 10.0f;
    }
    if (DebugCamera.fovy > 1000.0f)
    {
        DebugCamera.fovy = 1000.0f;
    }

    f32 CameraSpeed = 1.0f;

    // The camera speed is increased the higher the camera is (Y-axis)
    CameraSpeed += MainCamera.position.y / 512.0f;

    if (IsMouseButtonDown(MOUSE_RIGHT_BUTTON))
    {
        Vector2 mouseDelta = GetMouseDelta();
        f64 speedMultiplier = CameraSpeed;

        DisableCursor();

        // Calculate the movement delta based on a 45-degree angle
        f64 deltaX = (mouseDelta.y + mouseDelta.x) * speedMultiplier; // cos(45 degrees) = sin(45 degrees) = 0.7071
        f64 deltaZ = (mouseDelta.y - mouseDelta.x) * speedMultiplier;

        // Move the camera based on the calculated delta
        MainCamera.position.x += deltaX;
        MainCamera.position.z += deltaZ;

        // Update the camera target to maintain focus
        MainCamera.target.x += deltaX;
        MainCamera.target.z += deltaZ;

        // Update the debug camera position and target by adding the same delta
        DebugCamera.position.x += deltaX;
        DebugCamera.position.z += deltaZ;

        DebugCamera.target.x += deltaX;
        DebugCamera.target.z += deltaZ;
    }
    else if (IsMouseButtonReleased(MOUSE_RIGHT_BUTTON))
    {
        EnableCursor();
    }

    if (Debug)
    {
        if (IsKeyDown(KEY_W))
        {
            Vector3 forward = Vector3Normalize(Vector3Subtract(DebugCamera.target, DebugCamera.position));
            DebugCamera.position = Vector3Add(DebugCamera.position, Vector3Scale(forward, CameraSpeed));
            DebugCamera.target = Vector3Add(DebugCamera.target, Vector3Scale(forward, CameraSpeed));
        }
        if (IsKeyDown(KEY_S))
        {
            Vector3 forward = Vector3Normalize(Vector3Subtract(DebugCamera.target, DebugCamera.position));
            DebugCamera.position = Vector3Subtract(DebugCamera.position, Vector3Scale(forward, CameraSpeed));
            DebugCamera.target = Vector3Subtract(DebugCamera.target, Vector3Scale(forward, CameraSpeed));
        }
        if (IsKeyDown(KEY_A))
        {
            Vector3 right = Vector3CrossProduct(Vector3Normalize(Vector3Subtract(DebugCamera.target, DebugCamera.position)), DebugCamera.up);
            DebugCamera.position = Vector3Subtract(DebugCamera.position, Vector3Scale(right, CameraSpeed));
            DebugCamera.target = Vector3Subtract(DebugCamera.target, Vector3Scale(right, CameraSpeed));
        }
        if (IsKeyDown(KEY_D))
        {
            Vector3 right = Vector3CrossProduct(Vector3Normalize(Vector3Subtract(DebugCamera.target, DebugCamera.position)), DebugCamera.up);
            DebugCamera.position = Vector3Add(DebugCamera.position, Vector3Scale(right, CameraSpeed));
            DebugCamera.target = Vector3Add(DebugCamera.target, Vector3Scale(right, CameraSpeed));
        }

        // Up down left shift and ctrl (Debug Camera)
        if (IsKeyDown(KEY_LEFT_SHIFT))
        {
            DebugCamera.position.y += 8.0f;
            DebugCamera.target.y += 8.0f;
        }
        if (IsKeyDown(KEY_LEFT_CONTROL))
        {
            DebugCamera.position.y -= 8.0f;
            DebugCamera.target.y -= 8.0f;
        }
    }
}

internal void
CalculateBoundingBox(GroundTile *tile)
{
    const f32 halfWidth = tile->width / 2.0f;
    const f32 halfDepth = tile->depth / 2.0f;
    const f32 halfHeight = tile->height / 2.0f; // If you want to center the height

    // Define the corners of the box in local space
    Vector3 localCorners[8] = {
        {-halfWidth, -halfHeight, -halfDepth}, // Bottom-left
        {halfWidth, -halfHeight, -halfDepth},  // Bottom-right
        {-halfWidth, -halfHeight, halfDepth},  // Top-left
        {halfWidth, -halfHeight, halfDepth},   // Top-right
        {-halfWidth, halfHeight, -halfDepth},  // Upper-left
        {halfWidth, halfHeight, -halfDepth},   // Upper-right
        {-halfWidth, halfHeight, halfDepth},   // Lower-left
        {halfWidth, halfHeight, halfDepth}     // Upper-right
    };

    // Apply the matrix transform to each corner
    Vector3 transformedCorners[8];
    for (i32 i = 0; i < 8; ++i)
    {
        transformedCorners[i] = Vector3Transform(localCorners[i], tile->MatrixTransform);
    }

    // Initialize the bounding box
    tile->BoundingVolume.min = transformedCorners[0];
    tile->BoundingVolume.max = transformedCorners[0];

    // Update min/max based on transformed corners
    for (i32 i = 1; i < 8; ++i)
    {
        tile->BoundingVolume.min = Vector3Min(tile->BoundingVolume.min, transformedCorners[i]);
        tile->BoundingVolume.max = Vector3Max(tile->BoundingVolume.max, transformedCorners[i]);
    }
}

internal int
IsBoxInFrustum(const Frustum *frustum, const BoundingBox *box)
{
    for (i32 i = 0; i < 6; ++i)
    {
        Vector3 normal = frustum->planes[i].normal;
        f32 distance = frustum->planes[i].distance;

        // Find the most positive vertex (the farthest point along the normal)
        Vector3 positiveVertex = (Vector3){
            (normal.x > 0.0f) ? box->max.x : box->min.x,
            (normal.y > 0.0f) ? box->max.y : box->min.y,
            (normal.z > 0.0f) ? box->max.z : box->min.z};

        // Find the most negative vertex (the nearest point along the normal)
        Vector3 negativeVertex = (Vector3){
            (normal.x < 0.0f) ? box->max.x : box->min.x,
            (normal.y < 0.0f) ? box->max.y : box->min.y,
            (normal.z < 0.0f) ? box->max.z : box->min.z};

        // If the positive vertex is behind the plane, the box is outside
        if ((normal.x * positiveVertex.x + normal.y * positiveVertex.y + normal.z * positiveVertex.z + distance) < 0.0f)
        {
            return 0; // Box is outside the frustum
        }
    }

    return 1; // Box is inside or intersects the frustum
}

internal Frustum
CalculateFrustum(Camera3D camera)
{
    Frustum frustum;
    Matrix viewMatrix = MatrixLookAt(camera.position, camera.target, camera.up);
    f32 aspect = (f32)GetScreenWidth() / (f32)GetScreenHeight();
    f32 top = tanf(camera.fovy * 0.5f * DEG2RAD);
    f32 right = top * aspect;
    Matrix projectionMatrix = MatrixPerspective(camera.fovy * DEG2RAD, aspect, 0.01f, 4000.0f);
    Matrix viewProjMatrix = MatrixMultiply(viewMatrix, projectionMatrix);

    // Extract frustum planes (left, right, bottom, top, near, far)
    frustum.planes[0] = (Plane){.normal = {viewProjMatrix.m3 + viewProjMatrix.m0, viewProjMatrix.m7 + viewProjMatrix.m4, viewProjMatrix.m11 + viewProjMatrix.m8}, .distance = viewProjMatrix.m15 + viewProjMatrix.m12};  // Left
    frustum.planes[1] = (Plane){.normal = {viewProjMatrix.m3 - viewProjMatrix.m0, viewProjMatrix.m7 - viewProjMatrix.m4, viewProjMatrix.m11 - viewProjMatrix.m8}, .distance = viewProjMatrix.m15 - viewProjMatrix.m12};  // Right
    frustum.planes[2] = (Plane){.normal = {viewProjMatrix.m3 + viewProjMatrix.m1, viewProjMatrix.m7 + viewProjMatrix.m5, viewProjMatrix.m11 + viewProjMatrix.m9}, .distance = viewProjMatrix.m15 + viewProjMatrix.m13};  // Bottom
    frustum.planes[3] = (Plane){.normal = {viewProjMatrix.m3 - viewProjMatrix.m1, viewProjMatrix.m7 - viewProjMatrix.m5, viewProjMatrix.m11 - viewProjMatrix.m9}, .distance = viewProjMatrix.m15 - viewProjMatrix.m13};  // Top
    frustum.planes[4] = (Plane){.normal = {viewProjMatrix.m3 + viewProjMatrix.m2, viewProjMatrix.m7 + viewProjMatrix.m6, viewProjMatrix.m11 + viewProjMatrix.m10}, .distance = viewProjMatrix.m15 + viewProjMatrix.m14}; // Near
    frustum.planes[5] = (Plane){.normal = {viewProjMatrix.m3 - viewProjMatrix.m2, viewProjMatrix.m7 - viewProjMatrix.m6, viewProjMatrix.m11 - viewProjMatrix.m10}, .distance = viewProjMatrix.m15 - viewProjMatrix.m14}; // Far

    return frustum;
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
                                              (f32)GetScreenWidth() / (f32)GetScreenHeight(),
                                              0.01f, 4000.0f);
        rlSetMatrixProjection(projection);
    }

    // Center of the world
    Frustum cameraFrustum = CalculateFrustum(MainCamera); // Define and calculate the camera frustum here
    u64 InViewCount = 0;

    // Render wires for the cameraFrustum
    if (Debug)
    {
        // DrawCubeWires(MainCamera.position, 32.0f, 32.0f, 32.0f, RED);
        // DrawCube(MainCamera.position, 32.0f, 32.0f, 32.0f, Fade(RED, 0.1f));

        // Draw the Debug camera frustum
        for (i32 i = 0; i < 6; ++i)
        {
            DrawLine3D((Vector3){cameraFrustum.planes[i].normal.x, cameraFrustum.planes[i].normal.y, cameraFrustum.planes[i].normal.z},
                       (Vector3){cameraFrustum.planes[i].normal.x * 1000.0f, cameraFrustum.planes[i].normal.y * 1000.0f, cameraFrustum.planes[i].normal.z * 1000.0f},
                       GREEN);
        }
    }

    // Center of the world a test cube
    DrawCube((Vector3){0.0f, 32.0f, 0.0f}, 64.0f, 64.0f, 64.0f, RED);

    for (usize i = 0; i < MAP_SIZE; ++i)
    {
        for (usize j = 0; j < MAP_SIZE; ++j)
        {
            const usize Id = i * MAP_SIZE + j;

            // Calculate the bounding box for each tile
            // CalculateBoundingBox(&GroundTiles[Id]);

            // Check if the tile is in the frustum
            if (IsBoxInFrustum(&cameraFrustum, &GroundTiles[Id].BoundingVolume))
            {
                // Add the tile's transform to the appropriate list based on its material
                if (GroundTiles[Id].MaterialIndex == 0)
                {
                    TransformsInView01.push_back(GroundTiles[Id].MatrixTransform);
                }
                else if (GroundTiles[Id].MaterialIndex == 1)
                {
                    TransformsInView02.push_back(GroundTiles[Id].MatrixTransform);
                }
                else if (GroundTiles[Id].MaterialIndex == 2)
                {
                    TransformsInView03.push_back(GroundTiles[Id].MatrixTransform);
                }
                else if (GroundTiles[Id].MaterialIndex == 3)
                {
                    TransformsInView04.push_back(GroundTiles[Id].MatrixTransform);
                }

                InViewCount++;
            }

            // Draw the bounding box for each tile
            /*if (Debug)
            {
                DrawBoundingBox(GroundTiles[Id].BoundingVolume, RED);
            }*/
        }
    }

    // Batch render the tiles for each material
    if (!TransformsInView01.empty())
    {
        DrawMeshInstanced(GroundMesh, Mat01, TransformsInView01.data(), TransformsInView01.size());
    }
    if (!TransformsInView02.empty())
    {
        DrawMeshInstanced(GroundMesh, Mat02, TransformsInView02.data(), TransformsInView02.size());
    }
    if (!TransformsInView03.empty())
    {
        DrawMeshInstanced(GroundMesh, Mat03, TransformsInView03.data(), TransformsInView03.size());
    }
    if (!TransformsInView04.empty())
    {
        DrawMeshInstanced(GroundMesh, Mat04, TransformsInView04.data(), TransformsInView04.size());
    }

    // Highlight the selected tile
    if (SelectedGroundTile != NULL)
    {
        if (collision.hit)
        {
            // Calculate the position for the magenta cube based on the mouse position
            Vector3 cursorPosition = collision.point; // This is where the ray hit
            cursorPosition.y += 5.0f;                 // Offset it above the ground

            DrawCube(cursorPosition, 10.0f, 10.0f, 10.0f, MAGENTA);
            DrawCubeWires(cursorPosition, 10.0f, 10.0f, 10.0f, WHITE);

            Vector3 normalEnd = {
                collision.point.x + collision.normal.x,
                collision.point.y + collision.normal.y,
                collision.point.z + collision.normal.z};

            DrawLine3D(collision.point, normalEnd, RED);

            // Highlight the selected tile
            DrawCubeWires(Vector3Transform((Vector3){0.0f, 0.0f, 0.0f}, SelectedGroundTile->MatrixTransform), SelectedGroundTile->width, SelectedGroundTile->height, SelectedGroundTile->depth, WHITE);
        }

        DrawRay(ray, MAROON);
    }

    // DrawGrid(MAP_SIZE, SQUARE_SIZE);

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

    // if IsBoxInFrustum(&cameraFrustum, &GroundTiles[0].BoundingVolume)
    if (IsBoxInFrustum(&cameraFrustum, &GroundTiles[0].BoundingVolume))
    {
        DrawTextEx(MainFont, "Tile 01 is in the frustum", {10, 96}, 16, 2, BLACK);
        DrawTextEx(MainFont, "Tile 01 is in the frustum", {13, 99}, 16, 2, WHITE);
    }

    DrawTextEx(MainFont, TextFormat("In View Count: %i", InViewCount), {10, 118}, 32, 2, BLACK);
    DrawTextEx(MainFont, TextFormat("In View Count: %i", InViewCount), {13, 121}, 32, 2, WHITE);

    DrawTextEx(MainFont, TextFormat("MainCamera.position: %f, %f, %f", MainCamera.position.x, MainCamera.position.y, MainCamera.position.z), {10, 160}, 16, 2, BLACK);
    DrawTextEx(MainFont, TextFormat("MainCamera.position: %f, %f, %f", MainCamera.position.x, MainCamera.position.y, MainCamera.position.z), {13, 163}, 16, 2, WHITE);

    DrawTextEx(MainFont, TextFormat("DebugCamera.position: %f, %f, %f", DebugCamera.position.x, DebugCamera.position.y, DebugCamera.position.z), {10, 192}, 16, 2, BLACK);
    DrawTextEx(MainFont, TextFormat("DebugCamera.position: %f, %f, %f", DebugCamera.position.x, DebugCamera.position.y, DebugCamera.position.z), {13, 195}, 16, 2, WHITE);

    if (SelectedGroundTile != NULL)
    {
        DrawTextEx(MainFont, TextFormat("Selected Tile: %i", SelectedGroundTile->MaterialIndex), {10, 224}, 16, 2, BLACK);
        DrawTextEx(MainFont, TextFormat("Selected Tile: %i", SelectedGroundTile->MaterialIndex), {13, 227}, 16, 2, WHITE);

        // SelectedGroundTile transform position
        DrawTextEx(MainFont, TextFormat("Selected Tile Position: %f, %f, %f", SelectedGroundTile->MatrixTransform.m12, SelectedGroundTile->MatrixTransform.m13, SelectedGroundTile->MatrixTransform.m14), {10, 256}, 16, 2, BLACK);
        DrawTextEx(MainFont, TextFormat("Selected Tile Position: %f, %f, %f", SelectedGroundTile->MatrixTransform.m12, SelectedGroundTile->MatrixTransform.m13, SelectedGroundTile->MatrixTransform.m14), {13, 259}, 16, 2, WHITE);

        // Draw some debug GUI text
        DrawTextEx(MainFont, TextFormat("Hit Object: %s", hitObjectName), (Vector2){10, 288}, 20, 2, BLACK);
        DrawTextEx(MainFont, TextFormat("Hit Object: %s", hitObjectName), (Vector2){13, 291}, 20, 2, WHITE);

        DrawTextEx(MainFont, TextFormat("Hit Distance: %f", collision.distance), (Vector2){10, 320}, 20, 2, BLACK);
        DrawTextEx(MainFont, TextFormat("Hit Distance: %f", collision.distance), (Vector2){13, 323}, 20, 2, WHITE);
    }
    else
    {
        DrawTextEx(MainFont, "No Tile Selected", {10, 224}, 16, 2, BLACK);
        DrawTextEx(MainFont, "No Tile Selected", {13, 227}, 16, 2, WHITE);
    }

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
    MainCamera.fovy = 75.0f;
    MainCamera.projection = CAMERA_PERSPECTIVE;

    DebugCamera.position = DebugCameraStartPosition;
    DebugCamera.target = {0.0f, 0.0f, 0.0f};
    DebugCamera.up = {0.0f, 1.0f, 0.0f};
    DebugCamera.fovy = 75.0f;
    DebugCamera.projection = CAMERA_PERSPECTIVE;

    // Raylib setup ---------------------------------------------------
    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "raylib_orthographic");

    SetTargetFPS(144);
    SetWindowState(FLAG_VSYNC_HINT);
    // ----------------------------------------------------------------

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

                GroundTiles[Id].Id = Id;

                const f32 X = i - MAP_SIZE / 2.0f + 0.5f;
                const f32 Y = 0.0f;
                const f32 Z = j - MAP_SIZE / 2.0f + 0.5f;

                // Create a model matrix for each data poto position it
                GroundTiles[Id].MatrixTransform = MatrixIdentity();

                const f64 Scale = 0.03150;
                GroundTiles[Id].MatrixTransform = MatrixMultiply(GroundTiles[Id].MatrixTransform, MatrixScale(SQUARE_SIZE * Scale, SQUARE_SIZE * Scale, SQUARE_SIZE * Scale));

                GroundTiles[Id].MatrixTransform = MatrixMultiply(GroundTiles[Id].MatrixTransform, MatrixRotate((Vector3){0.0f, 1.0f, 0.0f}, 45.0f * DEG2RAD));

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

                // Bounding box from the GroundTiles[Id].MatrixTransform and the width, depth, height
                GroundTiles[Id].width = 1.0f * SQUARE_SIZE;
                GroundTiles[Id].depth = 1.0f * SQUARE_SIZE;
                GroundTiles[Id].height = 0.1f;

                GroundTiles[Id].BoundingVolume.min = (Vector3){-0.5f * SQUARE_SIZE, 0.0f * SQUARE_SIZE, -0.5f * SQUARE_SIZE};
                GroundTiles[Id].BoundingVolume.max = (Vector3){0.5f * SQUARE_SIZE, 0.0f * SQUARE_SIZE, 0.5f * SQUARE_SIZE};

                CalculateBoundingBox(&GroundTiles[Id]);
            } // j
        } // i
    } // block

    // 01
    i64 MaterialTargetIndex = 0;
    std::vector<GroundTile> Tiles01 = GetGroundTilesByMaterialIndex(GroundTiles, (MAP_SIZE * MAP_SIZE), MaterialTargetIndex);
    Mat01 = Tiles01.front().Mat;

    // 02
    MaterialTargetIndex = 1;
    std::vector<GroundTile> Tiles02 = GetGroundTilesByMaterialIndex(GroundTiles, (MAP_SIZE * MAP_SIZE), MaterialTargetIndex);
    Mat02 = Tiles02.front().Mat;

    // 03
    MaterialTargetIndex = 2;
    std::vector<GroundTile> Tiles03 = GetGroundTilesByMaterialIndex(GroundTiles, (MAP_SIZE * MAP_SIZE), MaterialTargetIndex);
    Mat03 = Tiles03.front().Mat;

    // 04
    MaterialTargetIndex = 3;
    std::vector<GroundTile> Tiles04 = GetGroundTilesByMaterialIndex(GroundTiles, (MAP_SIZE * MAP_SIZE), MaterialTargetIndex);
    Mat04 = Tiles04.front().Mat;

    // Rotated 45 degrees
    GroundMesh = GenMeshPlane(SQUARE_SIZE, SQUARE_SIZE, 1, 1);
    Matrix rotationMatrix = MatrixRotate((Vector3){0.0f, 1.0f, 0.0f}, 45.0f * DEG2RAD);
    for (usize i = 0; i < MAP_SIZE; ++i)
    {
        for (usize j = 0; j < MAP_SIZE; ++j)
        {
            const usize Id = i * MAP_SIZE + j;
            GroundTiles[Id].MatrixTransform = MatrixMultiply(rotationMatrix, GroundTiles[Id].MatrixTransform);
        }
    }

    collision.distance = FLT_MAX;
    collision.hit = false;

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
