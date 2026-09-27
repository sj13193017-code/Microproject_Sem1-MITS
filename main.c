//demo
#include"raylib.h"

int main(void) {
    InitWindow(800, 450, "Payroll Management System");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        BeginDrawing();
            ClearBackground(RAYWHITE);
            DrawText("Employee Payroll Management System", 190, 200, 20, BLACK);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}