#include <keyboard_input.hpp>

#include <cstdint>
#include <string>

// TODO build without X11
#ifndef VKBASALT_X11
#define VKBASALT_X11 1
#endif

#if VKBASALT_X11
#include "keyboard_input_x11.hpp"
#endif

namespace vkBasalt
{
    uint32_t convertToKeySym(const std::string& key)
    {
#if VKBASALT_X11
        return convertToKeySymX11(key);
#endif
        return 0U;
    }

    bool isKeyPressed(uint32_t keyCode)
    {
#if VKBASALT_X11
        return isKeyPressedX11(keyCode);
#endif
        return false;
    }
} // namespace vkBasalt
