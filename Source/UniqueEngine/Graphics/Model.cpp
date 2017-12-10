//
// Copyright (c) 2008-2017 the Unique project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "Precompiled.h"

#include "Core/Context.h"
#include "Core/Profiler.h"
#include "../Graphics/Geometry.h"
#include "../Graphics/GraphicsBuffer.h"
#include "../Graphics/Model.h"
#include "../Graphics/Graphics.h"
#include "IO/Log.h"
#include "IO/File.h"
#include "IO/FileSystem.h"
#include "Resource/ResourceCache.h"

// Legacy vertex element bitmasks.
static const unsigned MASK_NONE = 0x0;
static const unsigned MASK_POSITION = 0x1;
static const unsigned MASK_NORMAL = 0x2;
static const unsigned MASK_COLOR = 0x4;
static const unsigned MASK_TEXCOORD1 = 0x8;
static const unsigned MASK_TEXCOORD2 = 0x10;
static const unsigned MASK_CUBETEXCOORD1 = 0x20;
static const unsigned MASK_CUBETEXCOORD2 = 0x40;
static const unsigned MASK_TANGENT = 0x80;
static const unsigned MASK_BLENDWEIGHTS = 0x100;
static const unsigned MASK_BLENDINDICES = 0x200;
static const unsigned MASK_INSTANCEMATRIX1 = 0x400;
static const unsigned MASK_INSTANCEMATRIX2 = 0x800;
static const unsigned MASK_INSTANCEMATRIX3 = 0x1000;
static const unsigned MASK_OBJECTINDEX = 0x2000;

namespace Unique
{

unsigned LookupVertexBuffer(VertexBuffer* buffer, const Vector<SPtr<VertexBuffer> >& buffers)
{
    for (unsigned i = 0; i < buffers.size(); ++i)
    {
        if (buffers[i] == buffer)
            return i;
    }
    return 0;
}

unsigned LookupIndexBuffer(IndexBuffer* buffer, const Vector<SPtr<IndexBuffer> >& buffers)
{
    for (unsigned i = 0; i < buffers.size(); ++i)
    {
        if (buffers[i] == buffer)
            return i;
    }
    return 0;
}


uObject(Model)
{

}

Model::Model()
{
}

Model::~Model()
{
}

bool Model::BeginLoad(IFile& source)
{
    // Check ID
    String fileID = source.ReadFileID();
    if (fileID != "UMDL" && fileID != "UMD2")
    {
        UNIQUE_LOGERROR(source.GetName() + " is not a valid model file");
        return false;
    }

    bool hasVertexDeclarations = (fileID == "UMD2");

    geometries_.clear();
    geometryBoneMappings_.clear();
    geometryCenters_.clear();
    morphs_.clear();
    vertexBuffers_.clear();
    indexBuffers_.clear();

    unsigned memoryUse = sizeof(Model);
    bool async = GetAsyncLoadState() == ASYNC_LOADING;

    // Read vertex buffers
    unsigned numVertexBuffers = source.Read<unsigned>();
    vertexBuffers_.reserve(numVertexBuffers);
    morphRangeStarts_.resize(numVertexBuffers);
    morphRangeCounts_.resize(numVertexBuffers);
	loadVBData_.resize(numVertexBuffers);

    for (unsigned i = 0; i < numVertexBuffers; ++i)
    {
        VertexBufferDesc& desc = loadVBData_[i];

        desc.vertexCount_ = source.Read<uint>();
        if (!hasVertexDeclarations)
        {
            unsigned elementMask = source.Read<uint>();
        //    desc.vertexElements_ = VertexBuffer::GetElements(elementMask);
        }
        else
        {/*
            desc.vertexElements_.clear();
            unsigned numElements = source.Read<uint>();
            for (unsigned j = 0; j < numElements; ++j)
            {
                unsigned elementDesc = source.Read<uint>();
                VertexElementType type = (VertexElementType)(elementDesc & 0xff);
                VertexElementSemantic semantic = (VertexElementSemantic)((elementDesc >> 8) & 0xff);
                unsigned char index = (unsigned char)((elementDesc >> 16) & 0xff);
                desc.vertexElements_.push_back(VertexElement(type, semantic, index));
            }*/
        }

        morphRangeStarts_[i] = source.Read<uint>();
        morphRangeCounts_[i] = source.Read<uint>();

        SPtr<VertexBuffer> buffer(new VertexBuffer());
		unsigned vertexSize = 0;// VertexBuffer::GetVertexSize(desc.vertexElements_);
        desc.dataSize_ = desc.vertexCount_ * vertexSize;

        // Prepare vertex buffer data to be uploaded during EndLoad()
        if (async)
        {
            desc.data_ = new unsigned char[desc.dataSize_];
            source.Read(desc.data_.Get(), desc.dataSize_);
        }
        else
        {
            // If not async loading, use locking to avoid extra allocation & copy
            desc.data_.Reset(); // Make sure no previous data
			/*
            buffer->SetShadowed(true);
            buffer->SetSize(desc.vertexCount_, desc.vertexElements_);
            void* dest = buffer->Lock(0, desc.vertexCount_);
            source.Read(dest, desc.vertexCount_ * vertexSize);
            buffer->Unlock();*/
        }

        memoryUse += sizeof(VertexBuffer) + desc.vertexCount_ * vertexSize;
        vertexBuffers_.push_back(buffer);
    }

    // Read index buffers
    unsigned numIndexBuffers = source.Read<uint>();
    indexBuffers_.reserve(numIndexBuffers);
    loadIBData_.resize(numIndexBuffers);
    for (unsigned i = 0; i < numIndexBuffers; ++i)
    {
        unsigned indexCount = source.Read<uint>();
        unsigned indexSize = source.Read<uint>();

        SPtr<IndexBuffer> buffer(new IndexBuffer());

        // Prepare index buffer data to be uploaded during EndLoad()
        if (async)
        {
            loadIBData_[i].indexCount_ = indexCount;
            loadIBData_[i].indexSize_ = indexSize;
            loadIBData_[i].dataSize_ = indexCount * indexSize;
            loadIBData_[i].data_ = new unsigned char[loadIBData_[i].dataSize_];
            source.Read(loadIBData_[i].data_.Get(), loadIBData_[i].dataSize_);
        }
        else
        {
            // If not async loading, use locking to avoid extra allocation & copy
            loadIBData_[i].data_.Reset(); // Make sure no previous data
            /*
			buffer->SetShadowed(true);
            buffer->SetSize(indexCount, indexSize > sizeof(unsigned short));
            void* dest = buffer->Lock(0, indexCount);
            source.Read(dest, indexCount * indexSize);
            buffer->Unlock();*/
        }

        memoryUse += sizeof(IndexBuffer) + indexCount * indexSize;
        indexBuffers_.push_back(buffer);
    }

    // Read geometries
    unsigned numGeometries = source.Read<uint>();
    geometries_.reserve(numGeometries);
    geometryBoneMappings_.reserve(numGeometries);
    geometryCenters_.reserve(numGeometries);
    loadGeometries_.resize(numGeometries);
    for (unsigned i = 0; i < numGeometries; ++i)
    {
        // Read bone mappings
        unsigned boneMappingCount = source.Read<uint>();
        PODVector<unsigned> boneMapping(boneMappingCount);
        for (unsigned j = 0; j < boneMappingCount; ++j)
            boneMapping[j] = source.Read<uint>();
        geometryBoneMappings_.push_back(boneMapping);

        unsigned numLodLevels = source.Read<uint>();
        Vector<SPtr<Geometry> > geometryLodLevels;
        geometryLodLevels.reserve(numLodLevels);
        loadGeometries_[i].resize(numLodLevels);

        for (unsigned j = 0; j < numLodLevels; ++j)
        {
            float distance = source.Read<float>();
            PrimitiveTopology type = (PrimitiveTopology)source.Read<uint>();

            unsigned vbRef = source.Read<uint>();
            unsigned ibRef = source.Read<uint>();
            unsigned indexStart = source.Read<uint>();
            unsigned indexCount = source.Read<uint>();

            if (vbRef >= vertexBuffers_.size())
            {
                UNIQUE_LOGERROR("Vertex buffer index out of bounds");
                loadVBData_.clear();
                loadIBData_.clear();
                loadGeometries_.clear();
                return false;
            }
            if (ibRef >= indexBuffers_.size())
            {
                UNIQUE_LOGERROR("Index buffer index out of bounds");
                loadVBData_.clear();
                loadIBData_.clear();
                loadGeometries_.clear();
                return false;
            }

            SPtr<Geometry> geometry(new Geometry());
            geometry->SetLodDistance(distance);

            // Prepare geometry to be defined during EndLoad()
            loadGeometries_[i][j].type_ = type;
            loadGeometries_[i][j].vbRef_ = vbRef;
            loadGeometries_[i][j].ibRef_ = ibRef;
            loadGeometries_[i][j].indexStart_ = indexStart;
            loadGeometries_[i][j].indexCount_ = indexCount;

            geometryLodLevels.push_back(geometry);
            memoryUse += sizeof(Geometry);
        }

        geometries_.push_back(geometryLodLevels);
    }

    // Read morphs
    unsigned numMorphs = source.Read<uint>();
    morphs_.reserve(numMorphs);
    for (unsigned i = 0; i < numMorphs; ++i)
    {
        ModelMorph newMorph;

        newMorph.name_ = source.ReadString();
        newMorph.nameHash_ = newMorph.name_;
        newMorph.weight_ = 0.0f;
        unsigned numBuffers = source.Read<uint>();

        for (unsigned j = 0; j < numBuffers; ++j)
        {
            VertexBufferMorph newBuffer;
            unsigned bufferIndex = source.Read<uint>();

            newBuffer.elementMask_ = source.Read<uint>();
            newBuffer.vertexCount_ = source.Read<uint>();

            // Base size: size of each vertex index
            unsigned vertexSize = sizeof(unsigned);
            // Add size of individual elements
            if (newBuffer.elementMask_ & MASK_POSITION)
                vertexSize += sizeof(Vector3);
            if (newBuffer.elementMask_ & MASK_NORMAL)
                vertexSize += sizeof(Vector3);
            if (newBuffer.elementMask_ & MASK_TANGENT)
                vertexSize += sizeof(Vector3);
            newBuffer.dataSize_ = newBuffer.vertexCount_ * vertexSize;
            newBuffer.morphData_ = new unsigned char[newBuffer.dataSize_];

            source.Read(&newBuffer.morphData_[0], newBuffer.vertexCount_ * vertexSize);

            newMorph.buffers_[bufferIndex] = newBuffer;
            memoryUse += sizeof(VertexBufferMorph) + newBuffer.vertexCount_ * vertexSize;
        }

        morphs_.push_back(newMorph);
        memoryUse += sizeof(ModelMorph);
    }

    // Read skeleton
    skeleton_.Load(source);
    memoryUse += skeleton_.GetNumBones() * sizeof(Bone);

    // Read bounding box
    boundingBox_ = source.Read<BoundingBox>();

    // Read geometry centers
    for (unsigned i = 0; i < geometries_.size() && !source.IsEof(); ++i)
        geometryCenters_.push_back(source.Read<Vector3>());
    while (geometryCenters_.size() < geometries_.size())
        geometryCenters_.push_back(Vector3::ZERO);
    memoryUse += sizeof(Vector3) * (unsigned)geometries_.size();

    SetMemoryUse(memoryUse);

    return true;
}

bool Model::EndLoad()
{
#if false
    // Upload vertex buffer data
    for (unsigned i = 0; i < vertexBuffers_.size(); ++i)
    {
        VertexBuffer* buffer = vertexBuffers_[i];
        VertexBufferDesc& desc = loadVBData_[i];
        if (desc.data_)
        {
            buffer->SetShadowed(true);
            buffer->SetSize(desc.vertexCount_, desc.vertexElements_);
            buffer->SetData(desc.data_.Get());
        }
    }

    // Upload index buffer data
    for (unsigned i = 0; i < indexBuffers_.size(); ++i)
    {
        IndexBuffer* buffer = indexBuffers_[i];
        IndexBufferDesc& desc = loadIBData_[i];
        if (desc.data_)
        {
            buffer->SetShadowed(true);
            buffer->SetSize(desc.indexCount_, desc.indexSize_ > sizeof(unsigned short));
            buffer->SetData(desc.data_.Get());
        }
    }

    // Set up geometries
    for (unsigned i = 0; i < geometries_.size(); ++i)
    {
        for (unsigned j = 0; j < geometries_[i].size(); ++j)
        {
            Geometry* geometry = geometries_[i][j];
            GeometryDesc& desc = loadGeometries_[i][j];
            geometry->SetVertexBuffer(0, vertexBuffers_[desc.vbRef_]);
            geometry->SetIndexBuffer(indexBuffers_[desc.ibRef_]);
            geometry->SetDrawRange(desc.type_, desc.indexStart_, desc.indexCount_);
        }
    }

    loadVBData_.clear();
    loadIBData_.clear();
    loadGeometries_.clear();
#endif
    return true;
}

void Model::SetBoundingBox(const BoundingBox& box)
{
    boundingBox_ = box;
}

bool Model::SetVertexBuffers(const Vector<SPtr<VertexBuffer> >& buffers, const PODVector<unsigned>& morphRangeStarts,
    const PODVector<unsigned>& morphRangeCounts)
{
    for (unsigned i = 0; i < buffers.size(); ++i)
    {
        if (!buffers[i])
        {
            UNIQUE_LOGERROR("Null model vertex buffers specified");
            return false;
        }
    }

    vertexBuffers_ = buffers;
    morphRangeStarts_.resize((unsigned)buffers.size());
    morphRangeCounts_.resize((unsigned)buffers.size());

    // If morph ranges are not specified for buffers, assume to be zero
    for (unsigned i = 0; i < buffers.size(); ++i)
    {
        morphRangeStarts_[i] = i < morphRangeStarts.size() ? morphRangeStarts[i] : 0;
        morphRangeCounts_[i] = i < morphRangeCounts.size() ? morphRangeCounts[i] : 0;
    }

    return true;
}

bool Model::SetIndexBuffers(const Vector<SPtr<IndexBuffer> >& buffers)
{
    for (unsigned i = 0; i < buffers.size(); ++i)
    {
        if (!buffers[i])
        {
            UNIQUE_LOGERROR("Null model index buffers specified");
            return false;
        }

    }

    indexBuffers_ = buffers;
    return true;
}

void Model::SetNumGeometries(unsigned num)
{
    geometries_.resize(num);
    geometryBoneMappings_.resize(num);
    geometryCenters_.resize(num);

    // For easier creation of from-scratch geometry, ensure that all geometries start with at least 1 LOD level (0 makes no sense)
    for (unsigned i = 0; i < geometries_.size(); ++i)
    {
        if (geometries_[i].empty())
            geometries_[i].resize(1);
    }
}

bool Model::SetNumGeometryLodLevels(unsigned index, unsigned num)
{
    if (index >= geometries_.size())
    {
        UNIQUE_LOGERROR("Geometry index out of bounds");
        return false;
    }
    if (!num)
    {
        UNIQUE_LOGERROR("Zero LOD levels not allowed");
        return false;
    }

    geometries_[index].resize(num);
    return true;
}

bool Model::SetGeometry(unsigned index, unsigned lodLevel, Geometry* geometry)
{
    if (index >= geometries_.size())
    {
        UNIQUE_LOGERROR("Geometry index out of bounds");
        return false;
    }
    if (lodLevel >= geometries_[index].size())
    {
        UNIQUE_LOGERROR("LOD level index out of bounds");
        return false;
    }

    geometries_[index][lodLevel] = geometry;
    return true;
}

bool Model::SetGeometryCenter(unsigned index, const Vector3& center)
{
    if (index >= geometryCenters_.size())
    {
        UNIQUE_LOGERROR("Geometry index out of bounds");
        return false;
    }

    geometryCenters_[index] = center;
    return true;
}

void Model::SetSkeleton(const Skeleton& skeleton)
{
    skeleton_ = skeleton;
}

void Model::SetGeometryBoneMappings(const Vector<PODVector<unsigned> >& geometryBoneMappings)
{
    geometryBoneMappings_ = geometryBoneMappings;
}

void Model::SetMorphs(const Vector<ModelMorph>& morphs)
{
    morphs_ = morphs;
}
/*
SPtr<Model> Model::Clone(const String& cloneName) const
{
    SPtr<Model> ret(new Model());

    ret->SetName(cloneName);
    ret->boundingBox_ = boundingBox_;
    ret->skeleton_ = skeleton_;
    ret->geometryBoneMappings_ = geometryBoneMappings_;
    ret->geometryCenters_ = geometryCenters_;
    ret->morphs_ = morphs_;
    ret->morphRangeStarts_ = morphRangeStarts_;
    ret->morphRangeCounts_ = morphRangeCounts_;

    // Deep copy vertex/index buffers
    HashMap<VertexBuffer*, VertexBuffer*> vbMapping;
    for (Vector<SPtr<VertexBuffer> >::const_iterator i = vertexBuffers_.begin(); i != vertexBuffers_.end(); ++i)
    {
        VertexBuffer* origBuffer = *i;
        SPtr<VertexBuffer> cloneBuffer;

        if (origBuffer)
        {
            cloneBuffer = new VertexBuffer();
            cloneBuffer->SetSize(origBuffer->GetVertexCount(), origBuffer->GetElementMask(), origBuffer->usage());
            cloneBuffer->SetShadowed(origBuffer->IsShadowed());
            if (origBuffer->IsShadowed())
                cloneBuffer->SetData(origBuffer->GetShadowData());
            else
            {
                void* origData = origBuffer->Lock(0, origBuffer->GetVertexCount());
                if (origData)
                    cloneBuffer->SetData(origData);
                else
                    UNIQUE_LOGERROR("Failed to lock original vertex buffer for copying");
            }
            vbMapping[origBuffer] = cloneBuffer;
        }

        ret->vertexBuffers_.push_back(cloneBuffer);
    }

    HashMap<IndexBuffer*, IndexBuffer*> ibMapping;
    for (Vector<SPtr<IndexBuffer> >::const_iterator i = indexBuffers_.begin(); i != indexBuffers_.end(); ++i)
    {
        IndexBuffer* origBuffer = *i;
        SPtr<IndexBuffer> cloneBuffer;

        if (origBuffer)
        {
            cloneBuffer = new IndexBuffer();
            cloneBuffer->SetSize(origBuffer->GetIndexCount(), origBuffer->GetIndexSize() == sizeof(unsigned),
                origBuffer->IsDynamic());
            cloneBuffer->SetShadowed(origBuffer->IsShadowed());
            if (origBuffer->IsShadowed())
                cloneBuffer->SetData(origBuffer->GetShadowData());
            else
            {
                void* origData = origBuffer->Lock(0, origBuffer->GetIndexCount());
                if (origData)
                    cloneBuffer->SetData(origData);
                else
                    UNIQUE_LOGERROR("Failed to lock original index buffer for copying");
            }
            ibMapping[origBuffer] = cloneBuffer;
        }

        ret->indexBuffers_.push_back(cloneBuffer);
    }

    // Deep copy all the geometry LOD levels and refer to the copied vertex/index buffers
    ret->geometries_.resize(geometries_.size());
    for (unsigned i = 0; i < geometries_.size(); ++i)
    {
        ret->geometries_[i].resize(geometries_[i].size());
        for (unsigned j = 0; j < geometries_[i].size(); ++j)
        {
            SPtr<Geometry> cloneGeometry;
            Geometry* origGeometry = geometries_[i][j];

            if (origGeometry)
            {
                cloneGeometry = new Geometry();
                cloneGeometry->SetIndexBuffer(ibMapping[origGeometry->GetIndexBuffer()]);
                unsigned numVbs = origGeometry->GetNumVertexBuffers();
                for (unsigned k = 0; k < numVbs; ++k)
                {
                    cloneGeometry->SetVertexBuffer(k, vbMapping[origGeometry->GetVertexBuffer(k)]);
                }
                cloneGeometry->SetDrawRange(origGeometry->GetPrimitiveType(), origGeometry->GetIndexStart(),
                    origGeometry->GetIndexCount(), origGeometry->GetVertexStart(), origGeometry->GetVertexCount(), false);
                cloneGeometry->SetLodDistance(origGeometry->GetLodDistance());
            }

            ret->geometries_[i][j] = cloneGeometry;
        }
    }


    // Deep copy the morph data (if any) to allow modifying it
    for (Vector<ModelMorph>::iterator i = ret->morphs_.begin(); i != ret->morphs_.end(); ++i)
    {
        ModelMorph& morph = *i;
        for (HashMap<unsigned, VertexBufferMorph>::iterator j = morph.buffers_.begin(); j != morph.buffers_.end(); ++j)
        {
            VertexBufferMorph& vbMorph = j->second;
            if (vbMorph.dataSize_)
            {
                SharedArrayPtr<unsigned char> cloneData(new unsigned char[vbMorph.dataSize_]);
                memcpy(cloneData.Get(), vbMorph.morphData_.Get(), vbMorph.dataSize_);
                vbMorph.morphData_ = cloneData;
            }
        }
    }

    ret->SetMemoryUse(GetMemoryUse());

    return ret;
}*/

unsigned Model::GetNumGeometryLodLevels(unsigned index) const
{
    return index < geometries_.size() ? (unsigned)geometries_[index].size() : 0;
}

Geometry* Model::GetGeometry(unsigned index, unsigned lodLevel) const
{
    if (index >= geometries_.size() || geometries_[index].empty())
        return 0;

    if (lodLevel >= (unsigned)geometries_[index].size())
        lodLevel = (unsigned)geometries_[index].size() - 1;

    return geometries_[index][lodLevel];
}

const ModelMorph* Model::GetMorph(unsigned index) const
{
    return index < morphs_.size() ? &morphs_[index] : 0;
}

const ModelMorph* Model::GetMorph(const String& name) const
{
    return GetMorph(StringID(name));
}

const ModelMorph* Model::GetMorph(StringID nameHash) const
{
    for (Vector<ModelMorph>::const_iterator i = morphs_.begin(); i != morphs_.end(); ++i)
    {
        if (i->nameHash_ == nameHash)
            return &(*i);
    }

    return 0;
}

unsigned Model::GetMorphRangeStart(unsigned bufferIndex) const
{
    return bufferIndex < vertexBuffers_.size() ? morphRangeStarts_[bufferIndex] : 0;
}

unsigned Model::GetMorphRangeCount(unsigned bufferIndex) const
{
    return bufferIndex < vertexBuffers_.size() ? morphRangeCounts_[bufferIndex] : 0;
}

}
