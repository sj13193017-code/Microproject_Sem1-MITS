
 /*
 * EMPLOYEE PAYROLL MANAGEMENT SYSTEM
 * Language : C
 * Library  : raylib (for the graphical window)
 *
 * Features:
 *   1. Add employee
 *   2. View / search employees
 *   3. Edit employee
 *   4. Delete employee
 *   5. Payslip generation
 *   6. Payroll summary
 *   7. Data saved in a file (employees.dat)
 *
 * Salary rules (change them as you like):
 *   Earned Basic = Basic / 30 * Days Present
 *   HRA = 20% of earned basic
 *   DA  = 10% of earned basic
 *   Gross = Earned Basic + HRA + DA + Bonus
 *   PF  = 12% of earned basic
 *   Tax = 0% if gross < 25000, 5% if gross < 50000, else 10%
 *   Net Salary = Gross - PF - Tax
 *
 * Compile (Linux):
 *   gcc payroll_system.c -o payroll -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
 * Compile (Windows, MinGW):
 *   gcc payroll_system.c -o payroll.exe -lraylib -lopengl32 -lgdi32 -lwinmm
 * Compile (macOS):
 *   gcc payroll_system.c -o payroll -lraylib -framework OpenGL -framework Cocoa -framework IOKit
 */

#include "raylib.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define MAX_EMP   100
#define FILE_NAME "employees.dat"
#define SCREEN_W  900
#define SCREEN_H  600

/* ---------------- Data structures ---------------- */

typedef struct {
    int   id;
    char  name[30];
    char  dept[20];
    float basic;
    float bonus;
    int   daysPresent;      /* out of 30 */
} Employee;

typedef struct {
    float earned, hra, da, gross, pf, tax, net;
} Payslip;

typedef enum {
    SCR_MENU,
    SCR_ADD,
    SCR_LIST,
    SCR_PAYSLIP,
    SCR_SUMMARY
} Screen;

/* ---------------- Global variables ---------------- */

Employee emp[MAX_EMP];
int      count = 0;

Screen screen = SCR_MENU;
bool   exitApp = false;

/* Add / Edit form fields */
char fId[6], fName[30], fDept[20], fBasic[11], fBonus[11], fDays[3];
char *fieldBuf[6]         = { fId, fName, fDept, fBasic, fBonus, fDays };
const char *fieldLabel[6] = { "Employee ID", "Name", "Department",
                              "Basic Salary (monthly)", "Bonus", "Days Present (0-30)" };
int  fieldMax[6]  = { 5, 28, 18, 10, 10, 2 };
int  fieldMode[6] = { 1, 0, 0, 2, 2, 1 };   /* 0=text, 1=integer, 2=decimal */
int  activeField  = 0;
int  editIndex    = -1;                     /* -1 means adding a new employee */

/* List screen */
char searchBuf[30] = "";
int  selected = -1;
int  scroll   = 0;

/* Message shown at bottom of the screen */
char  msg[100] = "";
Color msgColor = DARKGREEN;

/* ---------------- File handling ---------------- */

void LoadData(void)
{
    FILE *fp = fopen(FILE_NAME, "rb");
    if (fp == NULL) return;               /* no file yet */
    count = (int)fread(emp, sizeof(Employee), MAX_EMP, fp);
    fclose(fp);
}

void SaveData(void)
{
    FILE *fp = fopen(FILE_NAME, "wb");
    if (fp == NULL) return;
    fwrite(emp, sizeof(Employee), count, fp);
    fclose(fp);
}

/* ---------------- Helper functions ---------------- */

void SetMsg(const char *text, Color c)
{
    strncpy(msg, text, sizeof(msg) - 1);
    msg[sizeof(msg) - 1] = '\0';
    msgColor = c;
}

Payslip Calculate(Employee e)
{
    Payslip p;
    float rate;

    p.earned = e.basic / 30.0f * e.daysPresent;
    p.hra    = p.earned * 0.20f;
    p.da     = p.earned * 0.10f;
    p.gross  = p.earned + p.hra + p.da + e.bonus;
    p.pf     = p.earned * 0.12f;

    if (p.gross < 25000)      rate = 0.0f;
    else if (p.gross < 50000) rate = 0.05f;
    else                      rate = 0.10f;

    p.tax = p.gross * rate;
    p.net = p.gross - p.pf - p.tax;
    return p;
}

bool ContainsIgnoreCase(const char *text, const char *word)
{
    int n = (int)strlen(text), w = (int)strlen(word);
    for (int i = 0; i + w <= n; i++) {
        int j = 0;
        while (j < w && tolower((unsigned char)text[i + j]) == tolower((unsigned char)word[j]))
            j++;
        if (j == w) return true;
    }
    return false;
}

int FindById(int id)
{
    for (int i = 0; i < count; i++)
        if (emp[i].id == id) return i;
    return -1;
}

/* Reads typed characters into buf. mode: 0 text, 1 integer, 2 decimal */
void HandleTyping(char *buf, int maxLen, int mode)
{
    int key = GetCharPressed();
    while (key > 0) {
        int len = (int)strlen(buf);
        bool ok;
        if (mode == 0)      ok = (key >= 32 && key <= 125);
        else if (mode == 1) ok = (key >= '0' && key <= '9');
        else                ok = ((key >= '0' && key <= '9') || key == '.');

        if (ok && len < maxLen) {
            buf[len]     = (char)key;
            buf[len + 1] = '\0';
        }
        key = GetCharPressed();
    }
    if (IsKeyPressed(KEY_BACKSPACE)) {
        int len = (int)strlen(buf);
        if (len > 0) buf[len - 1] = '\0';
    }
}

/* Draws a button and returns true if it was clicked */
bool Button(Rectangle r, const char *text)
{
    bool hover = CheckCollisionPointRec(GetMousePosition(), r);
    DrawRectangleRec(r, hover ? SKYBLUE : LIGHTGRAY);
    DrawRectangleLinesEx(r, 2, DARKGRAY);
    int w = MeasureText(text, 20);
    DrawText(text, (int)(r.x + (r.width - w) / 2), (int)(r.y + (r.height - 20) / 2), 20, BLACK);
    return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

void ClearForm(void)
{
    for (int i = 0; i < 6; i++) fieldBuf[i][0] = '\0';
    activeField = 0;
    editIndex = -1;
}

/* ---------------- Employee operations ---------------- */

void SaveEmployee(void)
{
    if (fId[0] == '\0' || fName[0] == '\0' || fDept[0] == '\0' ||
        fBasic[0] == '\0' || fDays[0] == '\0') {
        SetMsg("Please fill all fields (bonus can be empty).", RED);
        return;
    }

    int   id    = atoi(fId);
    float basic = (float)atof(fBasic);
    float bonus = (float)atof(fBonus);
    int   days  = atoi(fDays);

    if (id <= 0)                { SetMsg("ID must be greater than 0.", RED); return; }
    if (basic <= 0)             { SetMsg("Basic salary must be greater than 0.", RED); return; }
    if (days < 0 || days > 30)  { SetMsg("Days present must be between 0 and 30.", RED); return; }

    int found = FindById(id);
    if (found != -1 && found != editIndex) {
        SetMsg("This employee ID already exists!", RED);
        return;
    }

    Employee e;
    e.id = id;
    strcpy(e.name, fName);
    strcpy(e.dept, fDept);
    e.basic = basic;
    e.bonus = bonus;
    e.daysPresent = days;

    if (editIndex >= 0) {
        emp[editIndex] = e;
        SetMsg("Employee updated successfully.", DARKGREEN);
    } else {
        if (count >= MAX_EMP) { SetMsg("Employee list is full!", RED); return; }
        emp[count++] = e;
        SetMsg("Employee added successfully.", DARKGREEN);
    }
    SaveData();
    ClearForm();
    screen = SCR_LIST;
}

void DeleteEmployee(int index)
{
    for (int i = index; i < count - 1; i++)
        emp[i] = emp[i + 1];
    count--;
    selected = -1;
    SaveData();
    SetMsg("Employee deleted.", DARKGREEN);
}

void LoadEmployeeToForm(int index)
{
    sprintf(fId, "%d", emp[index].id);
    strcpy(fName, emp[index].name);
    strcpy(fDept, emp[index].dept);
    sprintf(fBasic, "%.2f", emp[index].basic);
    sprintf(fBonus, "%.2f", emp[index].bonus);
    sprintf(fDays, "%d", emp[index].daysPresent);
    editIndex = index;
    activeField = 0;
}

/* ---------------- Screens ---------------- */

void DrawMessage(void)
{
    if (msg[0] != '\0')
        DrawText(msg, 20, SCREEN_H - 30, 20, msgColor);
}

void ScreenMenu(void)
{
    const char *title = "EMPLOYEE PAYROLL SYSTEM";
    const char *sub   = "Simple payroll management in C with raylib";
    DrawText(title, (SCREEN_W - MeasureText(title, 40)) / 2, 70, 40, DARKBLUE);
    DrawText(sub, (SCREEN_W - MeasureText(sub, 20)) / 2, 125, 20, GRAY);

    if (Button((Rectangle){ 300, 190, 300, 50 }, "Add Employee")) {
        ClearForm();
        msg[0] = '\0';
        screen = SCR_ADD;
    }
    if (Button((Rectangle){ 300, 260, 300, 50 }, "View / Search Employees")) {
        msg[0] = '\0';
        selected = -1;
        scroll = 0;
        screen = SCR_LIST;
    }
    if (Button((Rectangle){ 300, 330, 300, 50 }, "Payroll Summary")) {
        screen = SCR_SUMMARY;
    }
    if (Button((Rectangle){ 300, 400, 300, 50 }, "Exit")) {
        exitApp = true;
    }
    DrawText(TextFormat("Total employees: %d", count), 20, SCREEN_H - 30, 20, DARKGRAY);
}

void ScreenAdd(void)
{
    DrawText(editIndex >= 0 ? "Edit Employee" : "Add New Employee", 20, 20, 30, DARKBLUE);

    /* Tab key moves to the next field */
    if (IsKeyPressed(KEY_TAB)) activeField = (activeField + 1) % 6;

    HandleTyping(fieldBuf[activeField], fieldMax[activeField], fieldMode[activeField]);

    for (int i = 0; i < 6; i++) {
        Rectangle box = { 320, (float)(80 + i * 60), 400, 36 };
        DrawText(fieldLabel[i], 40, (int)box.y + 8, 20, BLACK);

        if (CheckCollisionPointRec(GetMousePosition(), box) &&
            IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            activeField = i;

        DrawRectangleRec(box, WHITE);
        DrawRectangleLinesEx(box, 2, (i == activeField) ? BLUE : GRAY);
        DrawText(fieldBuf[i], (int)box.x + 8, (int)box.y + 8, 20, BLACK);

        /* blinking cursor */
        if (i == activeField && ((int)(GetTime() * 2) % 2 == 0))
            DrawText("|", (int)box.x + 8 + MeasureText(fieldBuf[i], 20), (int)box.y + 8, 20, BLACK);
    }

    if (Button((Rectangle){ 320, 460, 120, 40 }, "Save"))  SaveEmployee();
    if (Button((Rectangle){ 460, 460, 120, 40 }, "Clear")) {
        int e = editIndex;
        ClearForm();
        editIndex = e;
    }
    if (Button((Rectangle){ 600, 460, 120, 40 }, "Back")) {
        ClearForm();
        msg[0] = '\0';
        screen = SCR_MENU;
    }
    DrawText("Tip: press TAB to go to the next field", 320, 515, 18, GRAY);
}

void ScreenList(void)
{
    HandleTyping(searchBuf, 28, 0);

    DrawText("Employee List", 20, 15, 30, DARKBLUE);
    DrawText("Search (name/ID):", 20, 68, 20, BLACK);
    DrawRectangle(200, 60, 300, 36, WHITE);
    DrawRectangleLines(200, 60, 300, 36, BLUE);
    DrawText(searchBuf, 208, 68, 20, BLACK);

    /* Find matching employees */
    int match[MAX_EMP], m = 0;
    for (int i = 0; i < count; i++) {
        char idStr[12];
        sprintf(idStr, "%d", emp[i].id);
        if (searchBuf[0] == '\0' ||
            ContainsIgnoreCase(emp[i].name, searchBuf) ||
            ContainsIgnoreCase(idStr, searchBuf))
            match[m++] = i;
    }

    /* Scrolling with mouse wheel */
    int visible = 11;
    scroll -= (int)GetMouseWheelMove();
    if (scroll > m - visible) scroll = m - visible;
    if (scroll < 0) scroll = 0;

    /* Table header */
    DrawRectangle(20, 115, 860, 30, DARKBLUE);
    DrawText("ID",          30, 120, 20, WHITE);
    DrawText("Name",       100, 120, 20, WHITE);
    DrawText("Department", 330, 120, 20, WHITE);
    DrawText("Basic",      500, 120, 20, WHITE);
    DrawText("Days",       620, 120, 20, WHITE);
    DrawText("Net Salary", 720, 120, 20, WHITE);

    for (int r = 0; r < visible && r + scroll < m; r++) {
        int i = match[r + scroll];
        Rectangle row = { 20, (float)(148 + r * 30), 860, 28 };

        if (CheckCollisionPointRec(GetMousePosition(), row) &&
            IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            selected = i;

        Color bg = (r % 2) ? RAYWHITE : LIGHTGRAY;
        if (selected == i) bg = SKYBLUE;
        DrawRectangleRec(row, bg);

        Payslip p = Calculate(emp[i]);
        int y = (int)row.y + 4;
        DrawText(TextFormat("%d", emp[i].id),           30, y, 20, BLACK);
        DrawText(emp[i].name,                          100, y, 20, BLACK);
        DrawText(emp[i].dept,                          330, y, 20, BLACK);
        DrawText(TextFormat("%.0f", emp[i].basic),     500, y, 20, BLACK);
        DrawText(TextFormat("%d", emp[i].daysPresent), 620, y, 20, BLACK);
        DrawText(TextFormat("%.2f", p.net),            720, y, 20, BLACK);
    }

    if (m == 0) DrawText("No employees found.", 340, 250, 20, GRAY);
    DrawText(TextFormat("Showing %d of %d  (mouse wheel = scroll, click a row to select)", m, count),
             20, 485, 16, GRAY);

    /* Buttons */
    if (Button((Rectangle){ 20, 510, 130, 38 }, "Payslip")) {
        if (selected >= 0) { msg[0] = '\0'; screen = SCR_PAYSLIP; }
        else SetMsg("Select an employee first.", RED);
    }
    if (Button((Rectangle){ 165, 510, 130, 38 }, "Edit")) {
        if (selected >= 0) { LoadEmployeeToForm(selected); msg[0] = '\0'; screen = SCR_ADD; }
        else SetMsg("Select an employee first.", RED);
    }
    if (Button((Rectangle){ 310, 510, 130, 38 }, "Delete")) {
        if (selected >= 0) DeleteEmployee(selected);
        else SetMsg("Select an employee first.", RED);
    }
    if (Button((Rectangle){ 455, 510, 130, 38 }, "Back")) {
        msg[0] = '\0';
        screen = SCR_MENU;
    }
}

void ScreenPayslip(void)
{
    if (selected < 0 || selected >= count) { screen = SCR_LIST; return; }

    Employee e = emp[selected];
    Payslip  p = Calculate(e);

    DrawText("PAYSLIP", (SCREEN_W - MeasureText("PAYSLIP", 36)) / 2, 15, 36, DARKBLUE);
    DrawRectangle(150, 65, 600, 440, RAYWHITE);
    DrawRectangleLines(150, 65, 600, 440, DARKGRAY);

    int x = 175, y = 80;
    DrawText(TextFormat("Employee ID : %d", e.id),               x, y,      20, BLACK);
    DrawText(TextFormat("Name        : %s", e.name),             x, y + 28, 20, BLACK);
    DrawText(TextFormat("Department  : %s", e.dept),             x, y + 56, 20, BLACK);
    DrawText(TextFormat("Days Present: %d / 30", e.daysPresent), x, y + 84, 20, BLACK);
    DrawLine(160, y + 115, 740, y + 115, GRAY);

    DrawText("EARNINGS", x, y + 125, 20, DARKGREEN);
    DrawText(TextFormat("Basic (earned) : Rs. %.2f", p.earned), x, y + 153, 20, BLACK);
    DrawText(TextFormat("HRA (20%%)      : Rs. %.2f", p.hra),    x, y + 181, 20, BLACK);
    DrawText(TextFormat("DA (10%%)       : Rs. %.2f", p.da),     x, y + 209, 20, BLACK);
    DrawText(TextFormat("Bonus          : Rs. %.2f", e.bonus),  x, y + 237, 20, BLACK);
    DrawText(TextFormat("GROSS SALARY   : Rs. %.2f", p.gross),  x, y + 265, 20, DARKBLUE);

    DrawText("DEDUCTIONS", x, y + 300, 20, MAROON);
    DrawText(TextFormat("PF (12%%) : Rs. %.2f", p.pf),  x, y + 328, 20, BLACK);
    DrawText(TextFormat("Tax      : Rs. %.2f", p.tax), x, y + 356, 20, BLACK);

    DrawText(TextFormat("NET SALARY: Rs. %.2f", p.net), 175, y + 395, 26, DARKGREEN);

    if (Button((Rectangle){ 390, 535, 120, 40 }, "Back"))
        screen = SCR_LIST;
}

void ScreenSummary(void)
{
    DrawText("Payroll Summary", 20, 20, 30, DARKBLUE);

    float totalBasic = 0, totalGross = 0, totalNet = 0;
    int top = -1;
    float topNet = 0;

    for (int i = 0; i < count; i++) {
        Payslip p = Calculate(emp[i]);
        totalBasic += emp[i].basic;
        totalGross += p.gross;
        totalNet   += p.net;
        if (top == -1 || p.net > topNet) { top = i; topNet = p.net; }
    }

    DrawRectangle(150, 90, 600, 330, RAYWHITE);
    DrawRectangleLines(150, 90, 600, 330, DARKGRAY);

    DrawText(TextFormat("Total employees    : %d", count),              180, 115, 22, BLACK);
    DrawText(TextFormat("Total basic salary : Rs. %.2f", totalBasic),   180, 160, 22, BLACK);
    DrawText(TextFormat("Total gross salary : Rs. %.2f", totalGross),   180, 205, 22, BLACK);
    DrawText(TextFormat("Total net payable  : Rs. %.2f", totalNet),     180, 250, 22, DARKGREEN);

    if (count > 0) {
        DrawText(TextFormat("Average net salary : Rs. %.2f", totalNet / count), 180, 295, 22, BLACK);
        DrawText(TextFormat("Highest paid       : %s (Rs. %.2f)", emp[top].name, topNet),
                 180, 340, 22, DARKBLUE);
    } else {
        DrawText("No employee data yet.", 180, 295, 22, GRAY);
    }

    if (Button((Rectangle){ 390, 460, 120, 40 }, "Back"))
        screen = SCR_MENU;
}

/* ---------------- Main ---------------- */

int main(void)
{
    InitWindow(SCREEN_W, SCREEN_H, "Employee Payroll Management System");
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);       /* ESC will not close the window */

    LoadData();

    while (!WindowShouldClose() && !exitApp) {
        BeginDrawing();
        ClearBackground((Color){ 235, 240, 248, 255 });

        switch (screen) {
            case SCR_MENU:    ScreenMenu();    break;
            case SCR_ADD:     ScreenAdd();     break;
            case SCR_LIST:    ScreenList();    break;
            case SCR_PAYSLIP: ScreenPayslip(); break;
            case SCR_SUMMARY: ScreenSummary(); break;
        }

        if (screen != SCR_MENU) DrawMessage();
        EndDrawing();
    }

    SaveData();
    CloseWindow();
    return 0;
}
