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

#ifndef SPINE_SKELETONBATCH_H_
#define SPINE_SKELETONBATCH_H_

#include <spine/spine.h>
#include <spine/pool_allocator.hpp>

#include <cocos/renderer/CCTrianglesCommand.h>
#include <cocos/base/ccTypes.h>
#include <cocos/math/Mat4.h>

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace cocos2d
{
    class Renderer;
    class Texture2D;
    class GLProgramState;
    class Mat4;
}

namespace spine
{
    
    class SkeletonBatch final
    {
        pool_allocator<cocos2d::TrianglesCommand> _pool_commands;
        pool_allocator<cocos2d::V3F_C4B_T2F> _pool_vertices;
        pool_allocator<std::uint16_t> _pool_indices;

    public:
        static SkeletonBatch& getInstance();
		
		cocos2d::V3F_C4B_T2F* allocateVertices(std::uint32_t numVertices);
		void deallocateVertices(cocos2d::V3F_C4B_T2F* data, uint32_t numVertices);

        std::uint16_t* allocateIndices(std::uint32_t numIndices);
		void deallocateIndices(std::uint16_t* data, uint32_t numIndices);

		cocos2d::TrianglesCommand* addCommand(cocos2d::Renderer* renderer, float globalOrder, cocos2d::Texture2D* texture, cocos2d::GLProgramState* glProgramState, cocos2d::BlendFunc blendType, const cocos2d::TrianglesCommand::Triangles& triangles, const cocos2d::Mat4& mv, std::uint32_t flags);
        
    private:
        SkeletonBatch();
        SkeletonBatch(SkeletonBatch const&) = delete;
        SkeletonBatch& operator=(SkeletonBatch const&) = delete;
        SkeletonBatch(SkeletonBatch &&) noexcept = delete;
        SkeletonBatch& operator=(SkeletonBatch &&) noexcept = delete;
        ~SkeletonBatch();

        void reset();
		
		cocos2d::TrianglesCommand* allocateCommand();
    };
	
}

#endif // SPINE_SKELETONBATCH_H_
