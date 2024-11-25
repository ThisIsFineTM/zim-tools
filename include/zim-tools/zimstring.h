#ifndef OPENZIM_STRING_H
#define OPENZIM_STRING_H

#include <iosfwd>
#include <string>
#include <string_view>
#include <type_traits>

//using ZimChar = char8_t;
//using ZimString = std::basic_string<ZimChar>;
//using ZimStringView = std::basic_string_view<ZimChar>;
//using ZimStreampos = std::fpos<std::char_traits<ZimChar>::state_type>;
//
//constexpr ZimChar operator""_ZC(char);
//constexpr ZimChar operator""_ZC(char8_t);
//
//template <typename T>
//const ZimChar* zimCharCast(const T* c) noexcept
//{
//    static_assert(std::is_convertible_v<T, ZimChar>
//                  && sizeof(T) == sizeof(ZimChar)
//    );
//    return reinterpret_cast<const ZimChar*>(c);
//}
//
//template <typename T>
//ZimChar zimCharCast(T c) noexcept
//{
//    static_assert(std::is_convertible_v<T, ZimChar>
//                  && sizeof(T) == sizeof(ZimChar)
//    );
//    return static_cast<ZimChar>(c);
//}
//
//template <typename T>
//ZimString zimStrCast(std::basic_string_view<T> str)
//{
//    static_assert(std::is_convertible_v<T, ZimChar>
//                  && sizeof(T) == sizeof(ZimChar)
//    );
//    return ZimString(zimCharCast(str.data()), str.size());
//}
//
//template <typename T>
//ZimStringView zimStrViewCast(std::basic_string_view<T> str) noexcept
//{
//    static_assert(std::is_convertible_v<T, ZimChar>
//                  && sizeof(T) == sizeof(ZimChar)
//    );
//    return ZimStringView(zimCharCast(str.data()), str.size());
//}
//
////constexpr ZimString operator""_ZS(const char*);
//inline ZimString operator""_ZS(const char* str, std::size_t sz) {
//    return ZimString(zimCharCast(str), sz);
//}
//
//inline ZimString operator""_ZS(const char8_t* str, std::size_t sz) {
//    return ZimString(zimCharCast(str), sz);
//}
//
////constexpr ZimStringView operator""_ZSV(const char*) noexcept;
//inline ZimStringView operator""_ZSV(const char* str, std::size_t sz) noexcept {
//    return ZimStringView(zimCharCast(str), sz);
//}
//
//inline ZimStringView operator""_ZSV(const char8_t* str, std::size_t sz) noexcept {
//    return ZimStringView(zimCharCast(str), sz);
//}



#endif  // OPENZIM_STRING_H
