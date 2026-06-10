#include "reshade_uniforms.hpp"
#include "effect_module.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <cstdlib>
#include <cmath>
#include <iterator>
#include <string>
#include <utility>
#include <vector>
#include <memory>
#include <ratio>

#include <vulkan/vulkan_core.h>

#include <logger.hpp>

namespace vkBasalt
{
    namespace
    {
        template<typename T>
        constexpr auto AsBytes(T&& value) noexcept
        {
            return std::bit_cast<std::array<uint8_t, sizeof(T)>>(std::forward<T>(value));
        }

    } // namespace

    void enumerateReshadeUniforms(reshadefx::module module)
    {
        for (auto& uniform : module.uniforms)
        {
            auto source = std::ranges::find(uniform.annotations, "source", &reshadefx::annotation::name)->value.string_data;
            Logger::debug(source);
            Logger::debug("size: " + std::to_string(uniform.size));
            Logger::debug("offset: " + std::to_string(uniform.offset));
        }
    }

    std::vector<std::unique_ptr<ReshadeUniform>> createReshadeUniforms(reshadefx::module module)
    {
        std::vector<std::unique_ptr<ReshadeUniform>> uniforms;
        for (auto& uniform : module.uniforms)
        {
            const auto source = std::ranges::find(uniform.annotations, "source", &reshadefx::annotation::name)->value.string_data;
            if (source == "frametime")
            {
                uniforms.emplace_back(std::make_unique<FrameTimeUniform>(uniform));
            }
            else if (source == "framecount")
            {
                uniforms.emplace_back(std::make_unique<FrameCountUniform>(uniform));
            }
            else if (source == "date")
            {
                uniforms.emplace_back(std::make_unique<DateUniform>(uniform));
            }
            else if (source == "timer")
            {
                uniforms.emplace_back(std::make_unique<TimerUniform>(uniform));
            }
            else if (source == "pingpong")
            {
                uniforms.emplace_back(std::make_unique<PingPongUniform>(uniform));
            }
            else if (source == "random")
            {
                uniforms.emplace_back(std::make_unique<RandomUniform>(uniform));
            }
            else if (source == "key")
            {
                uniforms.emplace_back(std::make_unique<KeyUniform>(uniform));
            }
            else if (source == "mousebutton")
            {
                uniforms.emplace_back(std::make_unique<MouseButtonUniform>(uniform));
            }
            else if (source == "mousepoint")
            {
                uniforms.emplace_back(std::make_unique<MousePointUniform>(uniform));
            }
            else if (source == "mousedelta")
            {
                uniforms.emplace_back(std::make_unique<MouseDeltaUniform>(uniform));
            }
            else if (source == "bufready_depth")
            {
                uniforms.emplace_back(std::make_unique<DepthUniform>(uniform));
            }
        }
        return uniforms;
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    FrameTimeUniform::FrameTimeUniform(reshadefx::uniform_info uniformInfo)
    {
        if (auto source = std::ranges::find(uniformInfo.annotations, "source", &reshadefx::annotation::name);
            source->value.string_data != "frametime")
        {
            Logger::err("Tried to create a FrameTimeUniform from a non frametime uniform_info");
        }
        lastFrame = std::chrono::high_resolution_clock::now();
        offset    = uniformInfo.offset;
        size      = uniformInfo.size;
    }

    void FrameTimeUniform::update(void* mapedBuffer)
    {
        const auto                                     currentFrame = std::chrono::high_resolution_clock::now();
        const std::chrono::duration<float, std::milli> duration     = currentFrame - lastFrame;
        lastFrame                                                   = currentFrame;
        const float frametime                                       = duration.count();
        std::ranges::copy(AsBytes(frametime), static_cast<uint8_t*>(mapedBuffer) + offset);
    }

    FrameTimeUniform::~FrameTimeUniform() = default;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    FrameCountUniform::FrameCountUniform(reshadefx::uniform_info uniformInfo)
    {
        if (auto source = std::ranges::find(uniformInfo.annotations, "source", &reshadefx::annotation::name);
            source->value.string_data != "framecount")
        {
            Logger::err("Tried to create a FrameCountUniform from a non framecount uniform_info");
        }
        offset = uniformInfo.offset;
        size   = uniformInfo.size;
    }

    void FrameCountUniform::update(void* mapedBuffer)
    {
        std::ranges::copy(AsBytes(count), static_cast<uint8_t*>(mapedBuffer) + offset);
        count++;
    }

    FrameCountUniform::~FrameCountUniform() = default;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    DateUniform::DateUniform(reshadefx::uniform_info uniformInfo)
    {
        const auto source = std::ranges::find(uniformInfo.annotations, "source", &reshadefx::annotation::name);
        if (source->value.string_data != "date")
        {
            Logger::err("Tried to create a DateUniform from a non date uniform_info");
        }
        offset = uniformInfo.offset;
        size   = uniformInfo.size;
    }

    void DateUniform::update(void* mapedBuffer)
    {
        const auto        now         = std::chrono::system_clock::now();
        const std::time_t nowC        = std::chrono::system_clock::to_time_t(now);
        const auto*       currentTime = std::localtime(std::addressof(nowC));
        const auto        year        = 1900.0F + static_cast<float>(currentTime->tm_year);
        const auto        month       = 1.0F + static_cast<float>(currentTime->tm_mon);
        const auto        day         = static_cast<float>(currentTime->tm_mday);
        const auto        seconds     = static_cast<float>((((currentTime->tm_hour * 60) + currentTime->tm_min) * 60) + currentTime->tm_sec);
        const std::array  date        = {year, month, day, seconds};

        std::ranges::copy(date, static_cast<uint8_t*>(mapedBuffer) + offset);
    }

    DateUniform::~DateUniform() = default;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    TimerUniform::TimerUniform(reshadefx::uniform_info uniformInfo)
    {
        auto source = std::ranges::find(uniformInfo.annotations, "source", &reshadefx::annotation::name);
        if (source->value.string_data != "timer")
        {
            Logger::err("Tried to create a TimerUniform from a non timer uniform_info");
        }
        start  = std::chrono::high_resolution_clock::now();
        offset = uniformInfo.offset;
        size   = uniformInfo.size;
    }

    void TimerUniform::update(void* mapedBuffer)
    {
        const auto                                     currentFrame = std::chrono::high_resolution_clock::now();
        const std::chrono::duration<float, std::milli> duration     = currentFrame - start;
        const auto                                     timer        = duration.count();

        std::ranges::copy(AsBytes(timer), static_cast<uint8_t*>(mapedBuffer) + offset);
    }

    TimerUniform::~TimerUniform() = default;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    PingPongUniform::PingPongUniform(reshadefx::uniform_info uniformInfo)
    {
        auto source = std::ranges::find(uniformInfo.annotations, "source", &reshadefx::annotation::name);
        if (source->value.string_data != "pingpong")
        {
            Logger::err("Tried to create a PingPongUniform from a non pingpong uniform_info");
        }
        if (auto minAnnotation = std::ranges::find(uniformInfo.annotations, "min", &reshadefx::annotation::name);
            minAnnotation != std::cend(uniformInfo.annotations))
        {
            min = minAnnotation->type.is_floating_point() ? minAnnotation->value.as_float[0] : static_cast<float>(minAnnotation->value.as_int[0]);
        }
        if (auto maxAnnotation = std::ranges::find(uniformInfo.annotations, "max", &reshadefx::annotation::name);
            maxAnnotation != std::cend(uniformInfo.annotations))
        {
            max = maxAnnotation->type.is_floating_point() ? maxAnnotation->value.as_float[0] : static_cast<float>(maxAnnotation->value.as_int[0]);
        }
        if (auto smoothingAnnotation = std::ranges::find(uniformInfo.annotations, "smoothing", &reshadefx::annotation::name);
            smoothingAnnotation != std::cend(uniformInfo.annotations))
        {
            smoothing = smoothingAnnotation->type.is_floating_point() ? smoothingAnnotation->value.as_float[0]
                                                                      : static_cast<float>(smoothingAnnotation->value.as_int[0]);
        }
        if (auto stepAnnotation = std::ranges::find(uniformInfo.annotations, "step", &reshadefx::annotation::name);
            stepAnnotation != std::cend(uniformInfo.annotations))
        {
            stepMin =
                stepAnnotation->type.is_floating_point() ? stepAnnotation->value.as_float[0] : static_cast<float>(stepAnnotation->value.as_int[0]);
            stepMax =
                stepAnnotation->type.is_floating_point() ? stepAnnotation->value.as_float[1] : static_cast<float>(stepAnnotation->value.as_int[1]);
        }

        lastFrame = std::chrono::high_resolution_clock::now();

        offset = uniformInfo.offset;
        size   = uniformInfo.size;
    }

    void PingPongUniform::update(void* mapedBuffer)
    {
        const auto currentFrame = std::chrono::high_resolution_clock::now();

        const std::chrono::duration<float, std::ratio<1>> frameTime = currentFrame - lastFrame;

        float increment = stepMax == 0 ? stepMin : (stepMin + std::fmod(static_cast<float>(std::rand()), stepMax - stepMin + 1.0F));
        if (currentValue[1] >= 0)
        {
            increment = std::max(increment - std::max(0.0F, smoothing - (max - currentValue[0])), 0.05F);
            increment *= frameTime.count();

            if ((currentValue[0] += increment) >= max)
            {
                currentValue[0] = max, currentValue[1] = -1.0F;
            }
        }
        else
        {
            increment = std::max(increment - std::max(0.0F, smoothing - (currentValue[0] - min)), 0.05F);
            increment *= frameTime.count();

            if ((currentValue[0] -= increment) <= min)
            {
                currentValue[0] = min, currentValue[1] = 1.0F;
            }
        }
        std::ranges::copy(currentValue, static_cast<uint8_t*>(mapedBuffer) + offset);
    }

    PingPongUniform::~PingPongUniform() = default;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    RandomUniform::RandomUniform(reshadefx::uniform_info uniformInfo)
    {
        if (auto source = std::ranges::find(uniformInfo.annotations, "source", &reshadefx::annotation::name); source->value.string_data != "random")
        {
            Logger::err("Tried to create a RandomUniform from a non random uniform_info");
        }
        if (auto minAnnotation = std::ranges::find(uniformInfo.annotations, "min", &reshadefx::annotation::name);
            minAnnotation != uniformInfo.annotations.end())
        {
            min = minAnnotation->type.is_integral() ? minAnnotation->value.as_int[0] : static_cast<int>(minAnnotation->value.as_float[0]);
        }
        if (auto maxAnnotation = std::ranges::find(uniformInfo.annotations, "max", &reshadefx::annotation::name);
            maxAnnotation != uniformInfo.annotations.end())
        {
            max = maxAnnotation->type.is_integral() ? maxAnnotation->value.as_int[0] : static_cast<int>(maxAnnotation->value.as_float[0]);
        }
        offset = uniformInfo.offset;
        size   = uniformInfo.size;
    }

    void RandomUniform::update(void* mapedBuffer)
    {
        const int32_t value = min + (std::rand() % (max - min + 1));

        std::ranges::copy(AsBytes(value), static_cast<uint8_t*>(mapedBuffer) + offset);
    }

    RandomUniform::~RandomUniform() = default;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    KeyUniform::KeyUniform(reshadefx::uniform_info uniformInfo)
    {
        if (auto source = std::ranges::find(uniformInfo.annotations, "source", &reshadefx::annotation::name); source->value.string_data != "key")
        {
            Logger::err("Tried to create a KeyUniform from a non key uniform_info");
        }
        offset = uniformInfo.offset;
        size   = uniformInfo.size;
    }

    void KeyUniform::update(void* mapedBuffer)
    {
        const VkBool32 keyDown = VK_FALSE; // TODO
        std::ranges::copy(AsBytes(keyDown), static_cast<uint8_t*>(mapedBuffer) + offset);
    }

    KeyUniform::~KeyUniform() = default;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    MouseButtonUniform::MouseButtonUniform(reshadefx::uniform_info uniformInfo)
    {
        if (auto source = std::ranges::find(uniformInfo.annotations, "source", &reshadefx::annotation::name);
            source->value.string_data != "mousebutton")
        {
            Logger::err("Tried to create a MouseButtonUniform from a non mousebutton uniform_info");
        }
        offset = uniformInfo.offset;
        size   = uniformInfo.size;
    }

    void MouseButtonUniform::update(void* mapedBuffer)
    {
        const static VkBool32 keyDown = VK_FALSE; // TODO
        std::ranges::copy(AsBytes(keyDown), static_cast<uint8_t*>(mapedBuffer) + offset);
    }

    MouseButtonUniform::~MouseButtonUniform() = default;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    MousePointUniform::MousePointUniform(reshadefx::uniform_info uniformInfo)
    {
        if (auto source = std::ranges::find(uniformInfo.annotations, "source", &reshadefx::annotation::name);
            source->value.string_data != "mousepoint")
        {
            Logger::err("Tried to create a MousePointUniform from a non mousepoint uniform_info");
        }
        offset = uniformInfo.offset;
        size   = uniformInfo.size;
    }

    void MousePointUniform::update(void* mapedBuffer)
    {
        constexpr static std::array point{0.0F, 0.0F}; // TODO
        std::ranges::copy(point, static_cast<uint8_t*>(mapedBuffer) + offset);
    }

    MousePointUniform::~MousePointUniform() = default;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    MouseDeltaUniform::MouseDeltaUniform(reshadefx::uniform_info uniformInfo)
    {
        if (auto source = std::ranges::find(uniformInfo.annotations, "source", &reshadefx::annotation::name);
            source->value.string_data != "mousedelta")
        {
            Logger::err("Tried to create a MouseDeltaUniform from a non mousedelta uniform_info");
        }
        offset = uniformInfo.offset;
        size   = uniformInfo.size;
    }
    void MouseDeltaUniform::update(void* mapedBuffer)
    {
        constexpr static std::array delta{0.0F, 0.0F}; // TODO
        std::ranges::copy(delta, static_cast<uint8_t*>(mapedBuffer) + offset);
    }
    MouseDeltaUniform::~MouseDeltaUniform() = default;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    DepthUniform::DepthUniform(reshadefx::uniform_info uniformInfo)
    {
        if (auto source = std::ranges::find(uniformInfo.annotations, "source", &reshadefx::annotation::name);
            source->value.string_data != "bufready_depth")
        {
            Logger::err("Tried to create a DepthUniform from a non bufready_depth uniform_info");
        }
        offset = uniformInfo.offset;
        size   = uniformInfo.size;
    }

    void DepthUniform::update(void* mapedBuffer)
    {
        const VkBool32 hasDepth = VK_FALSE; // TODO
        std::ranges::copy(AsBytes(hasDepth), static_cast<uint8_t*>(mapedBuffer) + offset);
    }

    DepthUniform::~DepthUniform() = default;

} // namespace vkBasalt
