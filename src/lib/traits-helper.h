#pragma once

#include <string>

enum TraitsType {
    T_UNKNOWN = 0,
    T_INT32,
    T_UINT32,
    T_INT64,
    T_UINT64,
    T_BOOL,
    T_FLOAT,
    T_DOUBLE,
    T_STRING
};

template <typename T>
struct TraitsHelper {
    static const TraitsType field_type = T_UNKNOWN;
};

template <>
struct TraitsHelper<std::string> {
    static const TraitsType field_type = T_STRING;
    static std::string field_value() { return ""; }
};

template <>
struct TraitsHelper<int32_t> {
    static const TraitsType field_type = T_INT32;
    static const int32_t field_value() { return 0; }
};

template <>
struct TraitsHelper<uint32_t> {
    static const TraitsType field_type = T_UINT32;
    static const uint32_t field_value() { return 0; }
};

template <>
struct TraitsHelper<int64_t> {
    static const TraitsType field_type = T_INT64;
    static const int64_t field_value() { return 0; }
};

template <>
struct TraitsHelper<uint64_t> {
    static const TraitsType field_type = T_UINT64;
    static const uint64_t field_value() { return 0; }
};

template <>
struct TraitsHelper<bool> {
    static const TraitsType field_type = T_BOOL;
    static const bool field_value() { return false; }
};

template <>
struct TraitsHelper<float> {
    static const TraitsType field_type = T_FLOAT;
    static constexpr float field_value() { return 0; }
};

template <>
struct TraitsHelper<double> {
    static const TraitsType field_type = T_DOUBLE;
    static constexpr double field_value() { return 0; }
};