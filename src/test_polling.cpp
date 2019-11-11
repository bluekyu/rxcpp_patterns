#include "main.hpp"

auto Engine::test_polling() -> std::function<rxcpp::observable<size_t>(rxcpp::observable<size_t>)>
{
    return [this](rxcpp::observable<size_t> frames) {
        spdlog::error("test_polling()");
        return frames |
            rxcpp::rxo::filter([](size_t frame) {
                return frame % 100 == 0;
            }) |
            rxcpp::rxo::map([](size_t frame) {
                return rxcpp::rxs::interval(std::chrono::milliseconds(5)) |
                    rxcpp::rxo::filter([](int) {
                        static int k = 0;
                        ++k;
                        if (k > 10)
                        {
                            k = 0;
                            return true;
                        }
                        else
                        {
                            return false;
                        }
                    }) |
                    rxcpp::rxo::take(1) |
                    rxcpp::rxo::map([frame](auto) {
                        spdlog::info("Polled: {}", frame);
                        return frame;
                    });
            }) |
            rxcpp::rxo::merge();
    };
}
