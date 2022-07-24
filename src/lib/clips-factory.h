#pragma once
#include <string>
#include <stdexcept>
#include <unordered_map>

#include "lib/clips-utils.h"

class ClipsFactory {
   public:
    ClipsFactory(std::string rules);
    void *Create();
    void Destroy(void *clips);

   private:
    void *createClipsEnvFromRuleString();

    std::string _rules;
};