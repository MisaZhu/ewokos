// Copyright (C) 2021 syn1w
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <type_traits>

namespace tiny_stl {

template <typename T>
constexpr T* addressof(T& val) noexcept {
    return reinterpret_cast<T*>(
        &const_cast<char&>(reinterpret_cast<const volatile char&>(val)));
}

template <typename T>
const T* addressof(const T&&) = delete;

// wraps a static constant of specified type.
// as the base class for trait types.
template <typename T, T val>
struct integral_constant {
    static constexpr T value = val;
    using value_type = T;
    using type = integral_constant<T, val>;

    // type conversion
    constexpr operator T() const noexcept {
        return value;
    }

    constexpr T operator()() const noexcept {
        return value;
    }
};

using true_type = integral_constant<bool, true>;
using false_type = integral_constant<bool, false>;

template <bool B>
using bool_constant = integral_constant<bool, B>;

template <typename T1, typename T2>
struct is_same : false_type {};

template <typename T>
struct is_same<T, T> : true_type {};

template <typename T1, typename T2>
constexpr bool is_same_v = is_same<T1, T2>::value;

// is_function
template <typename>
struct is_function : false_type {};

// specialization for regular functions
template <typename Ret, typename... Args>
struct is_function<Ret(Args...)> : true_type {};

// specialization for variadic functions such as printf
template <typename Ret, typename... Args>
struct is_function<Ret(Args..., ...)> : true_type {};

// specialization for function types that have cv-qualifiers
template <typename Ret, typename... Args>
struct is_function<Ret(Args...) const> : true_type {};

template <typename Ret, typename... Args>
struct is_function<Ret(Args...) volatile> : true_type {};

template <typename Ret, typename... Args>
struct is_function<Ret(Args...) const volatile> : true_type {};

template <typename Ret, typename... Args>
struct is_function<Ret(Args..., ...) const> : true_type {};

template <typename Ret, typename... Args>
struct is_function<Ret(Args..., ...) volatile> : true_type {};

template <typename Ret, typename... Args>
struct is_function<Ret(Args..., ...) const volatile> : true_type {};

// specialization for function types that have ref-qualifiers
template <typename Ret, typename... Args>
struct is_function<Ret(Args...)&> : true_type {};

template <typename Ret, typename... Args>
struct is_function<Ret(Args...) const&> : true_type {};

template <typename Ret, typename... Args>
struct is_function<Ret(Args...) volatile&> : true_type {};

template <typename Ret, typename... Args>
struct is_function<Ret(Args...) const volatile&> : true_type {};

template <typename Ret, typename... Args>
struct is_function<Ret(Args..., ...)&> : true_type {};

template <typename Ret, typename... Args>
struct is_function<Ret(Args..., ...) const&> : true_type {};

template <typename Ret, typename... Args>
struct is_function<Ret(Args..., ...) volatile&> : true_type {};

template <typename Ret, typename... Args>
struct is_function<Ret(Args..., ...) const volatile&> : true_type {};

template <typename Ret, typename... Args>
struct is_function<Ret(Args...) &&> : true_type {};

template <typename Ret, typename... Args>
struct is_function<Ret(Args...) const&&> : true_type {};

template <typename Ret, typename... Args>
struct is_function<Ret(Args...) volatile&&> : true_type {};

template <typename Ret, typename... Args>
struct is_function<Ret(Args...) const volatile&&> : true_type {};

template <typename Ret, typename... Args>
struct is_function<Ret(Args..., ...) &&> : true_type {};

template <typename Ret, typename... Args>
struct is_function<Ret(Args..., ...) const&&> : true_type {};

template <typename Ret, typename... Args>
struct is_function<Ret(Args..., ...) volatile&&> : true_type {};

template <typename Ret, typename... Args>
struct is_function<Ret(Args..., ...) const volatile&&> : true_type {};

template <typename T>
constexpr bool is_function_v = is_function<T>::value;

template <typename... Ts>
using void_t = void;

template <typename T>
struct remove_extent {
    using type = T;
};

template <typename T>
struct remove_extent<T[]> {
    using type = T;
};

template <typename T, std::size_t N>
struct remove_extent<T[N]> {
    using type = T;
};

template <typename T>
using remove_extent_t = typename remove_extent<T>::type;

// remove top level cv qualifiers
template <typename T>
struct remove_const {
    using type = T;
};

template <typename T>
struct remove_const<const T> {
    using type = T;
};

template <typename T>
using remove_const_t = typename remove_const<T>::type;

template <typename T>
struct remove_volatile {
    using type = T;
};

template <typename T>
struct remove_volatile<volatile T> {
    using type = T;
};

template <typename T>
using remove_volatile_t = typename remove_volatile<T>::type;

template <typename T>
struct remove_cv {
    using type = typename remove_const<typename remove_volatile<T>::type>::type;
};

template <typename T>
using remove_cv_t = typename remove_cv<T>::type;

template <typename T>
struct remove_reference {
    using type = T;
};

template <typename T>
struct remove_reference<T&> {
    using type = T;
};

template <typename T>
struct remove_reference<T&&> {
    using type = T;
};

template <typename T>
using remove_reference_t = typename remove_reference<T>::type;

template <typename T>
constexpr T&& forward(remove_reference_t<T>& param) noexcept;

template <typename T>
constexpr T&& forward(remove_reference_t<T>& param) noexcept;

template <typename T>
struct remove_pointer {};

template <typename T>
struct remove_pointer<T*> {
    using type = T;
};

template <typename T>
using remove_pointer_t = typename remove_pointer<T>::type;

// add top level cv qualifiers
template <typename T>
struct add_const {
    using type = const T;
};

template <typename T>
using add_const_t = typename add_const<T>::type;

template <typename T>
struct add_volatile {
    using type = volatile T;
};

template <typename T>
using add_volatile_t = typename add_volatile<T>::type;

template <typename T>
struct add_cv {
    using type = const volatile T;
};

template <typename T>
using add_cv_t = typename add_cv<T>::type;

namespace {
template <typename T, typename = void>
struct AddReference {
    using Lvalue = T;
    using Rvalue = T;
};

template <typename T>
struct AddReference<T, void_t<T&>> {
    using Lvalue = T&;
    using Rvalue = T&&;
};

template <typename T, bool is_function_type = false>
struct AddPointer {
    using type = typename remove_reference<T>::type*;
};

template <typename T>
struct AddPointer<T, true> {
    using type = T;
};

template <typename T, typename... Args>
struct AddPointer<T(Args...), true> {
    using type = T (*)(Args...);
};

// for function e.g. printf
template <typename T, typename... Args>
struct AddPointer<T(Args..., ...), true> {};

} // namespace

template <typename T>
struct add_lvalue_reference {
    using type = typename AddReference<T>::Lvalue;
};

template <typename T>
using add_lvalue_reference_t = typename add_lvalue_reference<T>::type;

template <typename T>
struct add_rvalue_reference {
    using type = typename AddReference<T>::Rvalue;
};

template <typename T>
using add_rvalue_reference_t = typename add_rvalue_reference<T>::type;

template <typename T>
struct add_pointer : AddPointer<T, is_function<T>::value> {};

template <typename T>
using add_pointer_t = typename add_pointer<T>::type;

template <bool B, typename T = void>
struct enable_if {};

template <typename T>
struct enable_if<true, T> {
    using type = T;
};

template <bool B, typename T = void>
using enable_if_t = typename enable_if<B, T>::type;

// B ? T : F
template <bool B, typename T, typename F>
struct conditional {
    using type = T;
};

template <typename T, typename F>
struct conditional<false, T, F> {
    using type = F;
};

template <bool B, typename T, typename F>
using conditional_t = typename conditional<B, T, F>::type;

// logical AND
template <typename...>
struct conjunction : true_type {};

template <typename B>
struct conjunction<B> : B {};

template <typename B1, typename... Bn>
struct conjunction<B1, Bn...>
    : conditional_t<static_cast<bool>(B1::value), conjunction<Bn...>, B1> {};

template <typename... B>
constexpr bool conjunction_v = conjunction<B...>::value;

// logical OR
template <typename...>
struct disjunction : false_type {};

template <typename B>
struct disjunction<B> : B {};

template <typename B1, typename... Bn>
struct disjunction<B1, Bn...>
    : conditional_t<static_cast<bool>(B1::value), B1, disjunction<Bn...>> {};

template <typename... B>
constexpr bool disjunction_v = disjunction<B...>::value;

template <typename T>
struct is_void : tiny_stl::is_same<void, tiny_stl::remove_cv_t<T>> {};

template <typename T>
constexpr bool is_void_v = is_void<T>::value;

template <typename T>
struct is_array : false_type {};

template <typename T, std::size_t n>
struct is_array<T[n]> : true_type {};

template <typename T>
struct is_array<T[]> : true_type {};

template <typename T>
constexpr bool is_array_v = is_array<T>::value;

template <typename T>
struct is_class : bool_constant<std::is_class<T>::value> {};

template <typename T>
constexpr bool is_class_v = is_class<T>::value;

template <typename T>
struct is_union : bool_constant<std::is_union<T>::value> {};

template <typename T>
constexpr bool is_union_v = is_union<T>::value;

template <typename T>
struct is_enum : bool_constant<std::is_enum<T>::value> {};

template <typename T>
constexpr bool is_enum_v = is_enum<T>::value;

template <typename T>
struct is_final : bool_constant<std::is_final<T>::value> {};

template <typename T>
constexpr bool is_final_v = is_final<T>::value;

template <typename T>
struct IsFloatingPoint : false_type {};

template <>
struct IsFloatingPoint<float> : true_type {};

template <>
struct IsFloatingPoint<double> : true_type {};

template <>
struct IsFloatingPoint<long double> : true_type {};

template <typename T>
struct is_floating_point : IsFloatingPoint<remove_cv_t<T>>::type {};

template <typename T>
constexpr bool is_floating_point_v = is_floating_point<T>::value;

template <typename T>
struct IsIntegral : false_type {};

template <>
struct IsIntegral<bool> : true_type {};

template <>
struct IsIntegral<char> : true_type {};

template <>
struct IsIntegral<unsigned char> : true_type {};

template <>
struct IsIntegral<signed char> : true_type {};

template <>
struct IsIntegral<wchar_t> : true_type {};

template <>
struct IsIntegral<char16_t> : true_type {};

template <>
struct IsIntegral<char32_t> : true_type {};

template <>
struct IsIntegral<short> : true_type {};

template <>
struct IsIntegral<unsigned int> : true_type {};

template <>
struct IsIntegral<int> : true_type {};

template <>
struct IsIntegral<unsigned long> : true_type {};

template <>
struct IsIntegral<long> : true_type {};

template <>
struct IsIntegral<unsigned long long> : true_type {};

template <>
struct IsIntegral<long long> : true_type {};

template <typename T>
struct is_integral : IsIntegral<remove_cv_t<T>>::type {};

template <typename T>
constexpr bool is_integral_v = is_integral<T>::value;

template <typename T, bool = is_integral<T>::value>
struct SignBase {
    using U = remove_cv_t<T>;
    using Signed = bool_constant<U(-1) < U(0)>;
    using Unsiged = bool_constant<U(0) < U(-1)>;
};

template <typename T>
struct SignBase<T, false> {
    using Signed = typename is_floating_point<T>::type;
    using Unsigned = false_type;
};

template <typename T>
struct is_signed : SignBase<T>::SignBase {};

template <typename T>
constexpr bool is_signed_v = is_signed<T>::value;

template <typename T>
struct is_unsigned : SignBase<T>::Unsigned {};

template <typename T>
constexpr bool is_unsigned_v = is_unsigned<T>::value;

template <typename T>
struct ChangeSign {
    static_assert((is_integral<T>::value &&
                   !is_same<bool, remove_cv_t<T>>::value) ||
                      is_enum<T>::value,
                  "make_signed<T> or make_unsigned<T> need integral type or "
                  "emumeration type");

    using Signed = conditional_t<
        is_same<T, signed char>::value || is_same<T, unsigned char>::value,
        signed,
        conditional_t<
            is_same<T, short>::value || is_same<T, unsigned short>::value,
            short,
            conditional_t<
                is_same<T, int>::value || is_same<T, unsigned int>::value, int,
                conditional_t<
                    is_same<T, long>::value || is_same<T, unsigned long>::value,
                    long,
                    conditional_t<
                        is_same<T, long long>::value ||
                            is_same<T, unsigned long long>::value,
                        long long,
                        conditional_t<
                            sizeof(T) == sizeof(signed char), signed char,
                            conditional_t<
                                sizeof(T) == sizeof(short), short,
                                conditional_t<
                                    sizeof(T) == sizeof(int), int,
                                    conditional_t<sizeof(T) == sizeof(long),
                                                  long, long long>>>>>>>>>;

    using Unsigned = conditional_t<
        is_same<Signed, signed char>::value, unsigned char,
        conditional_t<
            is_same<Signed, short>::value, unsigned short,
            conditional_t<is_same<Signed, int>::value, unsigned int,
                          conditional_t<is_same<Signed, long>::value,
                                        unsigned long, unsigned long long>>>>;
};

template <typename T>
struct ChangeSign<const T> {
    using Signed = const typename ChangeSign<T>::Signed;
    using Unsigned = const typename ChangeSign<T>::Unsigned;
};

template <typename T>
struct ChangeSign<volatile T> {
    using Signed = volatile typename ChangeSign<T>::Signed;
    using Unsigned = volatile typename ChangeSign<T>::Unsigned;
};

template <typename T>
struct ChangeSign<const volatile T> {
    using Signed = const volatile typename ChangeSign<T>::Signed;
    using Unsigned = const volatile typename ChangeSign<T>::Unsigned;
};

template <typename T>
struct make_signed {
    using type = typename ChangeSign<T>::Signed;
};

template <typename T>
using make_signed_t = typename make_signed<T>::type;

template <typename T>
struct make_unsigned {
    using type = typename ChangeSign<T>::Unsigned;
};

template <typename T>
using make_unsigned_t = typename make_unsigned<T>::type;

template <typename T>
struct is_pod : bool_constant<std::is_pod<T>::value> {};

template <typename T>
constexpr bool is_pod_v = is_pod<T>::value;

template <typename T>
struct is_empty : bool_constant<std::is_empty<T>::value> {};

template <typename T>
constexpr bool is_empty_v = is_empty<T>::value;

template <typename T>
struct IsCharacter : false_type {};

template <>
struct IsCharacter<char> : true_type {};

template <>
struct IsCharacter<signed char> : true_type {};

template <>
struct IsCharacter<unsigned char> : true_type {};

template <typename T>
struct IsMemberPointer : false_type {};

template <typename T, typename C>
struct IsMemberPointer<T C::*> : true_type {};

template <typename T>
struct is_member_pointer : IsMemberPointer<typename remove_cv<T>::type> {};

template <typename T>
constexpr bool is_member_pointer_v = is_member_pointer<T>::value;

template <typename T>
struct IsPointer : false_type {};

template <typename T>
struct IsPointer<T*> : true_type {};

template <typename T>
struct is_pointer : IsPointer<typename remove_cv<T>::type> {};

template <typename T>
constexpr bool is_pointer_v = is_pointer<T>::value;

template <typename T>
struct is_null_pointer
    : bool_constant<is_same<remove_cv_t<T>, std::nullptr_t>::value> {};

template <typename T>
constexpr bool is_null_pointer_v = is_null_pointer<T>::value;

template <typename T>
struct is_lvalue_reference : false_type {};

template <typename T>
struct is_lvalue_reference<T&> : true_type {};

template <typename T>
constexpr bool is_lvalue_reference_v = is_lvalue_reference<T>::value;

template <typename T>
struct is_rvalue_reference : false_type {};

template <typename T>
struct is_rvalue_reference<T&&> : true_type {};

template <typename T>
constexpr bool is_rvalue_reference_v = is_rvalue_reference<T>::value;

template <typename T>
struct is_reference : bool_constant<is_lvalue_reference<T>::value ||
                                    is_rvalue_reference<T>::value> {};

template <typename T>
constexpr bool is_reference_v = is_reference<T>::value;

template <typename T>
struct is_const : false_type {};

template <typename T>
struct is_const<const T> : true_type {};

template <typename T>
constexpr bool is_const_v = is_const<T>::value;

template <typename T>
struct is_volatile : false_type {};

template <typename T>
struct is_volatile<volatile T> : true_type {};

template <typename T>
constexpr bool is_volatile_v = is_volatile<T>::value;

template <typename T, typename... Args>
struct is_constructible
    : bool_constant<std::is_constructible<T, Args...>::value> {};

template <typename T, typename... Args>
constexpr bool is_constructible_v = is_constructible<T, Args...>::value;

template <typename T>
struct is_default_constructible : is_constructible<T>::type {};

template <typename T>
constexpr bool is_default_constructible_v = is_default_constructible<T>::value;

template <typename T>
struct is_copy_constructible
    : is_constructible<T, add_lvalue_reference_t<const T>>::type {};

template <typename T>
constexpr bool is_copy_constructible_v = is_copy_constructible<T>::value;

template <typename T, typename... Args>
struct is_trivially_constructible
    : bool_constant<std::is_trivially_constructible<T, Args...>::value> {};

template <typename T>
struct is_destructible : bool_constant<std::is_destructible<T>::value> {};

template <typename T>
constexpr bool is_destructible_v = is_destructible<T>::value;

template <typename T>
struct is_trivially_destructible
    : bool_constant<std::is_trivially_destructible<T>::value> {};

template <typename T>
constexpr bool is_trivially_destructible_v =
    is_trivially_destructible<T>::value;

template <typename T, typename... Args>
struct is_nothrow_constructible
    : bool_constant<std::is_nothrow_constructible<T, Args...>::value> {};

template <typename T, typename... Args>
constexpr bool is_nothrow_constructible_v =
    is_nothrow_constructible<T, Args...>::value;

template <typename T>
struct is_move_constructible : is_constructible<T, add_rvalue_reference_t<T>> {
};

template <typename T>
constexpr bool is_move_constructible_v = is_move_constructible<T>::value;

template <typename T>
struct is_trivially_move_constructible
    : is_trivially_constructible<T, add_rvalue_reference_t<T>> {};

template <typename T>
constexpr bool is_trivially_move_constructible_v =
    is_trivially_move_constructible<T>::value;

template <typename T>
struct is_nothrow_move_constructible
    : is_nothrow_constructible<T, add_rvalue_reference_t<T>> {};

template <typename T>
constexpr bool is_nothrow_move_constructible_v =
    is_nothrow_move_constructible<T>::value;

template <typename From, typename To>
struct is_convertible : bool_constant<std::is_convertible<From, To>::value> {};

template <typename From, typename To>
constexpr bool is_convertible_v = is_convertible<From, To>::value;

template <typename To, typename From>
struct is_assignable : bool_constant<std::is_assignable<To, From>::value> {};

template <typename To, typename From>
constexpr bool is_assignable_v = is_assignable<To, From>::value;

template <typename To, typename From>
struct is_trivially_assignable
    : bool_constant<std::is_trivially_assignable<To, From>::value> {};

template <typename To, typename From>
constexpr bool is_trivially_assignable_v =
    is_trivially_assignable<To, From>::value;

template <typename To, typename From>
struct is_nothrow_assignable
    : bool_constant<std::is_nothrow_assignable<To, From>::value> {};

template <typename To, typename From>
constexpr bool is_nothrow_assignable_v = is_nothrow_assignable<To, From>::value;

template <typename T>
struct is_move_assignable
    : is_assignable<add_lvalue_reference_t<T>, add_rvalue_reference_t<T>> {};

template <typename T>
constexpr bool is_move_assignable_v = is_move_assignable<T>::value;

template <typename T>
struct is_trivially_move_assignable
    : is_trivially_assignable<add_lvalue_reference_t<T>,
                              add_rvalue_reference_t<T>> {};

template <typename T>
constexpr bool is_trivially_move_assignable_v =
    is_trivially_move_assignable<T>::value;

template <typename T>
struct is_nothrow_move_assignable
    : is_nothrow_assignable<add_lvalue_reference_t<T>,
                            add_rvalue_reference_t<T>> {};

template <typename T>
constexpr bool is_nothrow_move_assignable_v =
    is_nothrow_move_assignable<T>::value;

template <typename T>
struct is_arithmetic
    : bool_constant<is_integral<T>::value || is_floating_point<T>::value> {};

template <typename T>
constexpr bool is_arithmetic_v = is_arithmetic<T>::value;

template <typename T>
struct is_scalar
    : bool_constant<is_arithmetic<T>::value || is_enum<T>::value ||
                    is_pointer<T>::value || is_member_pointer<T>::value ||
                    is_null_pointer<T>::value> {};

template <typename T>
constexpr bool is_scalar_v = is_scalar<T>::value;

template <typename T>
struct is_object : bool_constant<is_scalar<T>::value || is_array<T>::value ||
                                 is_union<T>::value || is_class<T>::value> {};

template <typename T>
constexpr bool is_object_v = is_object<T>::value;

// lvalue   -> rvalue
// array    -> pointer
// function -> pointer
// remove_cv
template <typename T>
struct decay {
private:
    using U = typename remove_reference<T>::type;

public:
    using type = conditional_t<
        is_array<U>::value, remove_extent_t<U>*,
        conditional_t<is_function<U>::value, add_pointer_t<U>, remove_cv_t<U>>>;
};

template <typename T>
using decay_t = typename decay<T>::type;

template <typename T>
add_rvalue_reference_t<T> declval() noexcept;

template <typename>
struct result_of;

template <typename F, typename... Args>
struct result_of<F(Args...)> {
    using type = decltype(declval<F>()(declval<Args>()...));
};

template <typename T>
using reference_wrapper = std::reference_wrapper<T>;

template <typename T>
struct UnRefWrapHelper {
    using type = T;
    static constexpr bool isRefWrap = false;
};

template <typename T>
struct UnRefWrapHelper<reference_wrapper<T>> {
    using type = T&;
    static constexpr bool isRefWrap = true;
};

template <typename T>
struct UnRefWrap {
    using Decay = decay_t<T>;
    using type = typename UnRefWrapHelper<Decay>::type;
    static constexpr bool isRefWrap = UnRefWrapHelper<T>::isRefWrap;
};

template <typename Traits>
struct negation : bool_constant<!static_cast<bool>(Traits::value)> {};

template <typename Traits>
constexpr bool negation_v = negation<Traits>::value;

// map
template <typename T, bool isMap>
struct AssociatedTypeHelper {
    using key_type = typename T::first_type;
    using mapped_type = typename T::second_type;
};

// set
template <typename T>
struct AssociatedTypeHelper<T, false> {
    using key_type = T;
    using mapped_type = void;
};

template <typename T>
struct is_nothrow_swappable;

template <typename T>
void swap(T& lhs, T& rhs) noexcept(
    is_nothrow_move_constructible_v<T>&& is_nothrow_move_assignable_v<T>);

template <typename T, std::size_t N>
void swap(T (&lhs)[N], T (&rhs)[N]) noexcept(is_nothrow_swappable<T>::value);

template <typename T, typename U, typename = void>
struct SwappableHelper : false_type {};

template <typename T, typename U>
struct SwappableHelper<
    T, U,
    void_t<decltype(swap(tiny_stl::declval<T>(), tiny_stl::declval<U>()))>>
    : true_type {};

template <typename T, typename U>
struct is_swappable_with
    : bool_constant<
          conjunction_v<SwappableHelper<T, U>, SwappableHelper<U, T>>> {};

template <typename T>
struct is_swappable : is_swappable_with<add_lvalue_reference_t<T>,
                                        add_lvalue_reference_t<T>>::type {};

template <typename T, typename U>
struct SwapNoThrow
    : bool_constant<noexcept(swap(
          tiny_stl::declval<T>(),
          tiny_stl::declval<U>()))&& noexcept(swap(tiny_stl::declval<U>(),
                                                   tiny_stl::declval<T>()))> {};

template <typename T, typename U>
struct is_nothrow_swappable_with
    : bool_constant<conjunction_v<is_swappable_with<T, U>, SwapNoThrow<T, U>>> {
};

template <typename T>
struct is_nothrow_swappable
    : is_nothrow_swappable_with<add_lvalue_reference_t<T>,
                                add_lvalue_reference_t<T>>::type {};

template <typename T, typename U>
constexpr bool is_swappable_with_v = is_swappable_with<T, U>::value;

template <typename T>
constexpr bool is_swappable_v = is_swappable<T>::value;

template <typename T, typename U>
constexpr bool is_nothrow_swappable_with_v =
    is_nothrow_swappable_with<T, U>::value;

template <typename T>
constexpr bool is_nothrow_swappable_v = is_nothrow_swappable<T>::value;

} // namespace tiny_stl
