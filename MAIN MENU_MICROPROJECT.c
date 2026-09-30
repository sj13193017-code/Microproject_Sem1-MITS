#include "raylib.h"

// Define screen states globally
typedef enum { MENU, ADD_SCREEN, VIEW_SCREEN } Screenstate;

int main() {
    // Initialize the application window
    InitWindow(800, 500, "Employee Payroll System - MITS MICROPROJECT");
    
    //settarget helps to find out how computer code handles speed vs how our eyes perceive motion.
    // FPS: frames per second 
    // Why 60? standar systems refresh their screens exctly 60 times a second
    SetTargetFPS(60);

    Screenstate currentScreen = MENU;
    
    // here ! is logical not operator in c (REMEMBER)
    while (!WindowShouldClose()) {
        
        // --- 1. FIXED INPUT CONTROLS ---
        if (currentScreen == MENU) {
            if (IsKeyPressed(KEY_A)) currentScreen = ADD_SCREEN;
            if (IsKeyPressed(KEY_V)) currentScreen = VIEW_SCREEN;
        } else {
            if (IsKeyPressed(KEY_M)) currentScreen = MENU;
        }

        // --- 2. THE VISUAL GRAPHICS ENGINE ---
        
        // BeginDrawing: everything you draw happens in an invisible sheet.
        BeginDrawing();
        // ClearBackground: Cleans the screen befor any new shape is drawn.
        // GetColour(HEXADECIMAL CODE): colour palette 
        ClearBackground(GetColor(0x1E1E24FF)); // Modern dark charcoal theme

        switch (currentScreen) {
            case MENU:
            // Top Color Banner Accent
                //DrawRectangle(X,Y,Width, Height, colour);
                DrawRectangle(0, 0, 800, 80, MAROON); 
                //DrawText(text, X, Y, Pixel size, Colour);
                DrawText("EMPLOYEE PAYROLL SYSTEM", 210, 25, 28, RAYWHITE);
                
            // Content Titles
                DrawText("WELCOME TO THE MAIN MENU", 230, 180, 22, GOLD); 
                DrawText("Select an operation mode to begin:", 245, 220, 16, LIGHTGRAY);
                
            // Structured Menu Item Layout
                DrawRectangle(200, 280, 400, 45, GetColor(0x2D2D34FF));
                DrawText("Press [ A ] -> Add New Employee", 225, 292, 18, GREEN);
                
                DrawRectangle(200, 350, 400, 45, GetColor(0x2D2D34FF));
                DrawText("Press [ V ] -> View Database Logs", 225, 362, 18, SKYBLUE);
                break;

            case ADD_SCREEN:
            // Add Screen Layout
                DrawRectangle(0, 0, 800, 60, GREEN);
                DrawText("ADD NEW EMPLOYEE MODE", 270, 18, 20, BLACK);
                
                DrawText("Screen working successfully.", 260, 220, 20, RAYWHITE);
                DrawText("Press [ M ] to return to the Main Menu", 215, 300, 18, LIGHTGRAY);
                break;

            case VIEW_SCREEN:
            // View Screen Layout
                DrawRectangle(0, 0, 800, 60, SKYBLUE);
                DrawText("VIEW DATABASE RECORDS MODE", 240, 18, 20, BLACK);
                
                DrawText("Screen working successfully.", 260, 220, 20, RAYWHITE);
                DrawText("Press [ M ] to return to the Main Menu", 215, 300, 18, LIGHTGRAY);
                break;
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
