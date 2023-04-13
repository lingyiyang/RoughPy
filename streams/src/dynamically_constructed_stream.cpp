// Copyright (c) 2023 Datasig Group. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its contributors
// may be used to endorse or promote products derived from this software without
// specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
// USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

//
// Created by user on 18/03/23.
//

#include "dynamically_constructed_stream.h"

#include <roughpy/algebra/lie.h>

using namespace rpy;
using namespace rpy::streams;



void streams::DynamicallyConstructedStream::refine_accuracy(DynamicallyConstructedStream::data_increment increment, resolution_t desired) const {
    assert(increment->first.power() <= desired);

    // get all the intervals in the tree "below" increment->first
    auto range = m_data_tree.equal_range(increment->first);

    // update all the leaves below until we reach the desired accuracy
    while (range.first != range.second) {
        if (range.first->second.accuracy() >= desired) {
            // Do nothing, but advance until after this interval
            DyadicInterval val(range.first->first);
            ++val;

            for (; range.first->first < val; ++range.first) {}
        } else if (DataIncrement::is_leaf(range.first)) {
            // If we're at depth and have a leaf, then
            // divide and refine down.
            DyadicInterval val(range.first->first);
            val.shrink_interval_left();
            range.first = insert_children_and_refine(range.first, val);
        } else {
            // If we're not a leaf, then continue down the tree until
            // we reach a leaf.
            ++range.first;
        }
    }

}

DynamicallyConstructedStream::data_increment streams::DynamicallyConstructedStream::insert_node(DynamicallyConstructedStream::DyadicInterval di, DynamicallyConstructedStream::Lie &&value, resolution_t accuracy, DynamicallyConstructedStream::data_increment hint) const {
    DataIncrement new_incr(std::move(value), di.power(), m_data_tree.end(), m_data_tree.end());
    return m_data_tree.insert({di, std::move(new_incr)}).first;
}
typename DynamicallyConstructedStream::data_increment streams::DynamicallyConstructedStream::expand_root_until_contains(data_increment root, DyadicInterval di) const {
    const auto &md = metadata();
    while (!root->first.contains(di)) {
        auto old_root = root;
        DyadicInterval new_root(old_root->first);
        new_root.expand_interval();

        DyadicInterval neighbour(new_root);
        if (old_root->first.aligned()) {
            neighbour.shrink_to_omitted_end();
        } else {
            neighbour.shrink_to_contained_end();
        }
        assert(neighbour != old_root->first);

        auto it_neighbour = insert_node(neighbour,
                                        make_neighbour_root_increment(neighbour),
                                        neighbour.power(),
                                        old_root);
        root = insert_node(new_root,
                           make_new_root_increment(new_root),
                           old_root->first.power(),
                           old_root);

        old_root->second.parent() = root;
        old_root->second.sibling() = it_neighbour;
        it_neighbour->second.sibling() = old_root;
        it_neighbour->second.parent() = root;
    }
    return root;
}

void DynamicallyConstructedStream::update_increment(data_increment increment,
                                                    Lie &&new_value,
                                                    resolution_t resolution) const {

    // The lock is almost certainly held, but it is not guaranteed.
    // Take the lock now to prevent un-guarded access to the data.
    std::lock_guard<std::recursive_mutex> access(m_lock);
    increment->second.lie(std::move(new_value));
    increment->second.accuracy(resolution);
}

const DynamicallyConstructedStream::Lie &streams::DynamicallyConstructedStream::lie_value(DynamicallyConstructedStream::const_data_increment increment) noexcept {
    std::lock_guard<std::recursive_mutex> access(m_lock);
    return increment->second.lie();
}

DynamicallyConstructedStream::data_increment streams::DynamicallyConstructedStream::update_parent_accuracy(DynamicallyConstructedStream::data_increment below) const {
    const auto& md = metadata();

    auto parent = below->second.parent();
    if (parent != m_data_tree.end()) {
        // We're not at the root, so parent has a sibling
        auto sibling = below->second.sibling();
        auto accuracy_available = std::min(below->second.accuracy(),
                                           sibling->second.accuracy());

        if (parent->second.accuracy() < accuracy_available) {
            if (below->first.aligned()) {
                parent->second.lie(md.default_context->cbh(
                    below->second.lie(),
                    sibling->second.lie(),
                    md.cached_vector_type
                    ));
            } else {
                parent->second.lie(md.default_context->cbh(
                    sibling->second.lie(),
                    below->second.lie(),
                    md.cached_vector_type));
            }
            parent->second.accuracy(accuracy_available);
            below = parent;
        }
    }
    return below;
}
void streams::DynamicallyConstructedStream::update_parents(DynamicallyConstructedStream::data_increment current) const {
    data_increment top;
    data_increment below(current);
    while ((top = update_parent_accuracy(below)) != below) {
        below = top;
    }
}
DynamicallyConstructedStream::data_increment
streams::DynamicallyConstructedStream::insert_children_and_refine(DynamicallyConstructedStream::data_increment leaf, DynamicallyConstructedStream::DyadicInterval interval) const {

    assert(DataIncrement::is_leaf(leaf));
    assert(leaf->first.contains(interval));

    DyadicInterval left(leaf->first);
    DyadicInterval right(leaf->first);
    left.shrink_interval_left();
    right.shrink_interval_right();

    auto new_vals = compute_child_lie_increments(left, right, leaf->second.lie());

    auto it_left = insert_node(left, std::move(new_vals.first), left.power(), leaf);
    auto it_right = insert_node(right, std::move(new_vals.second), right.power(), it_left);

    assert(it_left != leaf && it_right != leaf);

    auto& left_incr = it_left->second;
    auto& right_incr = it_right->second;
    left_incr.sibling(it_right);
    left_incr.parent(leaf);
    right_incr.sibling(it_left);
    right_incr.parent(leaf);

    update_parents(it_left);

    if (it_left->first.contains(interval)) {
        return it_left;
    }
    return it_right;
}

algebra::Lie DynamicallyConstructedStream::log_signature(const intervals::DyadicInterval &interval, resolution_t resolution, const algebra::Context &ctx) const {
    std::lock_guard<std::recursive_mutex> access(m_lock);
    const auto &md = metadata();
    const auto end = m_data_tree.end();

    auto found = m_data_tree.find(interval);
    if (found != end) {
        if (found->second.accuracy() < resolution) {
            // We need to recurse down and increase the accuracy
            refine_accuracy(found, resolution);
        }

        // Our current value is now fine.
        return found->second.lie();
    }

    auto root = m_data_tree.begin();
    if (root == end) {
        // The tree currently holds no data.
        root = insert_node(interval,
                           make_new_root_increment(interval),
                           interval.power(),
                           root);

        if (root->second.accuracy() >= resolution) {
            return root->second.lie();
        }
    }

    // If we're here, the root exists and may or may not contain the
    // interval of interest.
    root = expand_root_until_contains(root, interval);
    assert(root->first.contains(interval));

    // Now the root contains the interval of interest. Let's compute
    // what we need

    // First, compute all the children recursively until we reach
    // the desired interval.
    while (root->first != interval) {
        root = insert_children_and_refine(root, interval);
    }

    // Now refine the accuracy of our value until it meets our requirement
    refine_accuracy(root, resolution);

    return root->second.lie();
}
algebra::Lie DynamicallyConstructedStream::log_signature(const intervals::Interval &domain, resolution_t resolution, const algebra::Context &ctx) const {
    const auto &md = metadata();

    if (empty(domain)) {
        return ctx.zero_lie(md.cached_vector_type);
    }

    auto dyadic_dissection = intervals::to_dyadic_intervals(domain, resolution);
    std::vector<Lie> lies;
    lies.reserve(dyadic_dissection.size());

    for (const auto &itvl : dyadic_dissection) {
        lies.push_back(log_signature(itvl, resolution, ctx));
    }

    return ctx.cbh(lies, md.cached_vector_type);
}

DynamicallyConstructedStream::Lie streams::DynamicallyConstructedStream::make_new_root_increment(DynamicallyConstructedStream::DyadicInterval di) const {
    return log_signature_impl(di, *metadata().default_context);
}
DynamicallyConstructedStream::Lie streams::DynamicallyConstructedStream::make_neighbour_root_increment(DynamicallyConstructedStream::DyadicInterval neighbour_di) const {
    return log_signature_impl(neighbour_di, *metadata().default_context);
}
pair<algebra::Lie, algebra::Lie> streams::DynamicallyConstructedStream::compute_child_lie_increments(DynamicallyConstructedStream::DyadicInterval left_di, DynamicallyConstructedStream::DyadicInterval right_di, const DynamicallyConstructedStream::Lie &parent_value) const {
    const auto& md = metadata();
    auto half = md.data_scalar_type->from(1, 2);
    return pair<Lie, Lie>(parent_value.smul(half), parent_value.smul(half));
}