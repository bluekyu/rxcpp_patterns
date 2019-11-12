#include "main.hpp"

#include "message.hpp"

Engine::Engine() : frames_creator_(frame_subject_.get_subscriber())
{
}

void Engine::setup()
{
    spdlog::set_level(spdlog::level::debug);
    spdlog::set_pattern("[%H:%M:%S.%e] [%t] [%^%l%$] %v");

    main_worker_ = rxcpp::rxsc::make_run_loop(loop_).create_worker();
    job_worker_ = rxcpp::schedulers::make_scheduler<rxcpp::schedulers::new_thread>().create_worker();

    frames_ = frame_subject_.get_observable()
                  .observe_on(rxcpp::identity_same_worker(job_worker_))
                  .tap([](size_t frame) {
                      spdlog::debug("Start frame: {}", frame);
                  })
                  .publish();

    frames_ |
        rxcpp::rxo::subscribe<size_t>(
            lifetime_,
            [this](size_t frame) {
                rendering_frame_ = frame;
                spdlog::debug("Frame {} is rendering ...", frame);
            },
            [](std::exception_ptr eptr) {
                try
                {
                    std::rethrow_exception(eptr);
                }
                catch (std::exception err)
                {
                    spdlog::critical("{}", err.what());
                }
            });

    frames_ |
        rxcpp::rxo::map([](size_t frame) {
            Message msg;
            msg->key = "frame" + std::to_string(frame % 10);
            msg->data = std::to_string(frame);

            spdlog::debug("Begin message: {}", msg->key);

            return msg;
        }) |
        test_group_by() |
        rxcpp::rxo::subscribe<Message>(
            lifetime_,
            [](Message msg) {
                spdlog::debug("End message: {}", msg->key);
            },
            [](std::exception_ptr eptr) {
                try
                {
                    std::rethrow_exception(eptr);
                }
                catch (std::exception err)
                {
                    spdlog::critical("{}", err.what());
                }
            });

    frames_ |
        test_nested_observable() |
        rxcpp::rxo::observe_on(get_main_worker()) |
        rxcpp::rxo::subscribe<size_t>(
            lifetime_,
            [](size_t frame) {
                spdlog::debug("End test_nested_observable: {}", frame);
            },
            [](std::exception_ptr eptr) {
                try
                {
                    std::rethrow_exception(eptr);
                }
                catch (std::exception err)
                {
                    spdlog::critical("{}", err.what());
                }
            });

    frames_ |
        test_polling() |
        rxcpp::rxo::subscribe<size_t>(
            lifetime_,
            [](size_t frame) {
                spdlog::debug("End test_polling: {}", frame);
            },
            [](std::exception_ptr eptr) {
                try
                {
                    std::rethrow_exception(eptr);
                }
                catch (std::exception err)
                {
                    spdlog::critical("{}", err.what());
                }
            });

    frames_.connect(lifetime_);

    start_time_ = std::chrono::high_resolution_clock::now();
    frames_creator_.on_next(rendering_frame_ + 1);
}

void Engine::update()
{
    while (!loop_.empty() && loop_.peek().when < loop_.now())
    {
        loop_.dispatch();
    }
}

bool Engine::render()
{
    if (swapped_frame_ != rendering_frame_)
    {
        spdlog::critical("Swap Buffer: {}", rendering_frame_);
        std::this_thread::sleep_for(std::chrono::microseconds(10));
        swapped_frame_ = rendering_frame_;

        frames_creator_.on_next(swapped_frame_ + 1);
    }
    return true;
}

auto Engine::calc_fps() -> std::function<rxcpp::observable<double>(rxcpp::observable<size_t>)>
{
    return [this](rxcpp::observable<size_t> frames) {
        return frames |
            rxcpp::rxo::map([this](size_t frame) {
                spdlog::info("Calculate FPS on frame {}", frame);
                return frame / std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - start_time_).count() * 1000.0;
            });
    };
}

// misuse case
/*
auto Engine::test_filter() -> std::function<rxcpp::observable<size_t>(rxcpp::observable<size_t>)>
{
    return [this](rxcpp::observable<size_t> frames) {
        spdlog::error("{}", __LINE__);

        auto even_stream =
            frames |
            rxcpp::rxo::filter([](size_t frame) {
                return frame % 2 == 0;
            }) |
            rxcpp::rxo::observe_on(*main_worker_) |
            rxcpp::rxo::map([](size_t frame) {
                spdlog::info("Even frame {}", frame);
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                return frame;
            }) |
            rxcpp::rxo::publish() |
            rxcpp::rxo::ref_count() | rxcpp::rxo::as_dynamic();

        auto odd_stream =
            frames |
            rxcpp::rxo::filter([](size_t frame) {
                return frame % 2 != 0;
            }) |
            rxcpp::rxo::observe_on(*job_worker_) |
            rxcpp::rxo::map([](size_t frame) {
                spdlog::warn("Odd frame {}", frame);
                return frame;
            }) |
            rxcpp::rxo::publish() |
            rxcpp::rxo::ref_count() | rxcpp::rxo::as_dynamic();

        return even_stream |
            rxcpp::rxo::merge(odd_stream) |
            rxcpp::rxo::observe_on(*job_worker_);
            // stream is out of order.
    };
}
*/

int main(int, char**)
{
    Engine engine;

    engine.setup();
    while (true)
    {
        engine.update();
        engine.render();
    }

    return 0;
}
