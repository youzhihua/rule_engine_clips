#pragma once

#include <string>
#include <ostream>
#include <vector>
#include "lib/json.hpp"
#include <memory>

#define OBJECT_SYSTEM 0 // 关闭 clips 面向对象功能，节省内存
#define EXTENDED_MATH_FUNCTIONS 0 // 关闭扩展数学函数（cos/sin etc.）
#define DEFGLOBAL_CONSTRUCT 0 // 关闭 defglobal
#define DEFGENERIC_CONSTRUCT 0 // 关闭 defgeneric
#define DEBUGGING_FUNCTIONS 0 // 关闭调试命令：agenda, facts, ppdefrule, ppdeffacts, etc
#define CONSTRUCT_COMPILER 0 // 关闭编译成 c 结构的功能，涉及命令： constructs-to-c
#include "clips/clips.h"

struct ClipsDestructor {
    void operator()(void *clips_env) { DestroyEnvironment(clips_env); }
};

using clips_ptr = std::unique_ptr<void, ClipsDestructor>;

// Clips GC's scope locker.
class ClipsGCLock {
   public:
    ClipsGCLock(void *clips) : _clips(clips) { EnvIncrementGCLocks(_clips); }

    ~ClipsGCLock() { EnvDecrementGCLocks(_clips); }

    ClipsGCLock(const ClipsGCLock &) = delete;
    ClipsGCLock(const ClipsGCLock &&) = delete;
    ClipsGCLock &operator=(const ClipsGCLock &) = delete;

   private:
    void *_clips;
};

clips_ptr CreateClips(const std::string &rules);

int ClipsEnvLoadFromString(void *clips_env, const std::string &constructs);

void ClipsCreateFacts(void* clips, const nlohmann::json &features);

nlohmann::json ClipsExecute(void *clips, const nlohmann::json &features,
                            int max_iters, const std::string &result_func,
                            int &halt);

nlohmann::json ClipsModuleExecute(void *clips, const nlohmann::json &features,
                                  int max_iters, const std::string &result_func,
                                  int &halt);