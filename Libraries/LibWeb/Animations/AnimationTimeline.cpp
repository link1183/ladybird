/*
 * Copyright (c) 2023-2024, Matthew Olsson <mattco@serenityos.org>.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <LibWeb/Animations/Animation.h>
#include <LibWeb/Animations/AnimationTimeline.h>
#include <LibWeb/Bindings/AnimationTimelinePrototype.h>
#include <LibWeb/DOM/Document.h>

namespace Web::Animations {

GC_DEFINE_ALLOCATOR(AnimationTimeline);

void AnimationTimeline::set_current_time(Optional<double> value)
{
    if (value == m_current_time)
        return;

    if (m_is_monotonically_increasing && m_current_time.has_value()) {
        if (!value.has_value() || value.value() < m_current_time.value())
            m_is_monotonically_increasing = false;
    }

    m_current_time = value;
    // The loop might modify the content of m_associated_animations, so let's iterate over a copy.
    auto temporary_copy = GC::RootVector<GC::Ref<Animation>>(vm().heap());
    temporary_copy.extend(m_associated_animations.values());
    for (auto& animation : temporary_copy) {
        animation->notify_timeline_time_did_change();
    }
}

void AnimationTimeline::set_associated_document(GC::Ptr<DOM::Document> document)
{
    if (document)
        document->associate_with_timeline(*this);
    if (m_associated_document)
        m_associated_document->disassociate_with_timeline(*this);
    m_associated_document = document;
}

// https://www.w3.org/TR/web-animations-1/#inactive-timeline
bool AnimationTimeline::is_inactive() const
{
    // A timeline is considered to be inactive when its time value is unresolved.
    return !m_current_time.has_value();
}

AnimationTimeline::AnimationTimeline(JS::Realm& realm)
    : Bindings::PlatformObject(realm)
{
}

void AnimationTimeline::finalize()
{
    if (m_associated_document)
        m_associated_document->disassociate_with_timeline(*this);
}

void AnimationTimeline::initialize(JS::Realm& realm)
{
    WEB_SET_PROTOTYPE_FOR_INTERFACE(AnimationTimeline);
    Base::initialize(realm);
}

void AnimationTimeline::visit_edges(Cell::Visitor& visitor)
{
    Base::visit_edges(visitor);
    visitor.visit(m_associated_document);
    visitor.visit(m_associated_animations);
}

}
