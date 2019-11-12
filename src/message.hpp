#pragma once

#include <string>
#include <memory>
#include <unordered_map>

struct Message
{
    using KeyType = std::string;
    using DataType = std::string;

    struct Data
    {
        KeyType key;
        DataType data;
    };

    Data* operator->() { return data.get(); }
    const Data* operator->() const { return data.get(); }

private:
    std::shared_ptr<Data> data = std::make_shared<Data>();
};

auto test_group_by() -> std::function<rxcpp::observable<Message>(rxcpp::observable<Message>)>;
