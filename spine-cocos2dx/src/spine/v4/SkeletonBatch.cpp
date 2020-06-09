/******************************************************************************
 * Spine Runtimes License Agreement
 * Last updated January 1, 2020. Replaces all prior versions.
 *
 * Copyright (c) 2013-2020, Esoteric Software LLC
 *
 * Integration of the Spine Runtimes into software or otherwise creating
 * derivative works of the Spine Runtimes is permitted under the terms and
 * conditions of Section 2 of the Spine Editor License Agreement:
 * http://esotericsoftware.com/spine-editor-license
 *
 * Otherwise, it is permitted to integrate the Spine Runtimes into software
 * or otherwise create derivative works of the Spine Runtimes (collectively,
 * "Products"), provided that each user of the Products must obtain their own
 * Spine Editor license and redistribution of the Products in any form must
 * include this license and copyright notice.
 *
 * THE SPINE RUNTIMES ARE PROVIDED BY ESOTERIC SOFTWARE LLC "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL ESOTERIC SOFTWARE LLC BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES,
 * BUSINESS INTERRUPTION, OR LOSS OF USE, DATA, OR PROFITS) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THE SPINE RUNTIMES, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *****************************************************************************/

#include <spine/spine-cocos2dx.h>
#if COCOS2D_VERSION >= 0x00040000

#include <spine/Extension.h>
#include <algorithm>

USING_NS_CC;
#define EVENT_AFTER_DRAW_RESET_POSITION "director_after_draw"
using std::max;
#define INITIAL_SIZE (10000)
#define INITIAL_CC_SIZE (10)

#include "renderer/ccShaders.h"
#include "renderer/backend/Device.h"
#include "renderer/CCProgramStateCache.h"

namespace spine {

SkeletonBatch &SkeletonBatch::getInstance()
{
    static SkeletonBatch _instance;
    return _instance;
}

SkeletonBatch::SkeletonBatch()
: _lastCmdMaterialId(0)
, _lastCmdMaterialIdGlobalOrder(-1e9f)
, _lastCmdMaterialIdGroupId(-1)
{
    for (unsigned int i = 0; i < INITIAL_SIZE; i++)
        _commandsPool.push_back(createNewTrianglesCommand());
    for (unsigned int i = 0; i < INITIAL_CC_SIZE; i++)
        _callbackCommandsPool.push_back(createNewCCCommand());
    
    reset();
    
    // callback after drawing is finished so we can clear out the batch state
    // for the next frame
    Director::getInstance()->getEventDispatcher()->addCustomEventListener(EVENT_AFTER_DRAW_RESET_POSITION, [this](EventCustom* eventCustom) {
        reset();
    });
}

SkeletonBatch::~SkeletonBatch()
{
    Director::getInstance()->getEventDispatcher()->removeCustomEventListeners(EVENT_AFTER_DRAW_RESET_POSITION);
    
    for (unsigned int i = 0; i < _commandsPool.size(); i++) {
//        CC_SAFE_RELEASE(_commandsPool[i]->getPipelineDescriptor().programState);
        delete _commandsPool[i];
        _commandsPool[i] = nullptr;
    }
    _commandsPool.clear();
    
    for (unsigned int i = 0; i < _callbackCommandsPool.size(); i++) {
        delete _callbackCommandsPool[i];
        _callbackCommandsPool[i] = nullptr;
    }
    _callbackCommandsPool.clear();
}

void SkeletonBatch::reset()
{
    // Release program state
//    for(auto &c : _commandsPool)
//        CC_SAFE_RELEASE_NULL(c->getPipelineDescriptor().programState);
    
    _nextFreeCommand = 0;
    _nextFreeCCCommand = 0;
    _numVertices = 0;
    _indices.setSize(0, 0);
    
    _lastCmdMaterialId = 0;
    _lastCmdMaterialIdGlobalOrder = -1e9f;
    _lastCmdMaterialIdGroupId = -1;
}

cocos2d::V3F_C4B_T2F* SkeletonBatch::allocateVertices(uint32_t numVertices)
{
    if (_vertices.size() - _numVertices < numVertices) {
        cocos2d::V3F_C4B_T2F* oldData = _vertices.data();
        _vertices.resize((_vertices.size() + numVertices) * 2 + 1);
        cocos2d::V3F_C4B_T2F* newData = _vertices.data();
        for (uint32_t i = 0; i < this->_nextFreeCommand; i++) {
            TrianglesCommand* command = _commandsPool[i];
            cocos2d::TrianglesCommand::Triangles& triangles = (cocos2d::TrianglesCommand::Triangles&)command->getTriangles();
            triangles.verts = newData + (triangles.verts - oldData);
        }
    }
    
    cocos2d::V3F_C4B_T2F* vertices = _vertices.data() + _numVertices;
    _numVertices += numVertices;
    return vertices;
}

void SkeletonBatch::deallocateVertices(uint32_t numVertices)
{
    _numVertices -= numVertices;
}

unsigned short* SkeletonBatch::allocateIndices(uint32_t numIndices)
{
    if (_indices.getCapacity() - _indices.size() < numIndices) {
        unsigned short* oldData = _indices.buffer();
        int oldSize = _indices.size();
        _indices.ensureCapacity(_indices.size() + numIndices);
        unsigned short* newData = _indices.buffer();
        for (uint32_t i = 0; i < this->_nextFreeCommand; i++) {
            TrianglesCommand* command = _commandsPool[i];
            cocos2d::TrianglesCommand::Triangles& triangles = (cocos2d::TrianglesCommand::Triangles&)command->getTriangles();
            if (triangles.indices >= oldData && triangles.indices < oldData + oldSize) {
                triangles.indices = newData + (triangles.indices - oldData);
            }
        }
    }
    
    unsigned short* indices = _indices.buffer() + _indices.size();
    _indices.setSize(_indices.size() + numIndices, 0);
    return indices;
}

void SkeletonBatch::deallocateIndices(uint32_t numIndices)
{
    _indices.setSize(_indices.size() - numIndices, 0);
}

cocos2d::TrianglesCommand* SkeletonBatch::addCommand(SkeletonRenderer*                           skeleton,
                                                     cocos2d::Renderer *                         renderer,
                                                     float                                       globalOrder,
                                                     cocos2d::Texture2D *                        texture,
                                                     cocos2d::BlendFunc                          blendType,
                                                     const cocos2d::TrianglesCommand::Triangles &triangles,
                                                     const cocos2d::Mat4 &                       mv,
                                                     uint32_t                                    flags)
{
    cocos2d::backend::ProgramState* programState = skeleton->getProgramState(); // in standard case the Node _programState is null, but sometimes (ie a GameFilter with useRenderTarget=false) can override the program
    
    if (!programState ||
        programState->getProgram()->getProgramType() == cocos2d::backend::ProgramType::POSITION_TEXTURE_COLOR) {
        programState = ProgramStateCache::getOrCreateProgramState(cocos2d::backend::ProgramType::POSITION_TEXTURE_COLOR,
                                                                  texture,
                                                                  blendType,
                                                                  0,
                                                                  [](auto p_programState) {
            ProgramStateCache::setUpStandardAttributeLayout(p_programState);
        });
    }
    
    TrianglesCommand* command = nextFreeCommand();
    command->getPipelineDescriptor().programState = programState;
    command->init(globalOrder, texture, blendType, triangles, mv, flags);
    if (programState->getProgram()->getProgramType() == cocos2d::backend::ProgramType::INVALID_PROGRAM) // INVALID_PROGRAM actually means that the program is not standard (ie in case of a filter)
        command->enableBatchingForCustomShader(programState->getUniformsBufferHash());
    
    if (_lastCmdMaterialId != command->getMaterialID() ||
        globalOrder < _lastCmdMaterialIdGlobalOrder ||
        _lastCmdMaterialIdGroupId != renderer->currentGroupId()) {
        ProgramStateCache::addStandardUniformRenderCommand(nextFreeCCCommand(),
                                                           renderer,
                                                           programState,
                                                           globalOrder,
                                                           texture,
                                                           false);
        _lastCmdMaterialId = command->getMaterialID();
        _lastCmdMaterialIdGlobalOrder = globalOrder;
        _lastCmdMaterialIdGroupId = renderer->currentGroupId();
    }
    
    renderer->addCommand(command);
    return command;
}

cocos2d::TrianglesCommand* SkeletonBatch::nextFreeCommand()
{
    if (_commandsPool.size() <= _nextFreeCommand) {
        unsigned int newSize = _commandsPool.size() * 2 + 1;
        for (int i = _commandsPool.size(); i < newSize; i++) {
            _commandsPool.push_back(createNewTrianglesCommand());
        }
    }
    return _commandsPool[_nextFreeCommand++];
}

cocos2d::TrianglesCommand *SkeletonBatch::createNewTrianglesCommand()
{
    auto* command = new TrianglesCommand();
    return command;
}

cocos2d::CallbackCommand* SkeletonBatch::nextFreeCCCommand()
{
    if (_callbackCommandsPool.size() <= _nextFreeCCCommand) {
        unsigned int newSize = _callbackCommandsPool.size() * 2 + 1;
        for (int i = _callbackCommandsPool.size(); i < newSize; i++) {
            _callbackCommandsPool.push_back(createNewCCCommand());
        }
    }
    return _callbackCommandsPool[_nextFreeCCCommand++];
}

cocos2d::CallbackCommand *SkeletonBatch::createNewCCCommand()
{
    auto* command = new CallbackCommand();
    return command;
}

}

#endif
