#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace CaffeineTake {

struct IconPack
{
    HICON caffeineDisabledLight     = NULL;
    HICON caffeineDisabledDark      = NULL;

    HICON caffeineEnabledLight      = NULL;
    HICON caffeineEnabledDark       = NULL;

    HICON caffeineAutoInactiveLight = NULL;
    HICON caffeineAutoInactiveDark  = NULL;

    HICON caffeineAutoActiveLight   = NULL;
    HICON caffeineAutoActiveDark    = NULL;
};

} // namespace CaffeineTake
