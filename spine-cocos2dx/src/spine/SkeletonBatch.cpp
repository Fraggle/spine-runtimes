/******************************************************************************
 * Spine Runtimes Software License v2.5
 *
 * Copyright (c) 2013-2016, Esoteric Software
 * All rights reserved.
 *
 * You are granted a perpetual, non-exclusive, non-sublicensable, and
 * non-transferable license to use, install, execute, and perform the Spine
 * Runtimes software and derivative works solely for personal or internal
 * use. Without the written permission of Esoteric Software (see Section 2 of
 * the Spine Software License Agreement), you may not (a) modify, translate,
 * adapt, or develop new applications using the Spine Runtimes or otherwise
 * create derivative works or improvements of the Spine Runtimes or (b) remove,
 * delete, alter, or obscure any trademarks or any copyright, trademark, patent,
 * or other intellectual property or proprietary rights notices on or in the
 * Software, including any copy thereof. Redistributions in binary or source
 * form must include this license and terms.
 *
 * THIS SOFTWARE IS PROVIDED BY ESOTERIC SOFTWARE "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL ESOTERIC SOFTWARE BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES, BUSINESS INTERRUPTION, OR LOSS OF
 * USE, DATA, OR PROFITS) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *****************************************************************************/

#include <spine/SkeletonBatch.h>
#include <spine/extension.h>
#include <algorithm>

USING_NS_CC;
#define EVENT_AFTER_DRAW_RESET_POSITION "director_after_draw"

namespace spine
{

    SkeletonBatch& SkeletonBatch::getInstance()
    {
        static SkeletonBatch instance;
        return instance;
    }

    SkeletonBatch::SkeletonBatch()
    {
        // callback after drawing is finished so we can clear out the batch state
        // for the next frame
        Director::getInstance()->getEventDispatcher()->addCustomEventListener(EVENT_AFTER_DRAW_RESET_POSITION, [this](EventCustom* eventCustom) {
            reset();
        });
    }

    SkeletonBatch::~SkeletonBatch()
    {
        Director::getInstance()->getEventDispatcher()->removeCustomEventListeners(EVENT_AFTER_DRAW_RESET_POSITION);
    }

    void SkeletonBatch::reset()
    {
        _pool_commands.deallocate_all();
        _pool_vertices.deallocate_all();
        _pool_indices.deallocate_all();
    }

    cocos2d::V3F_C4B_T2F* SkeletonBatch::allocateVertices(uint32_t numVertices)
    {
        return _pool_vertices.allocate(numVertices);
    }

    void SkeletonBatch::deallocateVertices(cocos2d::V3F_C4B_T2F* data, uint32_t numVertices)
    {
        _pool_vertices.deallocate(data, numVertices);
    }

    std::uint16_t* SkeletonBatch::allocateIndices(uint32_t numIndices)
    {
        return _pool_indices.allocate(numIndices);
    }

    void SkeletonBatch::deallocateIndices(std::uint16_t* data, uint32_t numIndices)
    {
        _pool_indices.deallocate(data, numIndices);
    }

    cocos2d::TrianglesCommand* SkeletonBatch::addCommand(cocos2d::Renderer* renderer, float globalOrder, cocos2d::Texture2D* texture, cocos2d::GLProgramState* glProgramState, cocos2d::BlendFunc blendType, const cocos2d::TrianglesCommand::Triangles& triangles, const cocos2d::Mat4& mv, uint32_t flags)
    {
        TrianglesCommand* command = allocateCommand();
        command->init(globalOrder, texture, glProgramState, blendType, triangles, mv, flags);
        renderer->addCommand(command);
        return command;
    }

    cocos2d::TrianglesCommand* SkeletonBatch::allocateCommand()
    {
        return _pool_commands.allocate(1);
    }
}
