
#include "database.h"

#include <stdio.h>
#include <string.h>
#include <jansson.h>

#include <psp2/io/dirent.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>

/* ---------------------------------------------------------
   Database configuration
   --------------------------------------------------------- */

#define DB_DIRECTORY "ux0:data/VitaCompanion"
#define DB_FILE     "ux0:data/VitaCompanion/database.json"

static HomebrewApp apps[DB_MAX_APPS];
static CompatibilityRecord records[DB_MAX_RECORDS];

static int appCount = 0;
static int recordCount = 0;

/* ---------------------------------------------------------
   Utility functions
   --------------------------------------------------------- */

static void copyString(char *dest, size_t size, const char *src)
{
    if (dest == NULL || size == 0) {
        return;
    }

    if (src == NULL) {
        dest[0] = '\0';
        return;
    }

    snprintf(dest, size, "%s", src);
}

static void readJsonString(
    json_t *object,
    const char *key,
    char *dest,
    size_t size
)
{
    json_t *value = json_object_get(object, key);

    if (json_is_string(value)) {
        copyString(dest, size, json_string_value(value));
    } else {
        dest[0] = '\0';
    }
}

static int readJsonInt(json_t *object, const char *key)
{
    json_t *value = json_object_get(object, key);

    if (json_is_integer(value)) {
        return (int)json_integer_value(value);
    }

    return 0;
}

static int appIdExists(int id)
{
    int i;

    for (i = 0; i < appCount; i++) {
        if (apps[i].id == id) {
            return 1;
        }
    }

    return 0;
}

static int recordIdExists(int id)
{
    int i;

    for (i = 0; i < recordCount; i++) {
        if (records[i].id == id) {
            return 1;
        }
    }

    return 0;
}

static int generateAppId(void)
{
    int id = 1;

    while (appIdExists(id)) {
        id++;
    }

    return id;
}

static int generateRecordId(void)
{
    int id = 1;

    while (recordIdExists(id)) {
        id++;
    }

    return id;
}

/* ---------------------------------------------------------
   JSON serialization
   --------------------------------------------------------- */

static json_t *appToJson(const HomebrewApp *app)
{
    json_t *object = json_object();

    if (object == NULL) {
        return NULL;
    }

    json_object_set_new(
        object,
        "id",
        json_integer(app->id)
    );

    json_object_set_new(
        object,
        "name",
        json_string(app->name)
    );

    json_object_set_new(
        object,
        "version",
        json_string(app->version)
    );

    json_object_set_new(
        object,
        "developer",
        json_string(app->developer)
    );

    json_object_set_new(
        object,
        "category",
        json_string(app->category)
    );

    return object;
}

static json_t *recordToJson(const CompatibilityRecord *record)
{
    json_t *object = json_object();

    if (object == NULL) {
        return NULL;
    }

    json_object_set_new(
        object,
        "id",
        json_integer(record->id)
    );

    json_object_set_new(
        object,
        "app_id",
        json_integer(record->app_id)
    );

    json_object_set_new(
        object,
        "firmware",
        json_string(record->firmware)
    );

    json_object_set_new(
        object,
        "status",
        json_string(record->status)
    );

    json_object_set_new(
        object,
        "notes",
        json_string(record->notes)
    );

    return object;
}

/* ---------------------------------------------------------
   JSON deserialization
   --------------------------------------------------------- */

static void jsonToApp(json_t *object, HomebrewApp *app)
{
    memset(app, 0, sizeof(HomebrewApp));

    app->id = readJsonInt(object, "id");

    readJsonString(
        object,
        "name",
        app->name,
        sizeof(app->name)
    );

    readJsonString(
        object,
        "version",
        app->version,
        sizeof(app->version)
    );

    readJsonString(
        object,
        "developer",
        app->developer,
        sizeof(app->developer)
    );

    readJsonString(
        object,
        "category",
        app->category,
        sizeof(app->category)
    );
}

static void jsonToRecord(
    json_t *object,
    CompatibilityRecord *record
)
{
    memset(record, 0, sizeof(CompatibilityRecord));

    record->id = readJsonInt(object, "id");
    record->app_id = readJsonInt(object, "app_id");

    readJsonString(
        object,
        "firmware",
        record->firmware,
        sizeof(record->firmware)
    );

    readJsonString(
        object,
        "status",
        record->status,
        sizeof(record->status)
    );

    readJsonString(
        object,
        "notes",
        record->notes,
        sizeof(record->notes)
    );
}

/* ---------------------------------------------------------
   Database initialization
   --------------------------------------------------------- */

int databaseInit(void)
{
    FILE *file;
    json_t *root;
    json_t *appArray;
    json_t *recordArray;
    json_error_t error;

    size_t i;

    appCount = 0;
    recordCount = 0;

    memset(apps, 0, sizeof(apps));
    memset(records, 0, sizeof(records));

    /*
     * Create the application data directory.
     * If it already exists, this call may fail harmlessly.
     */
    sceIoMkdir(DB_DIRECTORY, 0777);

    file = fopen(DB_FILE, "rb");

    /*
     * First launch: database file does not exist yet.
     */
    if (file == NULL) {
        return databaseSave();
    }

    fclose(file);

    root = json_load_file(DB_FILE, 0, &error);

    if (root == NULL) {
        return -1;
    }

    appArray = json_object_get(root, "apps");
    recordArray = json_object_get(root, "records");

    if (json_is_array(appArray)) {
        for (i = 0;
             i < json_array_size(appArray) &&
             appCount < DB_MAX_APPS;
             i++) {

            json_t *item = json_array_get(appArray, i);

            if (!json_is_object(item)) {
                continue;
            }

            jsonToApp(item, &apps[appCount]);

            if (apps[appCount].id > 0) {
                appCount++;
            }
        }
    }

    if (json_is_array(recordArray)) {
        for (i = 0;
             i < json_array_size(recordArray) &&
             recordCount < DB_MAX_RECORDS;
             i++) {

            json_t *item = json_array_get(recordArray, i);

            if (!json_is_object(item)) {
                continue;
            }

            jsonToRecord(item, &records[recordCount]);

            if (records[recordCount].id > 0) {
                recordCount++;
            }
        }
    }

    json_decref(root);

    return 0;
}

/* ---------------------------------------------------------
   Application CRUD
   --------------------------------------------------------- */

int databaseGetAppCount(void)
{
    return appCount;
}

const HomebrewApp *databaseGetApp(int index)
{
    if (index < 0 || index >= appCount) {
        return NULL;
    }

    return &apps[index];
}

int databaseAddApp(const HomebrewApp *app)
{
    HomebrewApp newApp;

    if (app == NULL || appCount >= DB_MAX_APPS) {
        return -1;
    }

    newApp = *app;

    if (newApp.id <= 0) {
        newApp.id = generateAppId();
    } else if (appIdExists(newApp.id)) {
        return -1;
    }

    apps[appCount] = newApp;
    appCount++;

    return newApp.id;
}

int databaseUpdateApp(
    int id,
    const HomebrewApp *updatedApp
)
{
    int i;

    if (updatedApp == NULL) {
        return -1;
    }

    for (i = 0; i < appCount; i++) {
        if (apps[i].id == id) {
            apps[i] = *updatedApp;
            apps[i].id = id;

            return 0;
        }
    }

    return -1;
}

int databaseDeleteApp(int id)
{
    int i;
    int j;

    for (i = 0; i < appCount; i++) {
        if (apps[i].id == id) {

            /*
             * Remove application.
             */
            for (j = i; j < appCount - 1; j++) {
                apps[j] = apps[j + 1];
            }

            appCount--;

            /*
             * Remove compatibility records belonging
             * to this application.
             */
            i = 0;

            while (i < recordCount) {
                if (records[i].app_id == id) {

                    for (j = i; j < recordCount - 1; j++) {
                        records[j] = records[j + 1];
                    }

                    recordCount--;
                } else {
                    i++;
                }
            }

            return 0;
        }
    }

    return -1;
}

/* ---------------------------------------------------------
   Compatibility record CRUD
   --------------------------------------------------------- */

int databaseGetRecordCount(void)
{
    return recordCount;
}

const CompatibilityRecord *databaseGetRecord(int index)
{
    if (index < 0 || index >= recordCount) {
        return NULL;
    }

    return &records[index];
}

int databaseAddRecord(const CompatibilityRecord *record)
{
    CompatibilityRecord newRecord;

    if (record == NULL || recordCount >= DB_MAX_RECORDS) {
        return -1;
    }

    if (!appIdExists(record->app_id)) {
        return -1;
    }

    newRecord = *record;

    if (newRecord.id <= 0) {
        newRecord.id = generateRecordId();
    } else if (recordIdExists(newRecord.id)) {
        return -1;
    }

    records[recordCount] = newRecord;
    recordCount++;

    return newRecord.id;
}

int databaseUpdateRecord(
    int id,
    const CompatibilityRecord *updatedRecord
)
{
    int i;

    if (updatedRecord == NULL) {
        return -1;
    }

    if (!appIdExists(updatedRecord->app_id)) {
        return -1;
    }

    for (i = 0; i < recordCount; i++) {
        if (records[i].id == id) {
            records[i] = *updatedRecord;
            records[i].id = id;

            return 0;
        }
    }

    return -1;
}

int databaseDeleteRecord(int id)
{
    int i;
    int j;

    for (i = 0; i < recordCount; i++) {
        if (records[i].id == id) {

            for (j = i; j < recordCount - 1; j++) {
                records[j] = records[j + 1];
            }

            recordCount--;

            return 0;
        }
    }

    return -1;
}

/* ---------------------------------------------------------
   Save database
   --------------------------------------------------------- */

int databaseSave(void)
{
    json_t *root;
    json_t *appArray;
    json_t *recordArray;

    int i;

    root = json_object();
    appArray = json_array();
    recordArray = json_array();

    if (root == NULL ||
        appArray == NULL ||
        recordArray == NULL) {

        if (root != NULL) {
            json_decref(root);
        }

        if (appArray != NULL) {
            json_decref(appArray);
        }

        if (recordArray != NULL) {
            json_decref(recordArray);
        }

        return -1;
    }

    for (i = 0; i < appCount; i++) {
        json_t *item = appToJson(&apps[i]);

        if (item == NULL) {
            json_decref(root);
            json_decref(appArray);
            json_decref(recordArray);

            return -1;
        }

        json_array_append_new(appArray, item);
    }

    for (i = 0; i < recordCount; i++) {
        json_t *item = recordToJson(&records[i]);

        if (item == NULL) {
            json_decref(root);
            json_decref(appArray);
            json_decref(recordArray);

            return -1;
        }

        json_array_append_new(recordArray, item);
    }

    json_object_set_new(root, "apps", appArray);
    json_object_set_new(root, "records", recordArray);

    /*
     * Write JSON to the database file.
     */
    if (json_dump_file(root, DB_FILE, JSON_INDENT(2)) != 0) {
        json_decref(root);
        return -1;
    }

    json_decref(root);

    return 0;
}
