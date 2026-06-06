#ifndef VKBASALT_KEYBOARD_INPUT_HPP
#define VKBASALT_KEYBOARD_INPUT_HPP

#include <cstdint>
#include <string>

namespace vkBasalt
{
    uint32_t convertToKeySym(std::string key);
    bool     isKeyPressed(uint32_t keyCode);
} // namespace vkBasalt

#endif // VKBASALT_KEYBOARD_INPUT_HPP