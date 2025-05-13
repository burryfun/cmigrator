#include <dirent.h>
#include <errno.h>
#include <locale.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "cutypes.h"
#include "libpq-fe.h"

char*  readFile(const char* filepath);
char** getScriptNames(const char* dirname, int* count);
void   executeScripts(PGconn* conn, const char* dirname);
bool   contains_non_ascii(const char* str);
char*  get_current_time();

static void exit_nicely(PGconn* conn)
{
    PQfinish(conn);
    exit(1);
}

int main(int argc, char** argv)
{
    const char* conninfo;
    PGconn*     conn;

    if (argc > 1)
        conninfo = argv[1];
    else
        conninfo = "postgresql://postgres:admin@localhost:5432/imdb_cmigrator";

    conn = PQconnectdb(conninfo);

    if (PQstatus(conn) != CONNECTION_OK)
    {
        fprintf(stderr, "Connection to database failed: %s",
                PQerrorMessage(conn));
        exit_nicely(conn);
    }

    executeScripts(conn, "scripts");

    PQfinish(conn);

    return 0;
}

char** getScriptNames(const char* dirname, int* count)
{
    DIR*           folder;
    struct dirent* entry;
    char**         fileNames = NULL;

    *count = 0;

    folder = opendir(dirname);

    if (folder == NULL)
    {
        fprintf(stderr, "[ERROR] Unnable to read directory: %s\n \t%s\n",
                dirname, strerror(errno));
        return NULL;
    }

    while ((entry = readdir(folder)))
    {
        if (!strcmp(entry->d_name, ".")) continue;
        if (!strcmp(entry->d_name, "..")) continue;
        if (!strstr(entry->d_name, ".sql")) continue;
        if (contains_non_ascii(entry->d_name))
        {
            fprintf(stderr, "[ERROR] Invalid filename: %s\n", entry->d_name);
            break;
        }
        fileNames = realloc(fileNames, sizeof(char*) * (*(count) + 1));
        fileNames[(*count)++] = strdup(entry->d_name);
    }

    closedir(folder);

    return fileNames;
}

char* readFile(const char* filepath)
{
    char* buffer;
    FILE* file = fopen(filepath, "rb");

    if (file == NULL)
    {
        fprintf(stderr, "[ERROR] Cannot opening file '%s': %s", filepath,
                strerror(errno));
        return NULL;
    }

    fseek(file, 0L, SEEK_END);
    long filesize = ftell(file);
    rewind(file);

    buffer = malloc(filesize + 1);

    if (buffer == NULL)
    {
        fprintf(stderr, "[ERROR] Memory allocation failed");
        return NULL;
    }

    fread(buffer, 1, filesize, file);
    buffer[filesize] = '\0';
    fclose(file);

    // ignore BOM
    if ((unsigned char)buffer[0] == 0xEF && (unsigned char)buffer[1] == 0xBB &&
        (unsigned char)buffer[2] == 0xBF)
    {
        memmove(buffer, buffer + 3, filesize - 2);
    }

    return buffer;
}

void executeScripts(PGconn* conn, const char* dirname)
{
    cu_char*  query;
    PGresult* res;
    int       fileCount = 0;
    char**    fileNames = getScriptNames(dirname, &fileCount);
    char      filepath[1024];

    res = PQexec(conn, "BEGIN");
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        fprintf(stderr, "BEGIN command failed: %s", PQerrorMessage(conn));
        PQclear(res);
        exit_nicely(conn);
    }
    PQclear(res);

    for (int i = 0; i < fileCount; i++)
    {
        char existsquery[1024];
        sprintf(existsquery,
                "SELECT 1 FROM \"_ScriptsMigration\" WHERE scriptname = '%s'",
                fileNames[i]);

        res = PQexec(conn, existsquery);
        if (PQntuples(res) == 1)
        {
            printf("File %3d: %s skipped\n", i + 1, fileNames[i]);
            PQclear(res);
            continue;
        }
        PQclear(res);

        snprintf(filepath, sizeof(filepath), "%s/%s", dirname, fileNames[i]);

        query = readFile(filepath);

        if (query == NULL)
        {
            continue;
        }

        res = PQexec(conn, query);
        ExecStatusType status = PQresultStatus(res);
        if (status != PGRES_COMMAND_OK && status != PGRES_TUPLES_OK)
        {
            fprintf(stderr, "[ERROR] Query command failed for file: %s \n\t%s",
                    fileNames[i], PQerrorMessage(conn));
            PQclear(res);
            res = PQexec(conn, "ROLLBACK");
            exit_nicely(conn);
            break;
        }
        PQclear(res);

        char insertquery[1024];
        snprintf(insertquery, sizeof(insertquery),
                 "INSERT INTO \"_ScriptsMigration\" (scriptname, applied) "
                 "VALUES ('%s', '%s')",
                 fileNames[i], get_current_time());
        res = PQexec(conn, insertquery);
        if (PQresultStatus(res) != PGRES_COMMAND_OK)
        {
            fprintf(stderr,
                    "[ERROR] Could not insert migration record for %s: %s\n",
                    fileNames[i], PQerrorMessage(conn));
            PQclear(res);
            PQexec(conn, "ROLLBACK");
            exit_nicely(conn);
        }
        PQclear(res);

        printf("File %3d: %s done\n", i + 1, fileNames[i]);

        free(fileNames[i]);
    }

    free(fileNames);
    res = PQexec(conn, "COMMIT");
    PQclear(res);
}

bool contains_non_ascii(const char* str)
{
    while (*str)
    {
        if ((unsigned char)*str > 127) return true;
        str++;
    }
    return false;
}

char* get_current_time()
{
    time_t      rawtime;
    struct tm*  ptm;
    static char buffer[32];

    time(&rawtime);
    ptm = localtime(&rawtime);

    sprintf(buffer, "%d-%02d-%02d %02d:%02d:%02d.0", 1900 + ptm->tm_year,
            ptm->tm_mon + 1, ptm->tm_mday, ptm->tm_hour % 24, ptm->tm_min,
            ptm->tm_sec);

    return buffer;
}