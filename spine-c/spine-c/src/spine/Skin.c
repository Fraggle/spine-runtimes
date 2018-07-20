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

#include <spine/Skin.h>

#include <spine/extension.h>
#include <spine/Attachment.h>
#include <spine/Skeleton.h>
#include <spine/Slot.h>

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

_Entry* _Entry_create (int slotIndex, const char* name, spAttachment* attachment) {
    size_t sz = sizeof(_Entry) + strlen(name) + 1;
    _Entry* self = (_Entry*)malloc(sz);
    memset(self, 0, sz); // zero fill
    
    self->attachment = attachment;
    self->slotIndex = slotIndex;
    strcpy(self->name, name);
    return self;
}

void _Entry_dispose (_Entry* self) {
    spAttachment_dispose(self->attachment);
    FREE(self);
}

/**/

spSkin* spSkin_create (const char* name) {
    spSkin* self = SUPER(NEW(_spSkin));
    MALLOC_STR(self->name, name);
    SUB_CAST(_spSkin, self)->entries = NULL;
    return self;
}

void spSkin_dispose (spSkin* self) {
    _Entry *entry, *tmp;
    HASH_ITER(hh, SUB_CAST(_spSkin, self)->entries, entry, tmp) {
        HASH_DEL(SUB_CAST(_spSkin, self)->entries, entry);
        _Entry_dispose(entry);
    }
    FREE(self->name);
    FREE(self);
}

void spSkin_addAttachment (spSkin* self, int p_slotIndex, const char* name, spAttachment* attachment) {
    _Entry* newEntry = _Entry_create(p_slotIndex, name, attachment);
    unsigned long keylen = offsetof(_Entry, name) + strlen(name) + 1 - offsetof(_Entry, slotIndex);
    HASH_ADD(hh, SUB_CAST(_spSkin, self)->entries, slotIndex, keylen, newEntry);
}

spAttachment* spSkin_getAttachment (const spSkin* self, int slotIndex, const char* name) {
    //Create the key to search for in the hash.
    //This is a bit hacky, would prefer to use a struct, but clang doesn't support VLA in structs, but at least doing it this way avoids a malloc/free
    unsigned long sz = strlen(name) + 1;
    char key[sizeof(slotIndex) + sz];
    memcpy(key, &slotIndex, sizeof(slotIndex));
    memcpy(key+sizeof(slotIndex), name, sz);
    
    _Entry* entry = NULL;
    HASH_FIND(hh, SUB_CAST(_spSkin, self)->entries, &key, sizeof(key), entry);

    if (entry) return entry->attachment;
    
    return 0;
}

const char* spSkin_getAttachmentName (const spSkin* self, int slotIndex, int attachmentIndex) {
    const _Entry* entry = SUB_CAST(_spSkin, self)->entries;
    int i = 0;
    while (entry) {
        if (entry->slotIndex == slotIndex) {
            if (i == attachmentIndex) return entry->name;
            i++;
        }
        entry = entry->hh.next;
    }
    return 0;
}

void spSkin_attachAll (const spSkin* self, spSkeleton* skeleton, const spSkin* oldSkin) {
    const _Entry *entry = SUB_CAST(_spSkin, oldSkin)->entries;
    while (entry) {
        spSlot *slot = skeleton->slots[entry->slotIndex];
        if (slot->attachment == entry->attachment) {
            spAttachment *attachment = spSkin_getAttachment(self, entry->slotIndex, entry->name);
            if (attachment) spSlot_setAttachment(slot, attachment);
        }
        entry = entry->hh.next;
    }
}
