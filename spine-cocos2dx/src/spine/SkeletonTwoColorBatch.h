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

#ifndef SPINE_SKELETONTWOCOLORBATCH_H_
#define SPINE_SKELETONTWOCOLORBATCH_H_

#include <spine/spine.h>
#include <spine/pool_allocator.hpp>

#include <cocos/base/ccTypes.h>
#include <cocos/math/Mat4.h>
#include <cocos/math/Vec3.h>
#include <cocos/platform/CCGL.h>
#include <cocos/renderer/CCCustomCommand.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <unordered_map>
#include <vector>

namespace cocos2d
{
    class Texture2D;
    class GLProgramState;
    class GLProgram;
    class Renderer;
}

namespace spine {
	struct V3F_C4B_C4B_T2F {
		cocos2d::Vec3 position;
		cocos2d::Color4B color;
		cocos2d::Color4B color2;
		cocos2d::Tex2F texCoords;
    };

	struct TwoColorTriangles {
		V3F_C4B_C4B_T2F* verts = nullptr;
		unsigned short* indices = nullptr;
		int vertCount = 0;
		int indexCount = 0;
	};

	class TwoColorTrianglesCommand final : public cocos2d::CustomCommand {
	public:
		TwoColorTrianglesCommand();
        TwoColorTrianglesCommand(TwoColorTrianglesCommand const&) = delete;
        TwoColorTrianglesCommand& operator=(TwoColorTrianglesCommand const&) = delete;
        TwoColorTrianglesCommand(TwoColorTrianglesCommand &&) noexcept = delete;
        TwoColorTrianglesCommand& operator=(TwoColorTrianglesCommand &&) noexcept = delete;
		~TwoColorTrianglesCommand() = default;

		void init(float globalOrder, cocos2d::Texture2D* texture, cocos2d::GLProgramState* glProgramState, cocos2d::BlendFunc blendType, const TwoColorTriangles& triangles, const cocos2d::Mat4& mv, uint32_t flags);

		void useMaterial() const;

        inline std::size_t getMaterialID() const noexcept { return _materialID; }

		inline GLuint getTextureID() const noexcept { return _textureID; }

		inline TwoColorTriangles const& getTriangles() const noexcept { return _triangles; }

        inline std::size_t getVertexCount() const noexcept { return _triangles.vertCount; }

        inline std::size_t getIndexCount() const noexcept { return _triangles.indexCount; }

		inline V3F_C4B_C4B_T2F const* getVertices() const noexcept { return _triangles.verts; }

		inline unsigned short const* getIndices() const noexcept { return _triangles.indices; }

		inline cocos2d::GLProgramState* getGLProgramState() const noexcept { return _glProgramState; }

		inline cocos2d::BlendFunc getBlendType() const noexcept { return _blendType; }

		inline const cocos2d::Mat4& getModelView() const noexcept { return _mv; }

		void draw();

		inline void setForceFlush(bool forceFlush) noexcept { _forceFlush = forceFlush; }

		inline bool isForceFlush() const noexcept { return _forceFlush; };

	protected:
		void generateMaterialID();
        std::size_t _materialID = std::numeric_limits<std::size_t>::max();
		GLuint _textureID = 0;
		cocos2d::GLProgramState* _glProgramState = nullptr;
		cocos2d::GLProgram* _glProgram = 0;
        cocos2d::BlendFunc _blendType = cocos2d::BlendFunc::DISABLE;
		TwoColorTriangles _triangles;
		cocos2d::Mat4 _mv;
		GLuint _alphaTextureID = 0;
		bool _forceFlush = false;
#ifdef DEBUG_TEXTURE_SIZE
        cocos2d::Vec2 _texSize = cocos2d::Vec2::ZERO;
#endif
	};

    class SkeletonTwoColorBatch final
    {
        static constexpr std::size_t const MAX_VERTICES = 64000;
        static constexpr std::size_t const MAX_INDICES = 64000;

        // pool
        pool_allocator<TwoColorTrianglesCommand> _pool_commands;
        pool_allocator<V3F_C4B_C4B_T2F> _pool_vertices;
        pool_allocator<std::uint16_t> _pool_indices;

        // two color tint shader and state
        cocos2d::GLProgram* _twoColorTintShader = nullptr;
        cocos2d::GLProgramState* _twoColorTintShaderState = nullptr;

        // VBO handles & attribute locations
        GLuint _vertexBufferHandle = 0;
        std::array<V3F_C4B_C4B_T2F, MAX_VERTICES> _vertexBuffer;
        uint32_t _numVerticesBuffer = 0;
        GLuint _indexBufferHandle = 0;
        uint32_t _numIndicesBuffer = 0;
        std::array<std::uint16_t, MAX_INDICES> _indexBuffer;
        GLint _positionAttributeLocation = GL_INVALID_VALUE;
        GLint _colorAttributeLocation = GL_INVALID_VALUE;
        GLint _color2AttributeLocation = GL_INVALID_VALUE;
        GLint _texCoordsAttributeLocation = GL_INVALID_VALUE;

        // last batched command, needed for flushing to set material
        TwoColorTrianglesCommand* _lastCommand = nullptr;

        // number of batches in the last frame
        uint32_t _numBatches = 0;

    public:
        static SkeletonTwoColorBatch& getInstance();

        void init();

        void onContextRecovered();

		V3F_C4B_C4B_T2F* allocateVertices(uint32_t numVertices);
		void deallocateVertices(V3F_C4B_C4B_T2F* data, uint32_t numVertices);

		std::uint16_t* allocateIndices(uint32_t numIndices);
		void deallocateIndices(std::uint16_t* data, uint32_t numIndices);

		TwoColorTrianglesCommand* addCommand(cocos2d::Renderer* renderer, float globalOrder, cocos2d::Texture2D* texture, cocos2d::GLProgramState* glProgramState, cocos2d::BlendFunc blendType, const TwoColorTriangles& triangles, const cocos2d::Mat4& mv, uint32_t flags);

		inline cocos2d::GLProgramState* getTwoColorTintProgramState() const noexcept { return _twoColorTintShaderState; }

		void batch(TwoColorTrianglesCommand* command);

		void flush(TwoColorTrianglesCommand* materialCommand);

		inline uint32_t getNumBatches() const noexcept { return _numBatches; };

    private:
        SkeletonTwoColorBatch();
        SkeletonTwoColorBatch(SkeletonTwoColorBatch const&) = delete;
        SkeletonTwoColorBatch& operator=(SkeletonTwoColorBatch const&) = delete;
        SkeletonTwoColorBatch(SkeletonTwoColorBatch &&) noexcept = delete;
        SkeletonTwoColorBatch& operator=(SkeletonTwoColorBatch &&) noexcept = delete;
        ~SkeletonTwoColorBatch();

		void reset();

		TwoColorTrianglesCommand* allocateCommand();
	};
}

#endif // SPINE_SKELETONTWOCOLORBATCH_H_
