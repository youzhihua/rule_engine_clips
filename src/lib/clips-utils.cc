#include <iostream>
#include <stdexcept>
#include <sstream>

#include "lib/clips-utils.h"
#include "lib/json-utils.h"
#include "clips/proflfun.h"

using std::runtime_error;
using std::invalid_argument;
using std::ostream;
using std::string;
using std::stringstream;
using json = nlohmann::json;

namespace {
bool IsSymbol(const string &text) {
    for (auto c : text) {
        if (c >= 'a' && c <= 'z') continue;
        if (c >= 'A' && c <= 'Z') continue;
        if (c >= '0' && c <= '9') continue;
        if (c == '.' || c == '_' || c == '-') continue;
        return false;
    }

    return true;
}

// @see IO Routers in CLIPS.
int FindGLogRouter(void *env, const char *logical_name) {
    if (logical_name == nullptr) return FALSE;
    if (strcmp(logical_name, WWARNING) == 0) return true;
    if (strcmp(logical_name, WERROR) == 0) return true;
    if (strcmp(logical_name, WTRACE) == 0) return true;
    if (strcmp(logical_name, WDIALOG) == 0) return true;
    if (strcmp(logical_name, WPROMPT) == 0) return true;
    if (strcmp(logical_name, WDISPLAY) == 0) return true;

    return FALSE;
}

// @see IO Routers in CLIPS
int GLogPrintLog(void *env, const char *logical_name, const char *log) {
    if (strcmp(logical_name, WWARNING) == 0) {
        std::cerr << "clips: " << log;
    } else if (strcmp(logical_name, WERROR) == 0) {
        std::cerr << "clips: " << log;
    } else {
        std::cout << "clips: " << log;
    }

    return 0;
}

json ExtractDataObject(void *clips, DATA_OBJECT_PTR dobject);
json ExtractField(void *clips, FIELD_PTR field);

json ExtractFactValue(void *clips, void *faddr) {
    DATA_OBJECT slot_names;
    EnvFactSlotNames(clips, faddr, &slot_names);
    void *slot_names_value = EnvGetValue(clips, slot_names);
    if (!EnvGetMFLength(clips, slot_names_value)) {
        throw runtime_error("fact has no slots");
    }

    json result;
    DATA_OBJECT slot_value;
    for (long i = EnvGetDOBegin(clips, slot_names);
         i <= EnvGetDOEnd(clips, slot_names); ++i) {
        const char *name =
            EnvValueToString(theEnv, GetMFValue(slot_names_value, i));
        EnvGetFactSlot(clips, faddr, name, &slot_value);
        auto slot_result = ExtractDataObject(clips, &slot_value);
        if (!slot_result.is_null()) {
            result[name] = move(slot_result);
        }
    }

    return result;
}

json ExtractFact(void *clips, DATA_OBJECT_PTR fact) {
    if (EnvGetpType(clips, fact) != FACT_ADDRESS) {
        stringstream msg;
        msg << "param is not a fact: " << EnvGetpType(clips, fact);
        throw runtime_error(msg.str());
    }
    void *faddr = EnvGetpValue(clips, fact);

    return ExtractFactValue(clips, faddr);
}

json ExtractMultifield(void *clips, DATA_OBJECT_PTR multifield) {
    if (EnvGetpType(clips, multifield) != MULTIFIELD) {
        stringstream msg;
        msg << "param is not a multifield: " << EnvGetpType(clips, multifield);
        throw runtime_error(msg.str());
    }

    json result(json::value_t::array);
    void *value = EnvGetpValue(clips, multifield);
    auto begin = EnvGetpDOBegin(clips, multifield);
    auto end = EnvGetpDOEnd(clips, multifield);
    for (auto i = begin; i <= end; ++i) {
        result.push_back(ExtractField(clips, EnvGetMFPtr(clips, value, i)));
    }
    return result;
}

json ExtractField(void *clips, FIELD_PTR field) {
    switch (field->type) {
        case STRING: {
            return json(EnvValueToString(clips, field->value));
        }

        case SYMBOL: {
            const char *symbol = EnvValueToString(clips, field->value);
            if (strcmp("TRUE", symbol) == 0) {
                return json(true);
            } else if (strcmp("FALSE", symbol) == 0) {
                return json(false);
            } else if (strcmp("nil", symbol) == 0) {
                return json(json::value_t::null);
            } else {
                // treat symbol as string
                return json(symbol);
            }
        }

        case FLOAT: {
            return json(EnvValueToDouble(clips, field->value));
        }

        case INTEGER: {
            return json(EnvValueToInteger(clips, field->value));
        }

        case FACT_ADDRESS: {
            return ExtractFactValue(clips, field->value);
        }

        default: {
            stringstream msg;
            msg << "unsupportted data type: " << field->type;
            throw runtime_error(msg.str());
        }
    }
}

json ExtractDataObject(void *clips, DATA_OBJECT_PTR dobject) {
    switch (EnvGetpType(clips, dobject)) {
        case STRING: {
            return json(EnvDOPToString(clips, dobject));
        }

        case SYMBOL: {
            const char *symbol = EnvDOPToString(clips, dobject);
            if (strcmp("TRUE", symbol) == 0) {
                return json(true);
            } else if (strcmp("FALSE", symbol) == 0) {
                return json(false);
            } else if (strcmp("nil", symbol) == 0) {
                return json(json::value_t::null);
            } else {
                // treat symbol as string
                return json(symbol);
            }
        }

        case FLOAT: {
            return json(EnvDOPToDouble(clips, dobject));
        }

        case INTEGER: {
            return json(EnvDOPToInteger(clips, dobject));
        }

        case MULTIFIELD: {
            return ExtractMultifield(clips, dobject);
        }

        case FACT_ADDRESS: {
            return ExtractFact(clips, dobject);
        }

        default: {
            stringstream msg;
            msg << "unsupportted data type: " << EnvGetpType(clips, dobject);
            throw runtime_error(msg.str());
        }
    }
}

json ExtractResult(void *clips, DATA_OBJECT_PTR result_object) {
    ClipsGCLock clips_gclock(clips);
    return ExtractDataObject(clips, result_object);
}

inline void ClipsCreatePrimitive(const json &obj, ostream &os) {
    if (obj.is_number_integer()) {
        os << obj.get<int64_t>();
    } else if (obj.is_number_float()) {
        os << obj.get<double>();
    } else if (obj.is_string()) {
        os << obj.dump();
    } else if (obj.is_boolean()) {
        if (obj.get<bool>()) {
            os << "TRUE";
        } else {
            os << "FALSE";
        }
    } else {
        ;  // ignore orthers
    }
}

}  // anonymous namespace


int preHandleExec(void *clips, const json &features);
void endHandleExec(void *clips, int handle_type);

int ClipsEnvLoadFromString(void *clips_env, const string &constructs) {
    const char *str_router = "*** load-constructs-from-string ***";
    int retcode;
    retcode = OpenStringSource(clips_env, str_router, constructs.c_str(), 0);
    if (!retcode) {
        return retcode;
    }

    retcode = LoadConstructsFromLogicalName(clips_env, str_router);
    CloseStringSource(clips_env, str_router);

    return retcode;
}

clips_ptr CreateClips(const string &rules) {
    clips_ptr clips(CreateEnvironment());
    if (!clips) {
        throw runtime_error("[FATAL] clips CreateEnvironment() failed");
    }
    EnvSetDynamicConstraintChecking(clips.get(), TRUE);

    int retcode;

    retcode = ClipsEnvLoadFromString(clips.get(), rules);
    if (retcode != 1) {
        throw runtime_error("[FATAL] clips EvnLoadFromString() failed, " +
                            rules);
    }

    return clips;
}

void ClipsCreateFacts(void* clips, const json &features) {
    if (!features.is_object()) {
        throw invalid_argument("'features' must be a json object");
    }
    for (auto iter = features.begin(); iter != features.end(); ++iter) {
        if (!IsSymbol(iter.key())) {
            continue;
        }

        stringstream facts;
        if (iter.value().is_primitive()) {
            facts << "(" << iter.key() << " ";
            ClipsCreatePrimitive(iter.value(), facts);
            facts << ")\n";
        } else if (iter.value().is_array()) {
            facts << "(" << iter.key();
            auto &values = iter.value();
            for (auto i = 0u; i < values.size() && values[i].is_primitive();
                 ++i) {
                facts << " ";
                ClipsCreatePrimitive(values[i], facts);
            }
            facts << ")";
        } else {
            continue;  // ignore others
        }
        EnvLoadFactsFromString(clips, facts.str().c_str(), -1);
    }
}

json ClipsExecute(void *clips, const json &features, int max_iters,
                  const string &result_func, int &halt) {
    // Construct facts

    // Trigger clips rule engine
    EnvReset(clips);
    ClipsCreateFacts(clips, features);
    EnvRun(clips, max_iters);

    halt = EvaluationData(clips)->HaltExecution;

    // Get result
    DATA_OBJECT result;
    int retcode = EnvFunctionCall(clips, result_func.c_str(), nullptr, &result);
    if (retcode) {
        throw runtime_error("clips failed to call " + result_func);
    }
    json match_result = ExtractResult(clips, &result);

    return match_result;
}

json ClipsModuleExecute(void *clips, const json &features, int max_iters,
                        const string &result_func,
                        int &halt) {
    // Trigger clips rule engine
    EnvReset(clips);

    ClipsCreateFacts(clips, features);
    EnvRun(clips, max_iters);

    halt = EvaluationData(clips)->HaltExecution;

    // Get result
    DATA_OBJECT result;
    int retcode = EnvFunctionCall(clips, result_func.c_str(), nullptr, &result);
    if (retcode) {
        throw runtime_error("clips failed to call " + result_func);
    }
    json match_result = ExtractResult(clips, &result);

    return match_result;
}
