
#include <stdio.h>
#include <string.h>

#include <psp2/ctrl.h>
#include <psp2/kernel/processmgr.h>
#include <vita2d.h>

#define SCREEN_WIDTH  960
#define SCREEN_HEIGHT 544

#define COLOR_BG       RGBA8(15, 24, 42, 255)
#define COLOR_PANEL    RGBA8(25, 39, 62, 255)
#define COLOR_SELECTED RGBA8(43, 78, 115, 255)
#define COLOR_ACCENT   RGBA8(76, 166, 255, 255)
#define COLOR_WHITE    RGBA8(245, 248, 255, 255)
#define COLOR_MUTED    RGBA8(151, 171, 198, 255)
#define COLOR_GREEN    RGBA8(90, 210, 140, 255)
#define COLOR_YELLOW   RGBA8(245, 190, 80, 255)
#define COLOR_BORDER   RGBA8(56, 76, 103, 255)

#define APP_COUNT 3
#define HOME_COUNT 3

typedef enum {
    PAGE_HOME,
    PAGE_APPLICATIONS,
    PAGE_APP_DETAIL,
    PAGE_RECORDS,
    PAGE_SETTINGS
} Page;

typedef struct {
    const char *name;
    const char *version;
    const char *developer;
    const char *category;
    const char *compatibility;
} HomebrewApp;

// Main menu
static const char *homeMenu[HOME_COUNT] = {
    "Applications",
    "Compatibility Records",
    "Settings"
};

// Temporary application data
static const HomebrewApp applications[APP_COUNT] = {
    {
        "VitaDB Downloader",
        "2.0",
        "Rinnegatamante",
        "Homebrew Manager",
        "Compatible"
    },
    {
        "VitaShell",
        "2.02",
        "TheFloW",
        "File Manager",
        "Compatible"
    },
    {
        "Adrenaline",
        "7.1",
        "TheFloW",
        "PSP Environment",
        "Not Tested"
    }
};

// Application state
static Page currentPage = PAGE_HOME;

static int homeSelected = 0;
static int appSelected = 0;

// Draw text helper
static void drawText(
    vita2d_pgf *font,
    float x,
    float y,
    unsigned int color,
    float scale,
    const char *text
) {
    vita2d_pgf_draw_text(
        font,
        x,
        y,
        color,
        scale,
        text
    );
}

// Header
static void drawHeader(
    vita2d_pgf *font,
    const char *subtitle
) {
    vita2d_draw_rectangle(
        0, 0,
        SCREEN_WIDTH, 105,
        COLOR_PANEL
    );

    drawText(
        font, 40, 49,
        COLOR_WHITE, 1.5f,
        "VitaCompanion"
    );

    drawText(
        font, 42, 76,
        COLOR_MUTED, 0.75f,
        subtitle
    );

    drawText(
        font, 825, 48,
        COLOR_ACCENT, 0.9f,
        "v0.4.0"
    );

    vita2d_draw_rectangle(
        0, 103,
        SCREEN_WIDTH, 2,
        COLOR_BORDER
    );
}

// Footer
static void drawFooter(vita2d_pgf *font) {
    vita2d_draw_rectangle(
        0, 505,
        SCREEN_WIDTH, 39,
        COLOR_PANEL
    );

    drawText(
        font, 40, 530,
        COLOR_MUTED, 0.75f,
        "DPAD: Navigate"
    );

    drawText(
        font, 330, 530,
        COLOR_WHITE, 0.75f,
        "O: Select"
    );

    drawText(
        font, 490, 530,
        COLOR_WHITE, 0.75f,
        "X: Back"
    );
}

// Main menu
static void drawHome(vita2d_pgf *font) {
    drawHeader(
        font,
        "HOMEBREW COMPATIBILITY MANAGER"
    );

    drawText(
        font, 45, 145,
        COLOR_MUTED, 0.85f,
        "MAIN MENU"
    );

    for (int i = 0; i < HOME_COUNT; i++) {
        int y = 170 + i * 100;

        unsigned int background =
            (i == homeSelected)
            ? COLOR_SELECTED
            : COLOR_PANEL;

        vita2d_draw_rectangle(
            40, y, 880, 82,
            background
        );

        if (i == homeSelected) {
            vita2d_draw_rectangle(
                40, y, 5, 82,
                COLOR_ACCENT
            );
        }

        drawText(
            font, 65, y + 36,
            COLOR_WHITE, 1.0f,
            homeMenu[i]
        );

        if (i == homeSelected) {
            drawText(
                font, 860, y + 47,
                COLOR_ACCENT, 1.0f,
                ">"
            );
        }
    }

    drawFooter(font);
}

// Applications page
static void drawApplications(vita2d_pgf *font) {
    drawHeader(
        font,
        "APPLICATION LIBRARY"
    );

    drawText(
        font, 45, 145,
        COLOR_MUTED, 0.85f,
        "REGISTERED HOMEBREW APPLICATIONS"
    );

    for (int i = 0; i < APP_COUNT; i++) {
        int y = 170 + i * 100;

        unsigned int background =
            (i == appSelected)
            ? COLOR_SELECTED
            : COLOR_PANEL;

        vita2d_draw_rectangle(
            40, y, 880, 82,
            background
        );

        if (i == appSelected) {
            vita2d_draw_rectangle(
                40, y, 5, 82,
                COLOR_ACCENT
            );
        }

        drawText(
            font, 65, y + 32,
            COLOR_WHITE, 1.0f,
            applications[i].name
        );

        drawText(
            font, 65, y + 57,
            COLOR_MUTED, 0.75f,
            applications[i].category
        );

        unsigned int statusColor =
            strcmp(
                applications[i].compatibility,
                "Compatible"
            ) == 0
            ? COLOR_GREEN
            : COLOR_YELLOW;

        drawText(
            font, 745, y + 43,
            statusColor, 0.8f,
            applications[i].compatibility
        );
    }

    drawFooter(font);
}

// Application detail page
static void drawAppDetail(vita2d_pgf *font) {
    const HomebrewApp *app =
        &applications[appSelected];

    drawHeader(
        font,
        "APPLICATION DETAILS"
    );

    drawText(
        font, 45, 150,
        COLOR_ACCENT, 1.2f,
        app->name
    );

    drawText(
        font, 45, 210,
        COLOR_WHITE, 0.9f,
        "Application Information"
    );

    drawText(
        font, 65, 250,
        COLOR_MUTED, 0.85f,
        "Version:"
    );

    drawText(
        font, 300, 250,
        COLOR_WHITE, 0.85f,
        app->version
    );

    drawText(
        font, 65, 290,
        COLOR_MUTED, 0.85f,
        "Developer:"
    );

    drawText(
        font, 300, 290,
        COLOR_WHITE, 0.85f,
        app->developer
    );

    drawText(
        font, 65, 330,
        COLOR_MUTED, 0.85f,
        "Category:"
    );

    drawText(
        font, 300, 330,
        COLOR_WHITE, 0.85f,
        app->category
    );

    drawText(
        font, 65, 370,
        COLOR_MUTED, 0.85f,
        "Compatibility:"
    );

    unsigned int statusColor =
        strcmp(
            app->compatibility,
            "Compatible"
        ) == 0
        ? COLOR_GREEN
        : COLOR_YELLOW;

    drawText(
        font, 300, 370,
        statusColor, 0.85f,
        app->compatibility
    );

    drawFooter(font);
}

// Compatibility records page
static void drawRecords(vita2d_pgf *font) {
    drawHeader(
        font,
        "COMPATIBILITY RECORDS"
    );

    drawText(
        font, 45, 160,
        COLOR_WHITE, 1.0f,
        "Compatibility Records"
    );

    drawText(
        font, 45, 205,
        COLOR_MUTED, 0.85f,
        "Registered applications:"
    );

    char buffer[64];

    snprintf(
        buffer,
        sizeof(buffer),
        "%d applications",
        APP_COUNT
    );

    drawText(
        font, 340, 205,
        COLOR_ACCENT, 0.85f,
        buffer
    );

    drawText(
        font, 45, 270,
        COLOR_MUTED, 0.85f,
        "Detailed test history will be implemented"
    );

    drawText(
        font, 45, 300,
        COLOR_MUTED, 0.85f,
        "in the next development stage."
    );

    drawFooter(font);
}

// Settings page
static void drawSettings(vita2d_pgf *font) {
    drawHeader(
        font,
        "APPLICATION SETTINGS"
    );

    drawText(
        font, 45, 165,
        COLOR_WHITE, 1.0f,
        "VitaCompanion Settings"
    );

    drawText(
        font, 45, 220,
        COLOR_MUTED, 0.85f,
        "Application Version: 0.4.0"
    );

    drawText(
        font, 45, 260,
        COLOR_MUTED, 0.85f,
        "Platform: PlayStation Vita"
    );

    drawText(
        font, 45, 300,
        COLOR_MUTED, 0.85f,
        "Storage: Not configured"
    );

    drawFooter(font);
}

// Render current page
static void drawScreen(vita2d_pgf *font) {
    vita2d_start_drawing();

    vita2d_clear_screen();

    switch (currentPage) {
        case PAGE_HOME:
            drawHome(font);
            break;

        case PAGE_APPLICATIONS:
            drawApplications(font);
            break;

        case PAGE_APP_DETAIL:
            drawAppDetail(font);
            break;

        case PAGE_RECORDS:
            drawRecords(font);
            break;

        case PAGE_SETTINGS:
            drawSettings(font);
            break;
    }

    vita2d_end_drawing();
    vita2d_swap_buffers();
}

// Handle controller input
static void handleInput(
    unsigned int pressed
) {
    // Main menu
    if (currentPage == PAGE_HOME) {

        if (pressed & SCE_CTRL_UP) {
            homeSelected =
                (homeSelected + HOME_COUNT - 1)
                % HOME_COUNT;
        }

        if (pressed & SCE_CTRL_DOWN) {
            homeSelected =
                (homeSelected + 1)
                % HOME_COUNT;
        }

        // O = Confirm
        if (pressed & SCE_CTRL_CIRCLE) {
            switch (homeSelected) {
                case 0:
                    currentPage = PAGE_APPLICATIONS;
                    break;

                case 1:
                    currentPage = PAGE_RECORDS;
                    break;

                case 2:
                    currentPage = PAGE_SETTINGS;
                    break;
            }
        }
    }

    // Applications page
    else if (currentPage == PAGE_APPLICATIONS) {

        if (pressed & SCE_CTRL_UP) {
            appSelected =
                (appSelected + APP_COUNT - 1)
                % APP_COUNT;
        }

        if (pressed & SCE_CTRL_DOWN) {
            appSelected =
                (appSelected + 1)
                % APP_COUNT;
        }

        // O = Open application detail
        if (pressed & SCE_CTRL_CIRCLE) {
            currentPage = PAGE_APP_DETAIL;
        }

        // X = Back
        if (pressed & SCE_CTRL_CROSS) {
            currentPage = PAGE_HOME;
        }
    }

    // Other pages
    else {

        // X = Back
        if (pressed & SCE_CTRL_CROSS) {
            if (currentPage == PAGE_APP_DETAIL) {
                currentPage = PAGE_APPLICATIONS;
            } else {
                currentPage = PAGE_HOME;
            }
        }
    }
}

// Main
int main(void) {
    SceCtrlData pad;

    unsigned int previousButtons = 0;

    sceCtrlSetSamplingMode(
        SCE_CTRL_MODE_DIGITAL
    );

    if (vita2d_init() < 0) {
        sceKernelExitProcess(1);
        return 1;
    }

    vita2d_set_clear_color(
        COLOR_BG
    );

    vita2d_pgf *font =
        vita2d_load_default_pgf();

    if (font == NULL) {
        vita2d_fini();

        sceKernelExitProcess(1);
        return 1;
    }

    memset(
        &pad,
        0,
        sizeof(pad)
    );

    // Main application loop
    while (1) {

        sceCtrlPeekBufferPositive(
            0,
            &pad,
            1
        );

        unsigned int pressed =
            pad.buttons & ~previousButtons;

        handleInput(pressed);

        previousButtons = pad.buttons;

        drawScreen(font);

        sceKernelDelayThread(16667);
    }

    // Cleanup is intentionally not tied to START.
    // The PS Vita system handles the PS button.
    vita2d_wait_rendering_done();

    vita2d_free_pgf(font);
    vita2d_fini();

    sceKernelExitProcess(0);

    return 0;
}
