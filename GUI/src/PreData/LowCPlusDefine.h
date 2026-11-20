#pragma once

#if !defined(_MSC_VER) && __cplusplus < 201703L
namespace std
{
template<typename T, typename... Args>
unique_ptr<T> make_unique(Args&&... args)
{
    return unique_ptr<T>(new T(std::forward<Args>(args)...));
}
}
#endif
