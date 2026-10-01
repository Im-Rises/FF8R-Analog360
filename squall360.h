// FF8R-Analog360 - Restores 360° analog movement in FINAL FANTASY VIII Remastered
// Copyright (C) 2026 Quentin MOREL
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

static constexpr const char* APP_NAME_EFIGS = "FFVIII_EFIGS.dll";
static constexpr const char* APP_NAME_JP = "FFVIII_JP.dll";

bool isSupportedGameModuleLoaded();
bool tryInstallSquall360Patch();
