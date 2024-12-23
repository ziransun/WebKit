/*
 * Copyright (C) 2017 Metrological Group B.V.
 * Copyright (C) 2017 Igalia S.L.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "CairoPaintingContext.h"

#if USE(CAIRO)
#include "CairoOperationRecorder.h"
#include "CoordinatedTileBuffer.h"
#include "GraphicsContext.h"
#include "GraphicsContextCairo.h"
#include <cairo.h>
#include <utility>
#include <wtf/TZoneMallocInlines.h>

namespace WebCore {
namespace Cairo {

WTF_MAKE_TZONE_ALLOCATED_IMPL(PaintingContext);

std::unique_ptr<PaintingContext> PaintingContext::createForPainting(WebCore::CoordinatedTileBuffer& buffer)
{
    return std::unique_ptr<PaintingContext>(new PaintingContext(buffer));
}

std::unique_ptr<PaintingContext> PaintingContext::createForRecording(PaintingOperations& paintingOperations)
{
    return std::unique_ptr<PaintingContext>(new PaintingContext(paintingOperations));
}

PaintingContext::PaintingContext(WebCore::CoordinatedTileBuffer& baseBuffer)
{
    // All buffers used for painting with Cairo are unaccelerated.
    auto& buffer = static_cast<WebCore::CoordinatedUnacceleratedTileBuffer&>(baseBuffer);

    // Balanced by the deref in the s_bufferKey user data destroy callback.
    buffer.ref();

    m_surface = adoptRef(cairo_image_surface_create_for_data(buffer.data(),
        CAIRO_FORMAT_ARGB32, baseBuffer.size().width(), baseBuffer.size().height(), buffer.stride()));

    static cairo_user_data_key_t s_bufferKey;
    cairo_surface_set_user_data(m_surface.get(), &s_bufferKey,
        new std::pair<WebCore::CoordinatedTileBuffer*, PaintingContext*> { &buffer, this }, [](void* data) {
            auto* userData = static_cast<std::pair<WebCore::CoordinatedTileBuffer*, PaintingContext*>*>(data);

            // Deref the CoordinatedTileBuffer object.
            userData->first->deref();
#if ASSERT_ENABLED
            // Mark the deletion of the cairo_surface_t object associated with this
            // PaintingContextCairo as complete. This way we check that the cairo_surface_t
            // object doesn't outlive the PaintingContextCairo through which it was used.
            userData->second->m_deletionComplete = true;
#endif
            delete userData;
        });

    m_graphicsContext = makeUnique<WebCore::GraphicsContextCairo>(m_surface.get());
}

PaintingContext::PaintingContext(PaintingOperations& paintingOperations)
    : m_graphicsContext(makeUnique<OperationRecorder>(paintingOperations))
{
}

PaintingContext::~PaintingContext()
{
    if (!m_surface)
        return;

    cairo_surface_flush(m_surface.get());

    m_graphicsContext = nullptr;
    m_surface = nullptr;

    // With all the Cairo references purged, the cairo_surface_t object should be destroyed
    // as well. This is checked by asserting that m_deletionComplete is true, which should
    // be the case if the s_bufferKey user data destroy callback has been invoked upon the
    // cairo_surface_t destruction.
    ASSERT(m_deletionComplete);
}

void PaintingContext::replay(const PaintingOperations& paintingOperations)
{
    ASSERT(m_surface);
    auto& context = *m_graphicsContext->platformContext();
    for (auto& operation : paintingOperations)
        operation->execute(context);
}

} // namespace Cairo
} // namespace WebCore

#endif // USE(CAIRO)
