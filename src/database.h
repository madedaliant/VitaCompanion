
#ifndef DATABASE_H
#define DATABASE_H

#define DB_MAX_APPS 64
#define DB_MAX_RECORDS 128

#define DB_TEXT_SIZE 128
#define DB_NOTES_SIZE 512

typedef struct {
    int id;

    char name[DB_TEXT_SIZE];
    char version[32];
    char developer[DB_TEXT_SIZE];
    char category[DB_TEXT_SIZE];

} HomebrewApp;

typedef struct {
    int id;
    int app_id;

    char firmware[32];
    char status[32];
    char notes[DB_NOTES_SIZE];

} CompatibilityRecord;

/* Database initialization */
int databaseInit(void);

/* Application CRUD */
int databaseGetAppCount(void);

const HomebrewApp *databaseGetApp(int index);

int databaseAddApp(const HomebrewApp *app);

int databaseUpdateApp(int id, const HomebrewApp *updatedApp);

int databaseDeleteApp(int id);

/* Compatibility record CRUD */
int databaseGetRecordCount(void);

const CompatibilityRecord *databaseGetRecord(int index);

int databaseAddRecord(const CompatibilityRecord *record);

int databaseUpdateRecord(
    int id,
    const CompatibilityRecord *updatedRecord
);

int databaseDeleteRecord(int id);

/* Save all database data */
int databaseSave(void);

#endif
