#include <stdio.h>
#include <raylib.h>

int main()
{
    InitWindow(800, 800, "test");
    SetTargetFPS(60);

    Font font = LoadFontEx("./font/UbuntuMono-Regular.ttf", 18, 0, 250);
    
    while (!WindowShouldClose())
    {
        ClearBackground(WHITE);

    }
    CloseWindow();
    return 0;
}