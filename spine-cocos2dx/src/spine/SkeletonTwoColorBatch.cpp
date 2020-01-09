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
#include <spine/SkeletonTwoColorBatch.h>

#include <spine/extension.h>

#include <cocos/base/CCDirector.h>
#include <cocos/base/CCEventDispatcher.h>
#include <cocos/renderer/CCRenderer.h>
#include <cocos/renderer/CCTexture2D.h>

#include <algorithm>

USING_NS_CC;
#define EVENT_AFTER_DRAW_RESET_POSITION "director_after_draw"
#define INITIAL_SIZE (10000)

namespace spine {

    TwoColorTrianglesCommand::TwoColorTrianglesCommand() : CustomCommand()
    {
    }

    void TwoColorTrianglesCommand::init(float globalOrder, cocos2d::Texture2D* texture,backend::ProgramState *programState, BlendFunc blendType, const TwoColorTriangles& triangles, const Mat4& mv, uint32_t flags) {

//        CCASSERT(programState->getVertexAttribsFlags() == 0, "No custom attributes are supported in QuadCommand");

        auto textureID = texture->getBackendTexture();

        RenderCommand::init(globalOrder, mv, flags);

        _triangles = triangles;
        if(_triangles.indexCount % 3 != 0) {
            int count = _triangles.indexCount;
            _triangles.indexCount = count / 3 * 3;
            CCLOGERROR("Resize indexCount from %zd to %zd, size must be multiple times of 3", count, _triangles.indexCount);
        }
        _mv = mv;

        if( _textureID != textureID ||
           _blendType.src != blendType.src ||
           _blendType.dst != blendType.dst ||
           _programState != programState ||
           _programState->getProgram() != programState->getProgram()) {
            _textureID = textureID;
            _blendType = blendType;
            _programState = programState;

            generateMaterialID();
        }
    }

    template <class T>
    inline constexpr void hash_combine(std::size_t& seed, T const& v)
    {
        std::hash<T> hasher;
        seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
    }

    void TwoColorTrianglesCommand::generateMaterialID()
    {
        std::size_t seed = 0;

        hash_combine(seed, _textureID);
        hash_combine(seed, _blendType.src);
        hash_combine(seed, _blendType.dst);
        // glProgramState is hashed because it contains:
        //  *  uniforms/values
        //  *  glProgram
        //
        // we safely can when the same glProgramState is being used then they share those states
        // if they don't have the same glProgramState, they might still have the same
        // uniforms/values and glProgram, but it would be too expensive to check the uniforms.
        hash_combine(seed, reinterpret_cast<std::uintptr_t>(_programState));

        _materialID = seed;
    }

    void TwoColorTrianglesCommand::useMaterial() {
        //Set texture
        _programState->setTexture(_programState->getUniformLocation(backend::Uniform::TEXTURE), 0, _textureID);

        if (_alphaTextureID != nullptr) {
            // ANDROID ETC1 ALPHA supports.
            _programState->setTexture(_programState->getUniformLocation(backend::Uniform::TEXTURE1), 0, _alphaTextureID);
        }
        //set blend mode
        _pipelineDescriptor.blendDescriptor.blendEnabled = true;
        _pipelineDescriptor.blendDescriptor.sourceRGBBlendFactor = _blendType.src;
        _pipelineDescriptor.blendDescriptor.destinationRGBBlendFactor = _blendType.dst;
    }

    void TwoColorTrianglesCommand::draw() {
        SkeletonTwoColorBatch::getInstance().batch(this);
    }

    const char* TWO_COLOR_TINT_VERTEX_SHADER = R"(
uniform mat4 u_MVPMatrix;
#ifdef GL_ES
    attribute mediump vec4 a_position;
    attribute lowp vec4 a_color;
    attribute lowp vec4 a_color2;
    attribute lowp vec2 a_texCoord;

    varying lowp vec4 v_light;
    varying lowp vec4 v_dark;
    varying lowp vec2 v_texCoord;
#else
    attribute vec4 a_position;
    attribute vec4 a_color;
    attribute vec4 a_color2;
    attribute vec2 a_texCoord;

    varying vec4 v_light;
    varying vec4 v_dark;
    varying vec2 v_texCoord;
#endif

    void main()
    {
        v_light = a_color;
        v_dark = a_color2;
        v_texCoord = a_texCoord;
        gl_Position = u_MVPMatrix * a_position;
    }

    )";

    const char* TWO_COLOR_TINT_FRAGMENT_SHADER = R"(
uniform sampler2D u_texture;
uniform sampler2D u_texture1;
#ifdef GL_ES
precision lowp float;

varying lowp vec4 v_light;
varying lowp vec4 v_dark;
varying lowp vec2 v_texCoord;
#else
varying vec4 v_light;
varying vec4 v_dark;
varying vec2 v_texCoord;
#endif

void main()
{
    vec4 texColor = texture2D(u_texture, v_texCoord);
    float alpha = texColor.a * v_light.a;
    gl_FragColor = vec4(((texColor.a - 1.0) * v_dark.a + 1.0 - texColor.rgb) * v_dark.rgb + texColor.rgb * v_light.rgb, alpha);
}

)";

    /** PRETTY SIMPLE ADDITION **/

    // the above shader would not work properlly with pre-multiplied alpha textures

    // ABOVE SHADER (spine one)
    // when pma v_dark.a = 1
    // when not pma v_dark.a = 0    => color = ((tex.a - 1.0) * dark.a + 1.0 - tex.rgb) * dark.rgb + tex.rgb * light.rgb
    //                              => color = (1.0 - tex.rgb) * dark.rgb + tex.rgb * light.rgb
    // which is an interpolation    => color = dark.rgb + tex.rgb * (light.rgb - dark.rgb)
    // all this in non-PMA mode
    
    // in pma mode tex.rgb has tex.a applyed tex.rgb(pma) = tex.rgb(non-pma) * tex.a right from the texture source
    // in pma mode light.rgb has light.a applyed light.rgb(pma) = light.rgb(non-pma) * light.a right from the uniform source

    // if your shader is outputing in non-pma mode but your texture is,
    // the common way is to divide tex.rgb by tex.a and divide light.rgb by light.a,
    // compute the color like the non-pma shader (above),
    // and output vec4(computedColor, alpha) if in non-PMA      (blend source factor is gl_SOURCE_ALPHA)
    // or output vec4(computedColor * alpha, alpha) if in PMA   (blend source factor is gl_ONE)

    // so if we take the default/above shader and do just that, we'll have the shader output the same thing whatever mode it is in
    
    // and to avoid the divisions....
    
    // color = dark.rgb + tex.rgb * (light.rgb - dark.rgb)
    //
    // colorPma = (dark.rgb + (tex.rgb / tex.a) * ((light.rgb / light.a) - dark.rgb)) * (tex.a * light.a)
    //
    // and we simplify to
    // colorPma = darg.rgb * tex.a * light.a + tex.rgb * (light.rgb - dark.rgb * light.a)
    

    const char* TWO_COLOR_TINT_PMA_FRAGMENT_SHADER = R"(
uniform sampler2D u_texture;

#ifdef GL_ES
precision lowp float;

varying lowp vec4 v_light; // a pma value (as skeletonRenderer opacityModifyRgb() must be true to work properly with pma)
varying lowp vec4 v_dark; // (dark alpha should be one)
varying lowp vec2 v_texCoord;
#else
varying vec4 v_light;
varying vec4 v_dark;
varying vec2 v_texCoord;
#endif

void main()
{
    vec4 texColor = texture2D(u_texture, v_texCoord); // texture is PMA

    float alpha = texColor.a * v_light.a;               // light color is PMA / new alpha will be texture alpha * light.alpha
    
    gl_FragColor = vec4(v_dark.rgb * alpha + texColor.rgb * (v_light.rgb - v_dark.rgb * v_light.a), alpha);
}

)";

    //-----------


    cocos2d::backend::ProgramState* SkeletonTwoColorBatch::getTwoColorTintProgramState()
    {
        return backend::ProgramState::create(TWO_COLOR_TINT_VERTEX_SHADER, TWO_COLOR_TINT_FRAGMENT_SHADER);
    }

    SkeletonTwoColorBatch& SkeletonTwoColorBatch::getInstance()
    {
        static SkeletonTwoColorBatch instance;
        return instance;
    }

    SkeletonTwoColorBatch::SkeletonTwoColorBatch()
    {
        // callback after drawing is finished so we can clear out the batch state
        // for the next frame
        Director::getInstance()->getEventDispatcher()->addCustomEventListener(EVENT_AFTER_DRAW_RESET_POSITION, [this](EventCustom* eventCustom) {
            reset();
        });

        init();
    }

    SkeletonTwoColorBatch::~SkeletonTwoColorBatch()
    {
        Director::getInstance()->getEventDispatcher()->removeCustomEventListeners(EVENT_AFTER_DRAW_RESET_POSITION);
    }

    void SkeletonTwoColorBatch::init()
    {  
    }

    void SkeletonTwoColorBatch::onContextRecovered()
    {
        init();
    }

    V3F_C4B_C4B_T2F* SkeletonTwoColorBatch::allocateVertices(uint32_t numVertices)
    {
        return _pool_vertices.allocate(numVertices);
    }

    void SkeletonTwoColorBatch::deallocateVertices(V3F_C4B_C4B_T2F* data, uint32_t numVertices)
    {
        _pool_vertices.deallocate(data, numVertices);
    }

    std::uint16_t* SkeletonTwoColorBatch::allocateIndices(uint32_t numIndices)
    {
        return _pool_indices.allocate(numIndices);
    }

    void SkeletonTwoColorBatch::deallocateIndices(std::uint16_t* data, uint32_t numIndices)
    {
        _pool_indices.deallocate(data, numIndices);
    }

    TwoColorTrianglesCommand* SkeletonTwoColorBatch::addCommand(cocos2d::Renderer* renderer, float globalOrder, cocos2d::Texture2D* texture, backend::ProgramState *programState,cocos2d::BlendFunc blendType, const TwoColorTriangles& triangles, const cocos2d::Mat4& mv, uint32_t flags)
    {
        TwoColorTrianglesCommand* command = allocateCommand();
        command->getPipelineDescriptor().programState = programState;
        command->init(globalOrder, texture, programState, blendType, triangles, mv, flags);
        
        const auto& matrixP = Director::getInstance()->getMatrix(MATRIX_STACK_TYPE::MATRIX_STACK_PROJECTION);
        programState->setUniform(programState->getUniformLocation(backend::Uniform::MVP_MATRIX),
                                 matrixP.m,
                                 sizeof(matrixP.m));
        
        renderer->addCommand(command);
        return command;
    }

    void SkeletonTwoColorBatch::batch(TwoColorTrianglesCommand* command)
    {
        if (_numVerticesBuffer + command->getTriangles().vertCount >= MAX_VERTICES || _numIndicesBuffer + command->getTriangles().indexCount >= MAX_INDICES)
        {
            flush(_lastCommand);
        }

        std::size_t materialID = command->getMaterialID();
        if (_lastCommand != nullptr && _lastCommand->getMaterialID() != materialID)
        {
            flush(_lastCommand);
        }

        std::memcpy(_vertexBuffer.data() + _numVerticesBuffer, command->getTriangles().verts, sizeof(V3F_C4B_C4B_T2F) * command->getTriangles().vertCount);
        const Mat4& modelView = command->getModelView();
        for (int i = _numVerticesBuffer; i < _numVerticesBuffer + command->getTriangles().vertCount; i++)
        {
            modelView.transformPoint(_vertexBuffer[i].position);
        }

        unsigned short vertexOffset = (unsigned short)_numVerticesBuffer;
        unsigned short* indices = command->getTriangles().indices;
        for (int i = 0, j = _numIndicesBuffer; i < command->getTriangles().indexCount; i++, j++)
        {
            _indexBuffer[j] = indices[i] + vertexOffset;
        }

        _numVerticesBuffer += command->getTriangles().vertCount;
        _numIndicesBuffer += command->getTriangles().indexCount;

        if (command->isForceFlush())
        {
            flush(command);
        }
        _lastCommand = command;
    }

    void SkeletonTwoColorBatch::flush(TwoColorTrianglesCommand* materialCommand)
    {
        if (materialCommand == nullptr)
            return;

        materialCommand->useMaterial();

        materialCommand->setDrawType(CustomCommand::DrawType::ELEMENT);
        materialCommand->setPrimitiveType(CustomCommand::PrimitiveType::TRIANGLE);
        
        if (materialCommand->getIndexCapacity() != MAX_INDICES)
            materialCommand->createIndexBuffer(backend::IndexFormat::U_SHORT,
                                      MAX_INDICES,
                                      backend::BufferUsage::STATIC);
        
        if (materialCommand->getVertexCapacity()!= MAX_VERTICES)
            materialCommand->createVertexBuffer(sizeof(V3F_C4B_C4B_T2F),
                                       MAX_VERTICES,
                                       backend::BufferUsage::STATIC);
        
        materialCommand->updateIndexBuffer(_indexBuffer.data(),
                                  static_cast<int>(_numIndicesBuffer * sizeof(unsigned short)));
        materialCommand->updateVertexBuffer(_vertexBuffer.data(),
                                   static_cast<int>(_numVerticesBuffer * sizeof(V3F_C4B_C4B_T2F)));
        
        materialCommand->setIndexDrawInfo(0, _numIndicesBuffer);
        
        _numVerticesBuffer = 0;
        _numIndicesBuffer = 0;
        _numBatches++;
    }

    void SkeletonTwoColorBatch::reset()
    {
        _pool_commands.deallocate_all();
        _pool_vertices.deallocate_all();
        _pool_indices.deallocate_all();

        _numVerticesBuffer = 0;
        _numIndicesBuffer = 0;
        _lastCommand = nullptr;
        _numBatches = 0;
    }

    TwoColorTrianglesCommand* SkeletonTwoColorBatch::allocateCommand()
    {
        auto ret = _pool_commands.allocate(1);
        ret->setForceFlush(false);
        return ret;
    }
}
