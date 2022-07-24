#include "lib/json-utils.h"
#include <algorithm>
#include <stdexcept>
#include <string>
#include <sstream>
#include <vector>

using std::invalid_argument;
using std::move;
using std::runtime_error;
using std::sort;
using std::string;
using std::stringstream;
using std::vector;
using json = nlohmann::json;
namespace {

// Implements json path parser
void ParseIdentifier(const string &path, size_t current_index,
                     const char **identifier, size_t *length) {
    *identifier = &path[current_index];
    size_t next = current_index;
    while (path[next] != '.' && path[next] != '[' && next < path.size()) ++next;
    *length = next - current_index;
}
void ParseDotIdentifier(const string &path, size_t current_index,
                     const char **identifier, size_t *length) {
    *identifier = &path[current_index];
    size_t next = current_index;
    while (!(path[next] == '\'' && path[next+1] == ']') && next < path.size() - 1) ++next;
    *length = next - current_index;
}

void ParseIndex(const string &path, size_t current_index, size_t *result,
                size_t *length) {
    *result = 0;
    size_t next = current_index;
    while (path[next] != ']' && next < path.size()) {
        if (isdigit(path[next])) {
            *result = *result * 10 + path[next] - '0';
        } else {
            stringstream ss;
            ss << "unexpected char '" << path[next] << "' at " << next;
            throw runtime_error(ss.str());
        }
        ++next;
    }
    *length = next - current_index;
}

void ParseElement(const string &path, size_t current_index, json *output) {
    while (current_index < path.size()) {
        switch (path[current_index]) {
            case '.':
                current_index += 1;
                const char *identifier;
                size_t identifier_length;
                ParseIdentifier(path, current_index, &identifier,
                                &identifier_length);
                if (identifier_length == 0) {
                    stringstream ss;
                    ss << "empty identifier at " << current_index;
                    throw runtime_error(ss.str());
                }
                output->push_back(string(identifier, identifier_length));
                current_index += identifier_length;
                break;

            case '[':
                current_index += 1;
                if (path[current_index] == '\'') {
                    current_index += 1;
                    const char *identifier;
                    size_t identifier_length;
                    ParseDotIdentifier(path, current_index, &identifier,
                                    &identifier_length);
                    if (identifier_length == 0) {
                        stringstream ss;
                        ss << "empty identifier at " << current_index;
                        throw runtime_error(ss.str());
                    }
                    output->push_back(string(identifier, identifier_length));
                    current_index += identifier_length;
                    current_index +=2;
                } else {
                    size_t index;
                    size_t index_length;
                    ParseIndex(path, current_index, &index, &index_length);
                    output->push_back(index);
                    current_index += index_length;
                    if (current_index >= path.size() ||
                        path[current_index] != ']') {
                        stringstream ss;
                        ss << "expect ']' at " << current_index;
                        throw runtime_error(ss.str());
                    }
                    current_index += 1;
                }

            break;

            default: {
                stringstream ss;
                ss << "unexpected char '" << path[current_index] << "' at "
                   << current_index;
                throw runtime_error(ss.str());
            }
        }
    }
}

}  // anonymous namespace


void Flatten(json &result, const string &prefix, const json &obj,
             const string &dot) {
    string prefix_with_dot;
    if (!prefix.empty()) {
        prefix_with_dot = prefix + dot;
    }

    for (auto it = obj.begin(); it != obj.end(); ++it) {
        if (it.key().find("__") == 0) continue;
        if (it.key() == "variable_key_list") continue;
        if ((*it).is_object()) {
            Flatten(result, prefix_with_dot + it.key(), (*it), dot);
        } else {
            result[prefix_with_dot + it.key()] = (*it);
        }
    }
}

void Merge(json &result, const json &obj) {
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        result[it.key()] = (*it);
    }
}

void Merge(json &result, json &&obj) {
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        result[it.key()] = move(*it);
    }
}

void AppendMerge(json &result, const json &obj) {
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        auto &merge = result[it.key()];
        if (merge.is_object()) {
            AppendMerge(merge, *it);
        } else {
            result[it.key()] = (*it);
        }
    }
}

void AppendMerge(json &result, json &&obj) {
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        auto &merge = result[it.key()];
        if (merge.is_object()) {
            AppendMerge(merge, *it);
        } else {
            result[it.key()] = (*it);
        }
    }
}

json DecodeJPath(const string &path) {
    json result(json::value_t::array);
    if (path.empty()) {
        throw runtime_error("path ends even before root '$' sign.");
    }
    if (path[0] != '$') {
        stringstream ss;
        ss << "unexpected char '" << path[0] << "' at 0";
        throw runtime_error(ss.str());
    }
    ParseElement(path, 1, &result);
    return result;
}

string EncodeJPath(const json &path) {
    if (!path.is_array()) {
        throw runtime_error("json path is not an array");
    }

    stringstream ss;
    ss << "$";
    for (unsigned int i = 0; i < path.size(); ++i) {
        if (path[i].is_string()) {
            ss << "." << path[i].get<string>();
        } else if (path[i].is_number_integer()) {
            ss << "[" << path[i].get<int>() << "]";
        } else {
            stringstream error;
            error << "json path contains illegal element at position " << i;
            throw runtime_error(error.str());
        }
    }
    return ss.str();
}

void CheckJPath(const json &path) {
    if (!path.is_array()) {
        throw runtime_error("json path is not an array");
    }
    for (size_t i = 0; i < path.size(); ++i) {
        if (!(path[i].is_string() || path[i].is_number_integer())) {
            stringstream error;
            error << "json path contains illegal element at position " << i;
            throw runtime_error(error.str());
        }
    }
}

const json *GetJSON(const json &root, const json &path) {
    return GetJSON(const_cast<json &>(root), path);
}

json *GetJSON(json &root, const json &path) {
    if (!path.is_array()) {
        throw runtime_error("json path is not an array");
    }

    json *next = &root;
    for (size_t i = 0; i < path.size(); ++i) {
        const json &element = path[i];
        if (element.is_string()) {
            if (!next->is_object()) return nullptr;
            auto it = next->find(element);
            if (it == next->end()) return nullptr;
            next = &(*it);
        } else if (element.is_number_integer()) {
            if (!next->is_array()) return nullptr;
            int index = element.get<int>();
            if (index < 0 || index >= next->size()) return nullptr;
            next = &((*next)[index]);
        } else {
            stringstream error;
            error << "json path contains illegal element at position " << i;
            throw runtime_error(error.str());
        }
    }

    return next;
}

const json *GetJSON(const json &root, const string &path) {
    return GetJSON(root, DecodeJPath(path));
}

json *GetJSON(json &root, const string &path) {
    return GetJSON(root, DecodeJPath(path));
}

string GetJsonString(const json &root, const string &path) {
    const json *j = GetJSON(root, path);
    if (j != nullptr && !j->is_null()) {
        return j->dump();
    }

    return "";
}

bool IsJPathSet(const json &path_set) {
    if (!path_set.is_array() || path_set.size() == 0) {
        return false;
    };

    for (size_t i = 0; i < path_set.size(); ++i) {
        if (!path_set[i].is_array()) {
            return false;
        }
    }
    return true;
}

void CheckJPathSet(const json &path_set) {
    if (!path_set.is_array())
        throw runtime_error("json path set is not an array");
    for (size_t i = 0; i < path_set.size(); ++i) {
        CheckJPath(path_set[i]);
    }
}

string EncodeJPathSet(const json &path_set) {
    if (path_set.size() == 1) {
        return EncodeJPath(path_set[0]);
    }

    vector<string> paths;
    for (size_t i = 0; i < path_set.size(); ++i) {
        paths.push_back(EncodeJPath(path_set[i]));
    }
    sort(paths.begin(), paths.end());

    stringstream ss;
    for (size_t i = 0; i < paths.size(); ++i) {
        if (i != 0) {
            ss << "-";
        }
        ss << paths[i];
    }
    return ss.str();
}

string EncodeKeySet(const json &keys) {
    if (!keys.is_array()) {
        throw invalid_argument("param keys is not an array");
    }
    vector<string> keys_copy;
    for (const auto &key : keys) {
        if (key.is_string()) {
            keys_copy.push_back(key.get<string>());
        } else {
            throw invalid_argument("key must be a string");
        }
    }
    sort(keys_copy.begin(), keys_copy.end());
    stringstream ss;
    for (size_t i = 0; i < keys_copy.size(); ++i) {
        if (i != 0) {
            ss << "-";
        }
        ss << keys_copy[i];
    }
    return ss.str();
}

void ErasePathSet(json *features, json *path_set) {
    if (!features->is_object()) {
        return;
    }

    if (path_set->is_null() || !path_set->is_array() || path_set->empty()) {
        return;
    }

    for (auto path_route : *path_set) {
        auto fea = features;
        auto index = 1u;
        for (auto path : path_route) {
            if (!fea->is_object() || fea->is_null()) {
                break;
            }
            if (!path.is_string()) {
                break;
            }
            if (index == path_route.size()) {
                fea->erase(path.get<string>());
                break;
            }
            if (fea->find(path) == fea->end()) {
                break;
            }
            fea = &(*fea)[path.get<string>()];
            index++;
        }
    }
}

void SetJObject(nlohmann::json *features, const std::string &key) {
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
        (*root)[path[i].get<std::string>()] = nlohmann::json::object();
    } catch (...) {
    }
}

bool JArrayContains(nlohmann::json &array, const std::string &key) {
    for (auto it = array.begin(); it != array.end(); it++) {
        if (*it == key) {
            return true;
        }
    }
    return false;
}

void CopyJSON(nlohmann::json *features, const std::string &from,
              const std::string &to) {
    try {
        auto from_node = GetJSON(*features, from);
        if (from_node == nullptr) {
            return;
        }
        auto to_path = DecodeJPath(to);
        if (to_path.is_null() || !to_path.is_array() || to_path.empty()) {
            return;
        }
        nlohmann::json *root = features;
        for (auto i = 0u; i < to_path.size() - 1; i++) {
            auto path_key = to_path[i].get<std::string>();
            if (root->find(path_key) == root->end() ||
                !(*root)[path_key].is_object()) {
                (*root)[path_key] = nlohmann::json::object();
            }
            root = &((*root)[path_key]);
        }
        (*root)[to_path[to_path.size() - 1].get<std::string>()] = *from_node;
    } catch (...) {
    }
}
