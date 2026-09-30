#pragma once

void logInit(const char* consoleName, const char* prefix);
void logStop();
void logPrint(const char* fmt, ...);
