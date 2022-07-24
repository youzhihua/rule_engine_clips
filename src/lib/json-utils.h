#pragma once

#include <stdexcept>
#include <string>
#include <vector>
#include "lib/json.hpp"
#include "lib/traits-helper.h"


// Get the member of jobj if exists, member should be a primitive type.
template <typename T>
inline T GetOrElse(const nlohmann::json &obj, const std::string &member,
                   T &&candidate) {
    auto it = obj.find(member);
    if (it == obj.end()) {
        return candidate;
    } else {
        try {
            return (*it).get<T>();
        } catch (std::exception &e) {
            return candidate;
        }
    }
}

template <typename T>
inline T GetOrElse(const nlohmann::json &obj, const std::string &member,
                   T &candidate) {
    auto it = obj.find(member);
    if (it == obj.end()) {
        return candidate;
    } else {
        try {
            return (*it).get<T>();
        } catch (std::exception &e) {
            return candidate;
        }
    }
}

// Flatten a json object into one level, ignore members start with "__"
void Flatten(nlohmann::json &result, const std::string &prefix,
             const nlohmann::json &obj, const std::string &dot = ".");

void Merge(nlohmann::json &result, const nlohmann::json &obj);
void Merge(nlohmann::json &result, nlohmann::json &&obj);

void AppendMerge(nlohmann::json &result, const nlohmann::json &obj);
void AppendMerge(nlohmann::json &result, nlohmann::json &&obj);

// Encode a json path into a string, throws runtime_error if @param path is
// illegal.
std::string EncodeJPath(const nlohmann::json &path);

// Decode a json path string into json path, throws runtime_error if @param
// path_str is illegal.
nlohmann::json DecodeJPath(const std::string &path_str);

void CheckJPath(const nlohmann::json &path);

// Get the value at the @param path in @param root. Return null if the path does
// not exist in
// @param root. throws runtime_error if @param path is illegal.
const nlohmann::json *GetJSON(const nlohmann::json &root,
                              const nlohmann::json &path);
nlohmann::json *GetJSON(nlohmann::json &root, const nlohmann::json &path);

const nlohmann::json *GetJSON(const nlohmann::json &root,
                              const std::string &path);
nlohmann::json *GetJSON(nlohmann::json &root, const std::string &path);
std::string GetJsonString(const nlohmann::json &root, const std::string& path); 

inline const nlohmann::json *GetJSON(const nlohmann::json &root,
                                     const char *path) {
    return GetJSON(root, std::string(path));
}
inline nlohmann::json *GetJSON(nlohmann::json &root, const char *path) {
    return GetJSON(root, std::string(path));
}

// Check if this is a json path set.
bool IsJPathSet(const nlohmann::json &path_set);

// Check if path_set is a legal json path set.
void CheckJPathSet(const nlohmann::json &path_set);

// Encode json path set into a unique string.
std::string EncodeJPathSet(const nlohmann::json &path_set);

// Encode keys
std::string EncodeKeySet(const nlohmann::json &keys);

void ErasePathSet(nlohmann::json *features, nlohmann::json *path_set);

template <typename valueT>
std::vector<valueT> GetJVector(nlohmann::json *features,
                               const std::string &key) {
    std::vector<valueT> result;
    try {
        auto node = GetJSON(*features, DecodeJPath(key));
        if (node == nullptr || !node->is_array()) {
            return result;
        }
        for (auto &item : *node) {
            try {
                result.push_back(item.get<valueT>());
            } catch (...) {
                // ignore
            }
        }
    } catch (...) {
    }
    return result;
}

template <typename valueT>
valueT GetJValue(nlohmann::json *features, const std::string &key) {
    valueT result = TraitsHelper<valueT>::field_value();
    try {
        auto node = GetJSON(*features, DecodeJPath(key));
        if (node == nullptr) {
            return result;
        }
        return node->get<valueT>();
    } catch (...) {
    }
    return result;
}

template <typename valueT>
void SetJValue(nlohmann::json *features, const std::string &key,
               valueT &&value) {
    try {
        auto path = DecodeJPath(key);
        if (path.is_null() || !path.is_array() || path.empty()) {
            return;
        }

        nlohmann::json *root = features;
        auto i = 0u;
        for (; i < path.size() - 1; i++) {
            if (root->find(path[i].get<std::string>()) == root->end()) {
                (*root)[path[i].get<std::string>()] = nlohmann::json::object();
            }
            root = &((*root)[path[i].get<std::string>()]);
        }
        (*root)[path[i].get<std::string>()] = move(value);
    } catch (...) {
    }
}

void SetJObject(nlohmann::json *features, const std::string &key);

bool JArrayContains(nlohmann::json &array, const std::string &key);

void CopyJSON(nlohmann::json *features, const std::string &from,
              const std::string &to);