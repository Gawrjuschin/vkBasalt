#ifndef RESHADE_UNIFORMS_HPP_INCLUDED
#define RESHADE_UNIFORMS_HPP_INCLUDED

#include <array>
#include <cstdint>
#include <vector>
#include <chrono>
#include <memory>

#include <effect_module.hpp>

namespace vkBasalt
{
    void enumerateReshadeUniforms(reshadefx::module module);

    class ReshadeUniform
    {
    public:
        ReshadeUniform()                                 = default;
        ReshadeUniform(const ReshadeUniform&)            = delete;
        ReshadeUniform& operator=(const ReshadeUniform&) = delete;
        ReshadeUniform(ReshadeUniform&&)                 = delete;
        ReshadeUniform& operator=(ReshadeUniform&&)      = delete;
        virtual ~ReshadeUniform()                        = default;

        void virtual update(void* mapedBuffer) = 0;

    protected:
        uint32_t offset{};
        uint32_t size{};
    };

    std::vector<std::unique_ptr<ReshadeUniform>> createReshadeUniforms(reshadefx::module module);

    class FrameTimeUniform final : public ReshadeUniform
    {
    public:
        FrameTimeUniform(reshadefx::uniform_info uniformInfo);
        FrameTimeUniform(const FrameTimeUniform&)            = delete;
        FrameTimeUniform& operator=(const FrameTimeUniform&) = delete;
        FrameTimeUniform(FrameTimeUniform&&)                 = delete;
        FrameTimeUniform& operator=(FrameTimeUniform&&)      = delete;
        ~FrameTimeUniform() override;

        void update(void* mapedBuffer) override;

    private:
        std::chrono::time_point<std::chrono::high_resolution_clock> lastFrame;
    };

    class FrameCountUniform final : public ReshadeUniform
    {
    public:
        FrameCountUniform(reshadefx::uniform_info uniformInfo);
        FrameCountUniform(const FrameCountUniform&)            = delete;
        FrameCountUniform& operator=(const FrameCountUniform&) = delete;
        FrameCountUniform(FrameCountUniform&&)                 = delete;
        FrameCountUniform& operator=(FrameCountUniform&&)      = delete;
        ~FrameCountUniform() override;

        void update(void* mapedBuffer) override;

    private:
        int32_t count = 0;
    };

    class DateUniform final : public ReshadeUniform
    {
    public:
        DateUniform(reshadefx::uniform_info uniformInfo);
        DateUniform(const DateUniform&)            = delete;
        DateUniform& operator=(const DateUniform&) = delete;
        DateUniform(DateUniform&&)                 = delete;
        DateUniform& operator=(DateUniform&&)      = delete;
        ~DateUniform() override;

        void update(void* mapedBuffer) override;
    };

    class TimerUniform final : public ReshadeUniform
    {
    public:
        TimerUniform(reshadefx::uniform_info uniformInfo);
        TimerUniform(const TimerUniform&)            = delete;
        TimerUniform& operator=(const TimerUniform&) = delete;
        TimerUniform(TimerUniform&&)                 = delete;
        TimerUniform& operator=(TimerUniform&&)      = delete;
        ~TimerUniform() override;

        void update(void* mapedBuffer) override;

    private:
        std::chrono::time_point<std::chrono::high_resolution_clock> start;
    };

    class PingPongUniform final : public ReshadeUniform
    {
    public:
        PingPongUniform(reshadefx::uniform_info uniformInfo);
        PingPongUniform(const PingPongUniform&)            = delete;
        PingPongUniform& operator=(const PingPongUniform&) = delete;
        PingPongUniform(PingPongUniform&&)                 = delete;
        PingPongUniform& operator=(PingPongUniform&&)      = delete;
        ~PingPongUniform() override;

        void update(void* mapedBuffer) override;

    private:
        std::chrono::time_point<std::chrono::high_resolution_clock> lastFrame;

        float min             = 0.0F;
        float max             = 0.0F;
        float stepMin         = 0.0F;
        float stepMax         = 0.0F;
        float smoothing       = 0.0F;
        std::array<float, 2U> currentValue{0.0F, 1.0F};
    };

    class RandomUniform final : public ReshadeUniform
    {
    public:
        RandomUniform(reshadefx::uniform_info uniformInfo);
        RandomUniform(const RandomUniform&)            = delete;
        RandomUniform& operator=(const RandomUniform&) = delete;
        RandomUniform(RandomUniform&&)                 = delete;
        RandomUniform& operator=(RandomUniform&&)      = delete;
        ~RandomUniform() override;

        void update(void* mapedBuffer) override;

    private:
        int max = 0;
        int min = 0;
    };

    class KeyUniform final : public ReshadeUniform
    {
    public:
        KeyUniform(reshadefx::uniform_info uniformInfo);
        KeyUniform(const KeyUniform&)            = delete;
        KeyUniform& operator=(const KeyUniform&) = delete;
        KeyUniform(KeyUniform&&)                 = delete;
        KeyUniform& operator=(KeyUniform&&)      = delete;
        ~KeyUniform() override;

        void update(void* mapedBuffer) override;
    };

    class MouseButtonUniform final : public ReshadeUniform
    {
    public:
        MouseButtonUniform(reshadefx::uniform_info uniformInfo);
        MouseButtonUniform(const MouseButtonUniform&)            = delete;
        MouseButtonUniform& operator=(const MouseButtonUniform&) = delete;
        MouseButtonUniform(MouseButtonUniform&&)                 = delete;
        MouseButtonUniform& operator=(MouseButtonUniform&&)      = delete;
        ~MouseButtonUniform() override;

        void update(void* mapedBuffer) override;
    };

    class MousePointUniform final : public ReshadeUniform
    {
    public:
        MousePointUniform(reshadefx::uniform_info uniformInfo);
        MousePointUniform(const MousePointUniform&)            = delete;
        MousePointUniform& operator=(const MousePointUniform&) = delete;
        MousePointUniform(MousePointUniform&&)                 = delete;
        MousePointUniform& operator=(MousePointUniform&&)      = delete;
        ~MousePointUniform() override;

        void update(void* mapedBuffer) override;
    };

    class MouseDeltaUniform final : public ReshadeUniform
    {
    public:
        MouseDeltaUniform(reshadefx::uniform_info uniformInfo);
        MouseDeltaUniform(const MouseDeltaUniform&)            = delete;
        MouseDeltaUniform& operator=(const MouseDeltaUniform&) = delete;
        MouseDeltaUniform(MouseDeltaUniform&&)                 = delete;
        MouseDeltaUniform& operator=(MouseDeltaUniform&&)      = delete;
        ~MouseDeltaUniform() override;

        void update(void* mapedBuffer) override;
    };

    class DepthUniform final : public ReshadeUniform
    {
    public:
        DepthUniform(reshadefx::uniform_info uniformInfo);
        DepthUniform(const DepthUniform&)            = delete;
        DepthUniform& operator=(const DepthUniform&) = delete;
        DepthUniform(DepthUniform&&)                 = delete;
        DepthUniform& operator=(DepthUniform&&)      = delete;
        ~DepthUniform() override;

        void update(void* mapedBuffer) override;
    };
} // namespace vkBasalt

#endif // RESHADE_UNIFORMS_HPP_INCLUDED
