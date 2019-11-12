#include "main.hpp"

#include "message.hpp"

template<int key>
auto frame_filter(rxcpp::observable<Message> messages)
{
    spdlog::critical("frame_filter: {}", key);
    return messages
        .tap([](Message msg) {
            spdlog::info("Message filtered on frame{}: {}", key, msg->data);
        });
};

using MessageOperator = std::function<rxcpp::observable<Message>(rxcpp::observable<Message>)>;
std::unordered_map<Message::KeyType, MessageOperator> message_filter = {
    { "frame0", frame_filter<0> },
    { "frame1", frame_filter<1> },
    { "frame2", frame_filter<2> },
    { "frame3", frame_filter<3> },
    { "frame4", frame_filter<4> },
    { "frame5", frame_filter<5> },
    { "frame6", frame_filter<6> },
    { "frame7", frame_filter<7> },
    { "frame8", frame_filter<8> },
    { "frame9", frame_filter<9> },
};

auto test_group_by() -> std::function<rxcpp::observable<Message>(rxcpp::observable<Message>)>
{
    return [](rxcpp::observable<Message> messages) {
        spdlog::error("test_group_by()");
        return messages |
            rxcpp::rxo::group_by(
                [](Message msg) {
                    return msg->key;
                },
                [](Message msg) {
                    return msg;
                }) |
            rxcpp::rxo::map([](rxcpp::grouped_observable<Message::KeyType, Message> group) -> rxcpp::observable<Message> {
                auto found = message_filter.find(group.get_key());
                if (found == message_filter.end())
                {
                    return group
                        .tap([](Message msg) {
                            spdlog::error("Key not found: {}", msg->key);
                        });
                }
                else
                {
                    return group | found->second;
                }
            }) |
            rxcpp::rxo::merge();
    };
}
