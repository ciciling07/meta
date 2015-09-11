/**
 * @file top_k.cpp
 * @author Sean Massung
 */

#include <iostream>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <string>
#include "cpptoml.h"
#include "corpus/corpus.h"
#include "analyzers/analyzer.h"
#include "analyzers/filters/all.h"
#include "util/progress.h"
#include "util/fixed_heap.h"
#include "logging/logger.h"

using namespace meta;

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << " config.toml k" << std::endl;
        std::cerr << "Prints out the top k most frequent terms in the corpus "
                     "according to the filter chain specified in the config "
                     "file." << std::endl;
        return 1;
    }

    uint64_t k = std::stoi(argv[2]);

    logging::set_cerr_logging();

    auto config = cpptoml::parse_file(argv[1]);
    auto group = config.get_table_array("analyzers");
    if (!group)
        throw std::runtime_error{"[[analyzers]] missing from config"};

    // only use the feature representation of the first analyzer
    auto filts = analyzers::load_filters(config, *(group->get()[0]));

    std::unordered_map<std::string, uint64_t> counts;
    auto docs = corpus::corpus::load(config);
    printing::progress prog{" > Reading corpus: ", docs->size()};
    while (docs->has_next())
    {
        auto doc = docs->next();
        prog(doc.id());
        auto content = doc.content();
        filts->set_content(std::move(content));
        while (*filts)
            ++counts[filts->next()];
    }
    prog.end();

    using pair_t = std::pair<std::string, uint64_t>;
    auto comp = [](const pair_t& a, const pair_t& b)
    {
        return a.second > b.second;
    };
    util::fixed_heap<pair_t, decltype(comp)> terms{k, comp};
    for (auto& term : counts)
        terms.emplace(term);

    auto sorted = terms.reverse_and_clear();
    for (const auto& it : sorted)
        std::cout << it.first << "\t" << it.second << std::endl;
}