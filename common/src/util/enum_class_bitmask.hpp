#pragma once

#include <type_traits>

namespace util {

template <typename Enum>
struct enable_bitmask
{
	static inline constexpr bool value = false;
};

template <typename Enum>
auto enum_underlying(Enum value)
{
	return static_cast<std::underlying_type_t<Enum>>(value);
}

}

template <typename Enum, class = std::enable_if_t<util::enable_bitmask<Enum>::value>>
constexpr Enum operator|(Enum l, Enum r)
{
	using Under_t = std::underlying_type_t<Enum>;
	return static_cast<Enum>(static_cast<Under_t>(l) | static_cast<Under_t>(r));
}

template <typename Enum, class = std::enable_if_t<util::enable_bitmask<Enum>::value>>
constexpr Enum operator&(Enum l, Enum r)
{
	using Under_t = std::underlying_type_t<Enum>;
	return static_cast<Enum>(static_cast<Under_t>(l) & static_cast<Under_t>(r));
}

template <typename Enum, class = std::enable_if_t<util::enable_bitmask<Enum>::value>>
constexpr Enum operator^(Enum l, Enum r)
{
	using Under_t = std::underlying_type_t<Enum>;
	return static_cast<Enum>(static_cast<Under_t>(l) & static_cast<Under_t>(r));
}

template <typename Enum, class = std::enable_if_t<util::enable_bitmask<Enum>::value>>
constexpr Enum operator~(Enum val)
{
	using Under_t = std::underlying_type_t<Enum>;
	return static_cast<Enum>(~static_cast<Under_t>(val));
}

template <typename Enum, class = std::enable_if_t<util::enable_bitmask<Enum>::value>>
constexpr Enum& operator|=(Enum &l, Enum r)
{
	l = l | r;
	return l;
}

template <typename Enum, class = std::enable_if_t<util::enable_bitmask<Enum>::value>>
constexpr Enum& operator&=(Enum &l, Enum r)
{
	l = l & r;
	return l;
}

template <typename Enum, class = std::enable_if_t<util::enable_bitmask<Enum>::value>>
constexpr Enum& operator^=(Enum &l, Enum r)
{
	l = l ^ r;
	return l;
}

template <typename Enum, class = std::enable_if_t<util::enable_bitmask<Enum>::value>>
constexpr bool operator!(Enum val)
{
	using Under_t = std::underlying_type_t<Enum>;
	return !static_cast<Under_t>(val);
}