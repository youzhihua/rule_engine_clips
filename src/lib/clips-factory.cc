#include "lib/clips-factory.h"

ClipsFactory::ClipsFactory(std::string rules) : _rules(std::move(rules)) {
}

void ClipsFactory::Destroy(void *clips) {
    if (clips) {
        DestroyEnvironment(clips);
    }
}

void *ClipsFactory::Create() {
    return createClipsEnvFromRuleString();
}

void *ClipsFactory::createClipsEnvFromRuleString() {
    clips_ptr clips = CreateClips(_rules);
    return clips.release();
}