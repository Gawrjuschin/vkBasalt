#ifndef VKBASALT_KEYBOARD_INPUT_X11_HPP
#define VKBASALT_KEYBOARD_INPUT_X11_HPP

#include <cstdint>
#include <string>

namespace vkBasalt
{
    uint32_t convertToKeySymX11(std::string key);
    bool     isKeyPressedX11(uint32_t ks);
} // namespace vkBasalt

#endif // VKBASALT_KEYBOARD_INPUT_X11_HPP