#include "main.hpp"

auto Engine::test_group_by() -> std::function<rxcpp::observable<size_t>(rxcpp::observable<size_t>)>
{
    return [this](rxcpp::observable<size_t> frames) {
        spdlog::error("test_group_by()");
        return frames |
            rxcpp::rxo::group_by(
                [](size_t frame) {
                    return frame % 2 == 0;
                },
                [](size_t frame) {
                    return frame;
                }) |
            rxcpp::rxo::map([this](rxcpp::grouped_observable<bool, size_t> group) -> rxcpp::observable<size_t> {
                if (group.get_key())
                {
                    spdlog::info("{}", __LINE__);  // NOTE: this is called only once
                    return group |
                        rxcpp::rxo::observe_on(get_main_worker()) |
                        rxcpp::rxo::map([](size_t frame) {
                            spdlog::warn("Even frame {}", frame);
                            std::this_thread::sleep_for(std::chrono::milliseconds(3));
                            return frame;
                        });
                }
                else
                {
                    spdlog::info("{}", __LINE__);  // NOTE: this is called only once
                    return group |
                        rxcpp::rxo::observe_on(get_job_worker()) |
                        rxcpp::rxo::map([](size_t frame) {
                            spdlog::warn("Odd frame {}", frame);
                            return frame;
                        });
                }
            }) |
            rxcpp::rxo::merge();
    };
}
