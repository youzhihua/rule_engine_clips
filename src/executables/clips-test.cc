#include <iostream>
#include <string>
#include "memory"
#include "lib/json-utils.h"
#include "lib/clips-utils.h"
#include "lib/clips-factory.h"
#include "lib/resource-pool.hpp"

using nlohmann::json;
using Resource = ResourcePool<void, ClipsFactory>;

int main(int argc, char **argv) {
    auto rule = "(deftemplate hit_result\n"
                "  (slot model)\n"
                "  (slot score)\n"
                "  (slot riskLevel))\n"
                "  \n"
                "\n"
                "(defrule M1000\n"
                "   (list.score ?score)\n"
                "   (test (>= ?score 200))\n"
                "=>\n"
                "  (assert (hit_result\n"
                "            (model \"M1000\")\n"
                "            (score 50)\n"
                "            (riskLevel \"PASS\"))))\n"
                "  \n"
                "\n"
                "(deffunction get-result ()\n"
                "  (nth 1 (find-fact ((?fact hit_result)) TRUE)))";

    auto result_func_ = "get-result";

    int halt = 0;
    auto factory = std::make_unique<ClipsFactory>(rule);
    auto resource = std::make_shared<Resource>(10 , factory.release());
    resource->set_max_capacity(10);
    resource->set_need_clear(false);
    auto flatten = json(json::value_t::object);
    flatten["list.score"] = 400;
    auto res = resource->RunWithResource<json>([&](void *env) -> json {
        return ClipsModuleExecute(env, flatten, 10000,result_func_, halt);
    });
    // {"model":"M1000","riskLevel":"PASS","score":50}
    std::cout << res.dump() << std::endl;

    // null
    flatten["list.score"] = 0;
    res = resource->RunWithResource<json>([&](void *env) -> json {
        return ClipsModuleExecute(env, flatten, 10000,result_func_, halt);
    });
    std::cout << res.dump() << std::endl;
}
