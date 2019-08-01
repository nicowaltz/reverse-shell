#pragma once

int startcmp(char* str, const char* cmp);
const char* replace(const char* search, const char* replace, const char* subject);
int split(const char* str, const char* delim, char ***r, bool command);
