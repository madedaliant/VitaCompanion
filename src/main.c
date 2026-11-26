#include <stdio.h>
#include <string.h>

#include <psp2/ctrl.h>
#include <psp2/kernel/processmgr.h>
#include <vita2d.h>

#include "database.h"

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
#define COLOR_RED      RGBA8(235, 100, 100, 255)
#define COLOR_BORDER   RGBA8(56, 76, 103, 255)

#define HOME_COUNT 3
#define LIST_VISIBLE 3

typedef enum {
    PAGE_HOME,
    PAGE_APPLICATIONS,
    PAGE_APP_DETAIL,
    PAGE_RECORDS,
    PAGE_SETTINGS,
    PAGE_DATABASE_ERROR
} Page;

static const char *homeMenu[HOME_COUNT] = {
    "Applications",
    "Compatibility Records",
    "Settings"
};

static Page currentPage = PAGE_HOME;
static int homeSelected = 0;
static int appSelected = 0;
static int recordSelected = 0;

/* Database initialization status */
static int databaseReady = 0;

/* ------------------------------------------------------------------------- */
/* Helpers                                                                   */
/* ------------------------------------------------------------------------- */

static void drawText(
    vita2d_pgf *font,
    float x,
    float y,
    unsigned int color,
    float scale,
    const char *text
) {
    if (text == NULL) {
        text = "";
    }

    vita2d_pgf_draw_text(font, x, y, color, scale, text);
}

static int getAppCount(void) {
    int count = databaseGetAppCount();

    if (count < 0) {
        return 0;
    }

    return count;
}

static int getRecordCount(void) {
    int count = databaseGetRecordCount();

    if (count < 0) {
        return 0;
    }

    return count;
}

/* Keep the selected item within the current database bounds. */
static void clampSelections(void) {
    int appCount = getAppCount();
    int recordCount = getRecordCount();

    if (appCount <= 0) {
        appSelected = 0;
    } else if (appSelected >= appCount) {
        appSelected = appCount - 1;
    }

    if (recordCount <= 0) {
        recordSelected = 0;
    } else if (recordSelected >= recordCount) {
        recordSelected = recordCount - 1;
    }
}

static const char *getAppCompatibility(int appId) {
    int count = getRecordCount();

    /* Use the first matching record available from the database. */
    for (int i = 0; i < count; i++) {
        const CompatibilityRecord *record =
            databaseGetRecord(i);

        if (record != NULL && record->app_id == appId) {
            if (record->status[0] != '\0') {
                return record->status;
            }
        }
    }

    return "No Record";
}

static unsigned int getStatusColor(const char *status) {
    if (status == NULL) {
        return COLOR_MUTED;
    }

    if (strcmp(status, "Compatible") == 0 ||
        strcmp(status, "Working") == 0 ||
        strcmp(status, "Playable") == 0) {
        return COLOR_GREEN;
    }

    if (strcmp(status, "Not Tested") == 0 ||
        strcmp(status, "No Record") == 0 ||
        strcmp(status, "Unknown") == 0) {
        return COLOR_YELLOW;
    }

    return COLOR_RED;
}

/* ------------------------------------------------------------------------- */
/* Header and footer                                                         */
/* ------------------------------------------------------------------------- */

static void drawHeader(vita2d_pgf *font, const char *subtitle) {
    vita2d_draw_rectangle(
        0, 0, SCREEN_WIDTH, 105,
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
        0, 103, SCREEN_WIDTH, 2,
        COLOR_BORDER
    );
}

static void drawFooter(vita2d_pgf *font) {
    vita2d_draw_rectangle(
        0, 505, SCREEN_WIDTH, 39,
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

/* ------------------------------------------------------------------------- */
/* Pages                                                                     */
/* ------------------------------------------------------------------------- */

static void drawHome(vita2d_pgf *font) {
    drawHeader(font, "HOMEBREW COMPATIBILITY MANAGER");

    drawText(
        font, 45, 145,
        COLOR_MUTED, 0.85f,
        "MAIN MENU"
    );

    for (int i = 0; i < HOME_COUNT; i++) {
        int y = 170 + i * 100;
        unsigned int bg =
            (i == homeSelected) ? COLOR_SELECTED : COLOR_PANEL;

        vita2d_draw_rectangle(40, y, 880, 82, bg);

        if (i == homeSelected) {
            vita2d_draw_rectangle(40, y, 5, 82, COLOR_ACCENT);
        }

        drawText(
            font, 65, y + 36,
            COLOR_WHITE, 1.0f,
            homeMenu[i]
        );

        if (i == homeSelected) {
            drawText(font, 860, y + 47, COLOR_ACCENT, 1.0f, ">");
        }
    }

    drawFooter(font);
}

static void drawApplications(vita2d_pgf *font) {
    drawHeader(font, "APPLICATION LIBRARY");

    int count = getAppCount();

    drawText(
        font, 45, 145,
        COLOR_MUTED, 0.85f,
        "REGISTERED HOMEBREW APPLICATIONS"
    );

    char countText[64];
    snprintf(countText, sizeof(countText), "%d apps", count);
    drawText(font, 820, 145, COLOR_ACCENT, 0.8f, countText);

    if (count == 0) {
        drawText(
            font, 45, 220,
            COLOR_YELLOW, 0.9f,
            "No applications in the database yet."
        );
        drawText(
            font, 45, 255,
            COLOR_MUTED, 0.8f,
            "Add an application through the database functions."
        );
        drawFooter(font);
        return;
    }

    int first = 0;
    if (appSelected >= LIST_VISIBLE) {
        first = appSelected - LIST_VISIBLE + 1;
    }

    for (int visible = 0; visible < LIST_VISIBLE; visible++) {
        int index = first + visible;
        if (index >= count) {
            break;
        }

        const HomebrewApp *app = databaseGetApp(index);
        if (app == NULL) {
            continue;
        }

        int y = 170 + visible * 100;
        unsigned int bg =
            (index == appSelected) ? COLOR_SELECTED : COLOR_PANEL;

        vita2d_draw_rectangle(40, y, 880, 82, bg);

        if (index == appSelected) {
            vita2d_draw_rectangle(40, y, 5, 82, COLOR_ACCENT);
        }

        drawText(font, 65, y + 32, COLOR_WHITE, 1.0f, app->name);
        drawText(font, 65, y + 57, COLOR_MUTED, 0.75f, app->category);

        const char *status = getAppCompatibility(app->id);
        drawText(
            font, 745, y + 43,
            getStatusColor(status), 0.8f,
            status
        );
    }

    drawFooter(font);
}

static void drawAppDetail(vita2d_pgf *font) {
    int count = getAppCount();

    drawHeader(font, "APPLICATION DETAILS");

    if (count <= 0) {
        drawText(font, 45, 180, COLOR_YELLOW, 0.9f, "No application selected.");
        drawFooter(font);
        return;
    }

    const HomebrewApp *app = databaseGetApp(appSelected);
    if (app == NULL) {
        drawText(font, 45, 180, COLOR_RED, 0.9f, "Unable to read application.");
        drawFooter(font);
        return;
    }

    const char *compatibility = getAppCompatibility(app->id);

    drawText(font, 45, 150, COLOR_ACCENT, 1.2f, app->name);
    drawText(font, 45, 210, COLOR_WHITE, 0.9f, "Application Information");

    drawText(font, 65, 250, COLOR_MUTED, 0.85f, "Version:");
    drawText(font, 300, 250, COLOR_WHITE, 0.85f, app->version);

    drawText(font, 65, 290, COLOR_MUTED, 0.85f, "Developer:");
    drawText(font, 300, 290, COLOR_WHITE, 0.85f, app->developer);

    drawText(font, 65, 330, COLOR_MUTED, 0.85f, "Category:");
    drawText(font, 300, 330, COLOR_WHITE, 0.85f, app->category);

    drawText(font, 65, 370, COLOR_MUTED, 0.85f, "Compatibility:");
    drawText(
        font, 300, 370,
        getStatusColor(compatibility), 0.85f,
        compatibility
    );

    drawFooter(font);
}

static void drawRecords(vita2d_pgf *font) {
    drawHeader(font, "COMPATIBILITY RECORDS");

    int count = getRecordCount();

    drawText(font, 45, 145, COLOR_MUTED, 0.85f, "COMPATIBILITY TEST HISTORY");

    char countText[64];
    snprintf(countText, sizeof(countText), "%d records", count);
    drawText(font, 820, 145, COLOR_ACCENT, 0.8f, countText);

    if (count == 0) {
        drawText(
            font, 45, 220,
            COLOR_YELLOW, 0.9f,
            "No compatibility records in the database yet."
        );
        drawFooter(font);
        return;
    }

    int first = 0;
    if (recordSelected >= LIST_VISIBLE) {
        first = recordSelected - LIST_VISIBLE + 1;
    }

    for (int visible = 0; visible < LIST_VISIBLE; visible++) {
        int index = first + visible;
        if (index >= count) {
            break;
        }

        const CompatibilityRecord *record =
            databaseGetRecord(index);

        if (record == NULL) {
            continue;
        }

        int y = 170 + visible * 100;
        unsigned int bg =
            (index == recordSelected) ? COLOR_SELECTED : COLOR_PANEL;

        vita2d_draw_rectangle(40, y, 880, 82, bg);

        if (index == recordSelected) {
            vita2d_draw_rectangle(40, y, 5, 82, COLOR_ACCENT);
        }

        char appName[DB_TEXT_SIZE];
        snprintf(appName, sizeof(appName), "Application ID: %d", record->app_id);

        drawText(font, 65, y + 30, COLOR_WHITE, 0.9f, appName);
        drawText(font, 65, y + 57, COLOR_MUTED, 0.75f, record->firmware);

        drawText(
            font, 745, y + 43,
            getStatusColor(record->status), 0.8f,
            record->status
        );
    }

    drawFooter(font);
}

static void drawSettings(vita2d_pgf *font) {
    drawHeader(font, "APPLICATION SETTINGS");

    drawText(font, 45, 165, COLOR_WHITE, 1.0f, "VitaCompanion Settings");
    drawText(font, 45, 220, COLOR_MUTED, 0.85f, "Application Version: 0.4.0");
    drawText(font, 45, 260, COLOR_MUTED, 0.85f, "Platform: PlayStation Vita");
    drawText(font, 45, 300, COLOR_MUTED, 0.85f, "Database: JSON (local storage)");

    drawFooter(font);
}

static void drawDatabaseError(vita2d_pgf *font) {
    drawHeader(font, "DATABASE ERROR");

    drawText(
        font, 45, 180,
        COLOR_RED, 0.95f,
        "Database initialization failed."
    );

    drawText(
        font, 45, 225,
        COLOR_MUTED, 0.8f,
        "Check database.c and the Vita storage path."
    );

    drawText(
        font, 45, 270,
        COLOR_MUTED, 0.8f,
        "Press X to return to the home screen."
    );

    drawFooter(font);
}

/* ------------------------------------------------------------------------- */
/* Rendering                                                                 */
/* ------------------------------------------------------------------------- */

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
        case PAGE_DATABASE_ERROR:
            drawDatabaseError(font);
            break;
    }

    vita2d_end_drawing();
    vita2d_swap_buffers();
}

/* ------------------------------------------------------------------------- */
/* Controller input                                                          */
/* ------------------------------------------------------------------------- */

static void handleInput(unsigned int pressed) {
    clampSelections();

    if (currentPage == PAGE_HOME) {
        if (pressed & SCE_CTRL_UP) {
            homeSelected = (homeSelected + HOME_COUNT - 1) % HOME_COUNT;
        }

        if (pressed & SCE_CTRL_DOWN) {
            homeSelected = (homeSelected + 1) % HOME_COUNT;
        }

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
    } else if (currentPage == PAGE_APPLICATIONS) {
        int count = getAppCount();

        if (count > 0 && (pressed & SCE_CTRL_UP)) {
            appSelected = (appSelected + count - 1) % count;
        }

        if (count > 0 && (pressed & SCE_CTRL_DOWN)) {
            appSelected = (appSelected + 1) % count;
        }

        if (pressed & SCE_CTRL_CIRCLE) {
            if (count > 0) {
                currentPage = PAGE_APP_DETAIL;
            }
        }

        if (pressed & SCE_CTRL_CROSS) {
            currentPage = PAGE_HOME;
        }
    } else if (currentPage == PAGE_RECORDS) {
        int count = getRecordCount();

        if (count > 0 && (pressed & SCE_CTRL_UP)) {
            recordSelected = (recordSelected + count - 1) % count;
        }

        if (count > 0 && (pressed & SCE_CTRL_DOWN)) {
            recordSelected = (recordSelected + 1) % count;
        }

        if (pressed & SCE_CTRL_CROSS) {
            currentPage = PAGE_HOME;
        }
    } else {
        if (pressed & SCE_CTRL_CROSS) {
            if (currentPage == PAGE_APP_DETAIL) {
                currentPage = PAGE_APPLICATIONS;
            } else {
                currentPage = PAGE_HOME;
            }
        }
    }
}

/* ------------------------------------------------------------------------- */
/* Main                                                                      */
/* ------------------------------------------------------------------------- */

int main(void) {
    SceCtrlData pad;
    unsigned int previousButtons = 0;

    sceCtrlSetSamplingMode(SCE_CTRL_MODE_DIGITAL);

    if (vita2d_init() < 0) {
        sceKernelExitProcess(1);
        return 1;
    }

    vita2d_set_clear_color(COLOR_BG);

    vita2d_pgf *font = vita2d_load_default_pgf();
    if (font == NULL) {
        vita2d_fini();
        sceKernelExitProcess(1);
        return 1;
    }

    memset(&pad, 0, sizeof(pad));

    /* databaseInit() is expected to return 0 when initialization succeeds. */
    if (databaseInit() == 0) {
        databaseReady = 1;
        currentPage = PAGE_HOME;
    } else {
        databaseReady = 0;
        currentPage = PAGE_DATABASE_ERROR;
    }

    while (1) {
        sceCtrlPeekBufferPositive(0, &pad, 1);

        unsigned int pressed = pad.buttons & ~previousButtons;

        if (databaseReady) {
            handleInput(pressed);
        } else if (pressed & SCE_CTRL_CROSS) {
            currentPage = PAGE_HOME;
        }

        previousButtons = pad.buttons;

        drawScreen(font);

        sceKernelDelayThread(16667);
    }

    /* The PS Vita system handles the PS button; START is not an exit shortcut. */
    vita2d_wait_rendering_done();
    vita2d_free_pgf(font);
    vita2d_fini();

    sceKernelExitProcess(0);
    return 0;
}
