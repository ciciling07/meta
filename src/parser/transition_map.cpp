/**
 * @file transition_map.h
 * @author Chase Geigle
 */

#include <cassert>
#include <fstream>

#include "io/filesystem.h"
#include "io/packed.h"
#include "parser/transition_map.h"

#ifdef META_HAS_ZLIB
#include "io/gzstream.h"
#endif

namespace meta
{
namespace parser
{

transition_map::transition_map(const std::string& prefix)
{
#ifdef META_HAS_ZLIB
    if (filesystem::file_exists(prefix + "/parser.trans.gz"))
    {
        io::gzifstream store{prefix + "/parser.trans.gz"};
        load(store);
        return;
    }
#endif
    std::ifstream store{prefix + "/parser.trans", std::ios::binary};
    load(store);
}

void transition_map::load(std::istream& store)
{
    if (!store)
        throw transition_map_exception{"missing transitions model file"};

    uint64_t num_trans;
    io::packed::read(store, num_trans);

    if (!store)
        throw transition_map_exception{"malformed transitions model file"};

    transitions_.reserve(num_trans);
    for (uint64_t i = 0; i < num_trans; ++i)
    {
        if (!store)
            throw transition_map_exception{
                "malformed transition model file (too few "
                "transitions written)"};

        transition::type_t trans_type;
        io::packed::read(store, trans_type);

        util::optional<transition> trans;
        switch (trans_type)
        {
            case transition::type_t::REDUCE_L:
            case transition::type_t::REDUCE_R:
            case transition::type_t::UNARY:
            {
                std::string lbl;
                io::packed::read(store, lbl);
                trans = transition{trans_type, class_label{lbl}};
                break;
            }

            default:
                trans = transition{trans_type};
                break;
        }

        auto id = static_cast<trans_id>(map_.size());
        map_[*trans] = id;
        transitions_.emplace_back(std::move(*trans));
    }
}

const transition& transition_map::at(trans_id id) const
{
    return transitions_.at(id);
}

trans_id transition_map::at(const transition& trans) const
{
    auto it = map_.find(trans);
    if (it == map_.end())
        throw std::out_of_range{"index out of bounds"};

    return it->second;
}

trans_id transition_map::operator[](const transition& trans)
{
    auto it = map_.find(trans);
    if (it != map_.end())
        return it->second;

    transitions_.push_back(trans);
    auto id = static_cast<trans_id>(map_.size());
    return map_[trans] = id;
}

uint64_t transition_map::size() const
{
    assert(map_.size() == transitions_.size());
    return map_.size();
}

void transition_map::save(const std::string& prefix) const
{
#ifdef META_HAS_ZLIB
    io::gzofstream store{prefix + "/parser.trans.gz"};
#else
    std::ofstream store{prefix + "/parser.trans", std::ios::binary};
#endif

    io::packed::write(store, transitions_.size());
    for (const auto& trans : transitions_)
    {
        io::packed::write(store, trans.type());
        switch (trans.type())
        {
            case transition::type_t::REDUCE_L:
            case transition::type_t::REDUCE_R:
            case transition::type_t::UNARY:
                io::packed::write(
                    store, static_cast<const std::string&>(trans.label()));
                break;

            default:
                break;
        }
    }
}
}
}
