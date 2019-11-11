#include "main.hpp"

auto Engine::test_nested_observable() -> std::function<rxcpp::observable<size_t>(rxcpp::observable<size_t>)>
{
    return [this](rxcpp::observable<size_t> frames) {
        spdlog::error("test_nested_observable()");
        return frames |
            rxcpp::rxo::map([=](size_t frame) {
                spdlog::info("nested observable");
                return rxcpp::rxs::just(frame) |
                    calc_fps() |
                    rxcpp::rxo::map([=](double fps) {
                        spdlog::warn("FPS {} on {}", fps, frame);
                        return std::make_tuple(frame, fps);
                    });
            }) |
            rxcpp::rxo::merge() |
            rxcpp::rxo::map(rxcpp::rxu::apply_to([](size_t frame, double) {
                return static_cast<size_t>(frame);
            }));
    };
}
