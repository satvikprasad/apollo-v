#pragma once

void
FSCreateDirectory(const char *path);

void
FSGetHomeDirectory(char *path);

const char *
FSFormatAssetsDirectory(const char *path);

const char *
FSFormatLuaDirectory(const char *path);

const char *
FSFormatDataDirectory(const char *path);

void
FSGetConfigDirectory(char *path);

void
FSGetApolloDirectory(char *path);

B8
FSCanRunCMD(const char *cmd);
