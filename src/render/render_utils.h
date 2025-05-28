#pragma once

#define VK_ERROR_CHECK(call, msg)                 \
([&]() -> decltype(auto) {                            \
    auto resultValue = (call);                        \
    if (resultValue.result != vk::Result::eSuccess) { \
        throw std::runtime_error(msg);            \
    }                                                 \
    return resultValue.value;                         \
}())

#define VK_ERROR_AND_EMPRY_CHECK(call, msg1, msg2) \
([&]() -> decltype(auto) {                                 \
    auto resultValue = (call);                             \
    if (resultValue.result != vk::Result::eSuccess) {      \
        throw std::runtime_error(msg1);                \
    }                                                      \
    if (resultValue.value.empty()) {                       \
        throw std::runtime_error(msg2);                \
    }                                                      \
    return resultValue.value;                              \
}())

#define INSERT_ELEMENTS_M(set, source, member) \
set.reserve(source.size());                    \
for (const auto& item : source) {              \
    set.emplace(item.member);                  \
}

#define INSERT_ELEMENTS(set, source) \
set.reserve(source.size());          \
for (const auto& item : source) {    \
    set.emplace(item);               \
}

#define CONTAINER_COUNT(container) static_cast<uint32_t>(container.size())
#define CONTAINER_RANGE(container) container.begin(), container.end()