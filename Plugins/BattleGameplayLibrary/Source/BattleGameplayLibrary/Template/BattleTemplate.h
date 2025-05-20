#pragma once
#include <string_view>

#if WITH_EDITOR
#define BATTLE_INLINE 
#else
#define BATTLE_INLINE inline
#endif


//https://stackoverflow.com/questions/56466282/stop-compilation-if-if-constexpr-does-not-match
//https://stackoverflow.com/questions/38304847/constexpr-if-and-static-assert
template<class>
inline constexpr bool dependent_false_v = false;

template <auto A>
inline constexpr bool dependent_v = A;

template <typename T>
struct is_complete_helper {
    template <typename U>
    static auto test(U*)  -> std::integral_constant<bool, sizeof(U) == sizeof(U)>;
    static auto test(...) -> std::false_type;
    using type = decltype(test((T*)0));
};

template <typename T>
struct TIsComplete : is_complete_helper<T>::type {};

#define BATTLE_GENERATE_MEMBER_FUNCTION_CHECK(FUNC)\
template <typename T, typename... Args>\
class THas##FUNC##Function\
{\
	template <typename C, typename = decltype( DeclVal<C>().FUNC(DeclVal<Args>()...) )>\
	static uint8 test(int32);\
	template <typename A>\
	static uint16 test(...);\
public:\
	enum { Value = sizeof(test<T>(0)) == sizeof(uint8) };\
};

template <typename T>
class TIsUStructType
{
	template <typename A>
	static uint8 Check(decltype(&A::StaticStruct));
	template <typename B>
	static uint16 Check(...);

public:
	enum { Value = sizeof(Check<T>(0)) == sizeof(uint8) };
};

template <typename T>
class TIsUClassType
{
	template <typename A>
	static uint8 Check(decltype(&A::StaticClass));
	template <typename B>
	static uint16 Check(...);

public:
	enum { Value = sizeof(Check<T>(0)) == sizeof(uint8) };
};


template <typename T, typename TEnableIf<TIsUClassType<T>::Value>::Type* = nullptr>
FORCEINLINE static decltype(auto) GetUEObjectType()
{
	return T::StaticClass();
}

template <typename T, typename TEnableIf<TIsUStructType<T>::Value>::Type* = nullptr>
FORCEINLINE static decltype(auto) GetUEObjectType()
{
	return T::StaticStruct();
}

#if defined(_MSC_VER) && !defined(__clang__)
#define SIG __FUNCSIG__
#define SIG_STARTCHAR '<'
#define SIG_ENDCHAR '>'
#else
	#define SIG __PRETTY_FUNCTION__
	#define SIG_STARTCHAR '='
	#define SIG_ENDCHAR ']'
#endif

template <typename Type>
[[nodiscard]] constexpr auto stripped_type_name()
{
#ifdef SIG
	std::string_view pretty_function{SIG};
	auto first = pretty_function.find_first_not_of(' ', pretty_function.find_first_of(SIG_STARTCHAR) + 1);
	auto value = pretty_function.substr(first, pretty_function.find_last_of(SIG_ENDCHAR) - first);
	return value.data();
#else
	return "";
#endif
}

template <typename Type>
[[nodiscard]] constexpr auto stripped_type_name_view()
{
#ifdef SIG
	std::string_view pretty_function{SIG};
	auto first = pretty_function.find_first_not_of(' ', pretty_function.find_first_of(SIG_STARTCHAR) + 1);
	auto value = pretty_function.substr(first, pretty_function.find_last_of(SIG_ENDCHAR) - first);
	return value;
#else
	return {};
#endif
}

template <typename Type>
[[nodiscard]] constexpr TStringView<ANSICHAR> stripped_type_stringviewname()
{
#ifdef SIG
	std::string_view pretty_function{SIG};
	auto first = pretty_function.find_first_not_of(' ', pretty_function.find_first_of(SIG_STARTCHAR) + 1);
	auto value = pretty_function.substr(first, pretty_function.find_last_of(SIG_ENDCHAR) - first);
	return value.data();
#else
	return "";
#endif
}

template <typename Type>
[[nodiscard]] FString stripped_type_unrealname()
{
#ifdef SIG
	std::string_view pretty_function{SIG};
	auto first = pretty_function.find_first_not_of(' ', pretty_function.find_first_of(SIG_STARTCHAR) + 1);
	auto value = pretty_function.substr(first, pretty_function.find_last_of(SIG_ENDCHAR) - first);
	return value.data();
#else
	return TEXT("");
#endif
}

#undef SIG
#undef SIG_STARTCHAR
#undef SIG_ENDCHAR
