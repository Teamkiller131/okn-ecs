#pragma once

#include <okn/ecs/ecs_types.hpp>
#include <okn/core/api/hash.hpp>
#include <type_traits>
#include <string_view>

namespace okn::ecs {

template <class T>
struct ComponentTraits {
    static constexpr bool is_tag = false;
};

template <class T>
concept EcbComponent = std::is_trivially_copyable_v<T> && !std::is_pointer_v<T>;

struct ComponentInfo {
    ComponentTypeId type_id = 0;
    usize size = 0;
    usize alignment = 0;
    std::string_view name;
    void (*constructor)(void* ptr) = nullptr;
    void (*destructor)(void* ptr) = nullptr;
    void (*move_constructor)(void* dst, void* src) = nullptr;
    void (*copy_constructor)(void* dst, const void* src) = nullptr;

    template <class T>
    static auto from_type() -> ComponentInfo {
        ComponentInfo info{};
        info.type_id = hash_type<T>();
        info.size = sizeof(T);
        info.alignment = alignof(T);
        info.name = typeid(T).name();
        if constexpr (!std::is_trivially_constructible_v<T>) {
            info.constructor = [](void* p) { new (p) T{}; };
            info.destructor = [](void* p) { static_cast<T*>(p)->~T(); };
            info.move_constructor = [](void* d, void* s) { new (d) T(std::move(*static_cast<T*>(s))); };
            info.copy_constructor = [](void* d, const void* s) { new (d) T(*static_cast<const T*>(s)); };
        }
        return info;
    }

private:
    template <class T>
    static auto hash_type() -> ComponentTypeId {
#if defined(_MSC_VER)
        constexpr std::string_view sig = __FUNCSIG__;
#else
        constexpr std::string_view sig = __PRETTY_FUNCTION__;
#endif
        return okn::core::hash_string_view(sig);
    }
};

inline auto component_type_id(std::string_view name) -> ComponentTypeId {
    return okn::core::hash_string_view(name);
}

} // namespace okn::ecs
