//
//  RenderableLeoPolyEntityItem.cpp
//  libraries/entities-renderer/src/
//
//  Created by Seth Alves on 2016-11-5.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <math.h>
#include <QObject>
#include <QByteArray>
#include <QtConcurrent/QtConcurrentRun>
#include <glm/gtx/transform.hpp>
#include <OBJReader.h>
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#endif

#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

#include <Model.h>
#include <PerfStat.h>
#include <render/Scene.h>

#include "model/Geometry.h"
#include "EntityTreeRenderer.h"
#include "leoPoly_vert.h"
#include "leoPoly_frag.h"
#include "polyvox_vert.h"
#include "polyvox_frag.h"
#include "RenderableLeoPolyEntityItem.h"
#include "EntityEditPacketSender.h"
#include "PhysicalEntitySimulation.h"
#include <AssetScriptingInterface.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#ifdef WIN32
#include <io.h>
#include <curl/curl.h>
#else
#include <unistd.h>
#endif
#include <qfileinfo.h>

// Plugin.h(85) : warning C4091 : '__declspec(dllimport)' : ignored on left of 'LeoPlugin' when no variable is declared
#ifdef Q_OS_WIN
#pragma warning( push )
#pragma warning( disable : 4091 )
#endif

#include <Plugin.h>

#ifdef Q_OS_WIN
#pragma warning( pop )
#endif

gpu::PipelinePointer RenderableLeoPolyEntityItem::_pipeline = nullptr;

#ifdef WIN32
/* NOTE: if you want this example to work on Windows with libcurl as a
DLL, you MUST also provide a read callback with CURLOPT_READFUNCTION.
Failing to do so will give you a crash since a DLL may not use the
variable's memory when passed in to it from an app like this. */
static size_t read_callback(void *ptr, size_t size, size_t nmemb, void *stream)
{
    curl_off_t nread;

    size_t retcode = fread(ptr, size, nmemb, (FILE*)stream);

    nread = (curl_off_t)retcode;

    fprintf(stderr, "*** We read %" CURL_FORMAT_CURL_OFF_T
        " bytes from file\n", nread);
    return retcode;
}
#endif
EntityItemPointer RenderableLeoPolyEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    EntityItemPointer entity{ new RenderableLeoPolyEntityItem(entityID) };
    entity->setProperties(properties);
    return entity;
}

RenderableLeoPolyEntityItem::RenderableLeoPolyEntityItem(const EntityItemID& entityItemID) :
LeoPolyEntityItem(entityItemID)
{
}

RenderableLeoPolyEntityItem::~RenderableLeoPolyEntityItem() {
}

bool RenderableLeoPolyEntityItem::findDetailedRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
    bool& keepSearching, OctreeElementPointer& element,
    float& distance, BoxFace& face, glm::vec3& surfaceNormal,
    void** intersectedObject, bool precisionPicking) const
{
    if (!precisionPicking) {
        // just intersect with bounding box
        return true;
    }

    // TODO
    return true;
}

ShapeType RenderableLeoPolyEntityItem::getShapeType() const {
    return SHAPE_TYPE_BOX;
}


// FIXME - this is probably cruft and can go...
void RenderableLeoPolyEntityItem::updateRegistrationPoint(const glm::vec3& value) {
    if (value != _registrationPoint) {
        EntityItem::updateRegistrationPoint(value);
    }
}

// FIXME - for now this is cruft and can probably go... but we will want to think about how physics works with these shapes
bool RenderableLeoPolyEntityItem::isReadyToComputeShape() {
    if (!EntityItem::isReadyToComputeShape()) {
        return false;
    }
    return true;
}

// FIXME - for now this is cruft and can probably go... but we will want to think about how physics works with these shapes
void RenderableLeoPolyEntityItem::computeShapeInfo(ShapeInfo& info) {
    info.setParams(getShapeType(), 0.5f * getDimensions());
    adjustShapeInfoByRegistration(info);
}

void RenderableLeoPolyEntityItem::update(const quint64& now) {
    LeoPolyEntityItem::update(now);
    LeoPolyPlugin::Instance().SculptApp_Frame();
    updateGeometryFromLeoPlugin();
}

EntityItemID RenderableLeoPolyEntityItem::getCurrentlyEditingEntityID() {
    EntityItemID entityUnderSculptID;

    if (LeoPolyPlugin::Instance().CurrentlyUnderEdit.data1 != 0) {
        entityUnderSculptID.data1 = LeoPolyPlugin::Instance().CurrentlyUnderEdit.data1;
        entityUnderSculptID.data2 = LeoPolyPlugin::Instance().CurrentlyUnderEdit.data2;
        entityUnderSculptID.data3 = LeoPolyPlugin::Instance().CurrentlyUnderEdit.data3;
        for (int i = 0; i < 8; i++) {
            entityUnderSculptID.data4[i] = LeoPolyPlugin::Instance().CurrentlyUnderEdit.data4[i];
        }
    }
    return entityUnderSculptID;
}

void RenderableLeoPolyEntityItem::createShaderPipeline() {
    // FIXME - we should use a different shader
    gpu::ShaderPointer vertexShader = gpu::Shader::createVertex(std::string(leoPoly_vert));
    gpu::ShaderPointer pixelShader = gpu::Shader::createPixel(std::string(leoPoly_frag));

    gpu::Shader::BindingSet slotBindings;
    slotBindings.insert(gpu::Shader::Binding(std::string("materialBuffer"), MATERIAL_GPU_SLOT));
    slotBindings.insert(gpu::Shader::Binding(std::string("xMap"), 0));

    gpu::ShaderPointer program = gpu::Shader::createProgram(vertexShader, pixelShader);
    gpu::Shader::makeProgram(*program, slotBindings);

    auto state = std::make_shared<gpu::State>();
    state->setCullMode(gpu::State::CULL_BACK);
    state->setDepthTest(true, true, gpu::LESS_EQUAL);

    _pipeline = gpu::Pipeline::create(program, state);
}

void RenderableLeoPolyEntityItem::render(RenderArgs* args) {
    PerformanceTimer perfTimer("RenderableLeoPolyEntityItem::render");
    assert(getType() == EntityTypes::LeoPoly);
    Q_ASSERT(args->_batch);

    // if we don't have a _modelResource yet, then we can't render...
    if (_modelResource==nullptr) 
    {
        initializeModelResource();
        return;
    }

    // FIXME - this is janky... 
    if (!_mesh || getEntityItemID() == getCurrentlyEditingEntityID()) {
        getMesh();
    }

    model::MeshPointer mesh;
    withReadLock([&] {
        mesh = _mesh;
    });

    if (!_pipeline) {
        createShaderPipeline();
    }

    if (!mesh) {
        return;
    }

    gpu::Batch& batch = *args->_batch;
    batch.setPipeline(_pipeline);

    bool success;
    Transform transform = getTransformToCenter(success);
    if (!success) {
        return;
    }

    // get the bounds of the mesh, so we can scale it into the bounds of the entity
    //int numMeshParts = (int)mesh->getNumParts();
    //auto meshBounds = mesh->evalMeshBound();
    model::Box meshBounds = evalMeshBound(mesh);

    // determine the correct scale to fit mesh into entity bounds, set transform accordingly
    auto entityScale = getScale();
    auto meshBoundsScale = meshBounds.getScale();
    auto fitInBounds = entityScale / meshBoundsScale;
    transform.setScale(fitInBounds);

    // make sure the registration point on the model aligns with the registration point in the entity. 
    auto registrationPoint = getRegistrationPoint(); // vec3(0) to vec3(1) for the entity space
    auto lowestBounds = meshBounds.getMinimum();
    glm::vec3 adjustLowestBounds = ((registrationPoint * meshBoundsScale) + lowestBounds) * -1.0f;
    transform.postTranslate(adjustLowestBounds);
    batch.setModelTransform(transform);
    batch.setInputBuffer(gpu::Stream::POSITION, mesh->getVertexBuffer());
    batch.setIndexBuffer(gpu::UINT32, mesh->getIndexBuffer()._buffer, 0);
    batch.setInputFormat(VertexNormalTexCoord::getVertexFormat());

    for (unsigned int i = 0; i < _materials.size(); i++)
    {
        gpu::TexturePointer actText;
        if (_materials[i].diffuseTextureUrl != "")
        {
            actText = DependencyManager::get<TextureCache>()->getImageTexture(_materials[i].diffuseTextureUrl.c_str());
        }
        if (actText)
        {
            batch.setResourceTexture(0, actText);
        }
        else
        {
            batch.setResourceTexture(0, DependencyManager::get<TextureCache>()->getBlueTexture());
        }

        batch.drawIndexed(gpu::TRIANGLES, (gpu::uint32)_materials[i].numIndices, _materials[i].indexOffset);
    }
}

bool RenderableLeoPolyEntityItem::addToScene(EntityItemPointer self,
                                             const render::ScenePointer& scene,
                                             render::Transaction& transaction) {

    _myItem = scene->allocateID();

    auto renderItem = std::make_shared<LeoPolyPayload>(getThisPointer());
    auto renderData = LeoPolyPayload::Pointer(renderItem);
    auto renderPayload = std::make_shared<LeoPolyPayload::Payload>(renderData);

    render::Item::Status::Getters statusGetters;
    makeEntityItemStatusGetters(getThisPointer(), statusGetters);
    renderPayload->addStatusGetters(statusGetters);

    transaction.resetItem(_myItem, renderPayload);

    return true;
}

void RenderableLeoPolyEntityItem::removeFromScene(EntityItemPointer self,
                                                  const render::ScenePointer& scene,
                                                  render::Transaction& transaction) {
    transaction.removeItem(_myItem);
    render::Item::clearID(_myItem);
}

namespace render {
    template <> const ItemKey payloadGetKey(const LeoPolyPayload::Pointer& payload) {
        return ItemKey::Builder::opaqueShape();
    }

    template <> const Item::Bound payloadGetBound(const LeoPolyPayload::Pointer& payload) {
        if (payload && payload->_owner) {
            auto leoPolyEntity = std::dynamic_pointer_cast<RenderableLeoPolyEntityItem>(payload->_owner);
            bool success;
            auto result = leoPolyEntity->getAABox(success);
            if (!success) {
                return render::Item::Bound();
            }
            return result;
        }
        return render::Item::Bound();
    }

    template <> void payloadRender(const LeoPolyPayload::Pointer& payload, RenderArgs* args) {
        if (args && payload && payload->_owner) {
            payload->_owner->render(args);
        }
    }
}


void RenderableLeoPolyEntityItem::initializeModelResource() {
    // FIXME -- some open questions....
    //   1) what do we do if someone changes the URL while under edit?
    //   2) how do we managed the previous mesh resources associated with the old GeometryResource
    //
    //  _modelResource->getURL() ... this might be useful
    // 
    if (!_leoPolyURL.isEmpty())
    {
        auto modelCache = DependencyManager::get<ModelCache>();
        EntityItemID entityUnderSculptID = getCurrentlyEditingEntityID();
        if (getEntityItemID() == entityUnderSculptID)
        {
            _modelResource = modelCache->getGeometryResource(_leoPolyURL);
            return;
        }
        // use an asset client to upload the asset
        auto assetClient = DependencyManager::get<AssetClient>();
        auto urlPath = _leoPolyURL.toStdString();
        if (std::string::npos != urlPath.find("atp:/"))
        {
            urlPath.erase(0, 5);
        }
        static bool isIdle = true;
        auto assetRequest=assetClient->createRequest(QString(urlPath.c_str()));
        assetRequest->connect(assetRequest, &AssetRequest::finished, assetClient.data(), [=](AssetRequest* request) mutable {
            Q_ASSERT(request->getState() == AssetRequest::Finished);

            if (request->getError() == AssetRequest::Error::NoError) 
            {
                auto data=request->getData();
                
                QFile file("Temp\\" + QString(urlPath.c_str())+".obj");
                file.open(QFile::WriteOnly | QFile::Truncate);
                file.write(data);
                file.close();
                _modelResource = modelCache->getGeometryResource(QFileInfo(file.fileName()).absoluteFilePath());
            }
            else
            {
                _modelResource = modelCache->getGeometryResource(_leoPolyURL);
            }
            request->deleteLater();
            isIdle = true;
        });
        if (isIdle)
        {
            assetRequest->start();
            isIdle = false;
        }
       
    }
}

void RenderableLeoPolyEntityItem::getMesh() {
    EntityItemID entityUnderSculptID = getCurrentlyEditingEntityID();
    _materials.clear();
    // FIXME -- this seems wrong, but It think I'm begining to understand it.
    // getMesh() will only produce a mesh of the current entity is not currently under edit.
    // if the current entity is under edit, then it's assumed that the _mesh will be updated
    // but calling updateGeometryFromLeoPlugin()...
    if (getEntityItemID() == entityUnderSculptID) 
    {
        update(0);
        bool success;
        Transform transform = getTransformToCenter(success);
        if (!success) {
            return;
        }
        withWriteLock([&] {
            // get the bounds of the mesh, so we can scale it into the bounds of the entity
            //int numMeshParts = (int)_mesh->getNumParts();
            //auto meshBounds = mesh->evalMeshBound();
            model::Box meshBounds = evalMeshBound(_mesh);

            // determine the correct scale to fit mesh into entity bounds, set transform accordingly
            auto entityScale = getScale();
            auto meshBoundsScale = meshBounds.getScale();
            auto fitInBounds = entityScale / meshBoundsScale;
            transform.setScale(fitInBounds);

            // make sure the registration point on the model aligns with the registration point in the entity. 
            auto registrationPoint = getRegistrationPoint(); // vec3(0) to vec3(1) for the entity space
            auto lowestBounds = meshBounds.getMinimum();
            glm::vec3 adjustLowestBounds = ((registrationPoint * meshBoundsScale) + lowestBounds) * -1.0f;
            transform.postTranslate(adjustLowestBounds);


            LeoPolyPlugin::Instance().setSculptMeshMatrix(const_cast<float*>(glm::value_ptr(glm::transpose(transform.getMatrix()))));
        });
        return;
    }

    if (!_modelResource || !_modelResource->isLoaded()) {
        // model not yet loaded... can't make a mesh from it yet...
        return;
    }
    LeoPolyPlugin::Instance().CurrentlyUnderEdit.data1 =getID().data1;
    LeoPolyPlugin::Instance().CurrentlyUnderEdit.data2 = getID().data2;
    LeoPolyPlugin::Instance().CurrentlyUnderEdit.data3 = getID().data3;
    LeoPolyPlugin::Instance().CurrentlyUnderEdit.data4 = new unsigned char[8];
    memcpy(LeoPolyPlugin::Instance().CurrentlyUnderEdit.data4, getID().data4, 8 * sizeof(unsigned char));

    importToLeoPoly();
    return;
    //  const GeometryMeshes& getMeshes() const { return *_meshes; }
    auto meshes = _modelResource->getMeshes();

    // FIXME -- in the event that a model has multiple meshes, we should flatten them into a single mesh...
    if (meshes.size() > 1) {
        qDebug() << __FUNCTION__ << "WARNING- model resources with multiple meshes not yet supported...";
    }

    if (meshes.size() < 1) {
        model::MeshPointer emptyMesh(new model::Mesh()); // an empty mesh....
    } else {
        // FIXME- this is a bit of a hack to work around const-ness
        model::MeshPointer copyOfMesh(new model::Mesh());
        std::vector<VertexNormalTexCoord> verticesNormalsMaterials;
        
        for (unsigned int i = 0; i < meshes[0]->getNumVertices(); i++)
        {
            glm::vec3 actVert = glm::vec3(_modelResource->getFBXGeometry().meshes[0].vertices[i]);
            glm::vec3 actNorm = glm::vec3(_modelResource->getFBXGeometry().meshes[0].normals[i]);
            glm::vec2 actTexCoord = glm::vec2(_modelResource->getFBXGeometry().meshes[0].texCoords[i]);
            verticesNormalsMaterials.push_back(VertexNormalTexCoord{ actVert, actNorm, actTexCoord });
        }

        auto vertexBuffer = std::make_shared<gpu::Buffer>(verticesNormalsMaterials.size() * sizeof(VertexNormalTexCoord),
            (gpu::Byte*)verticesNormalsMaterials.data());
        auto vertexBufferPtr = gpu::BufferPointer(vertexBuffer);
        

        gpu::BufferView vertexBufferView(vertexBufferPtr, 0, vertexBuffer->getSize(),
            sizeof(VertexNormalTexCoord),
            gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::XYZ));
        copyOfMesh->setVertexBuffer(vertexBufferView);
        copyOfMesh->addAttribute(gpu::Stream::NORMAL,
            gpu::BufferView(vertexBufferPtr,
            offsetof(VertexNormalTexCoord, normal),
            vertexBufferPtr->getSize(),
            sizeof(VertexNormalTexCoord),
            gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::XYZ)));
        copyOfMesh->addAttribute(gpu::Stream::TEXCOORD0,
            gpu::BufferView(vertexBufferPtr,
            offsetof(VertexNormalTexCoord, texCoord),
            vertexBufferPtr->getSize(),
            sizeof(VertexNormalTexCoord),
            gpu::Element(gpu::VEC2, gpu::FLOAT, gpu::UV)));


        verticesNormalsMaterials.clear();


        std::vector<uint32_t> vecIndices;
        for (int part = 0; part < _modelResource->getFBXGeometry().meshes[0].parts.size(); part++)
        for (unsigned int i = 0; i < meshes[0]->getNumIndices(); i++) 
        {
            vecIndices.push_back(_modelResource->getFBXGeometry().meshes[0].parts[part].triangleIndices[i]);
        }


        auto indexBuffer = std::make_shared<gpu::Buffer>(vecIndices.size() * sizeof(uint32_t), (gpu::Byte*)vecIndices.data());
        auto indexBufferPtr = gpu::BufferPointer(indexBuffer);

        gpu::BufferView indexBufferView(indexBufferPtr, gpu::Element(gpu::SCALAR, gpu::UINT32, gpu::RAW));
        copyOfMesh->setIndexBuffer(indexBufferView);

        vecIndices.clear();

        LeoPlugin::IncomingMaterial defaultMat;
        defaultMat.materialID = "default";
        defaultMat.name = "Default";
        defaultMat.indexOffset = 0;
        defaultMat.numIndices = static_cast<unsigned int>(copyOfMesh->getNumIndices());
        _materials.push_back(defaultMat);
        setMesh(copyOfMesh);
    }
}


void RenderableLeoPolyEntityItem::setUnderSculpting(bool value) {
    // TODO -- how do we want to enable/disable sculpting...
}


void RenderableLeoPolyEntityItem::doExportCurrentState()
{
    std::string uploadPath = "Temp\\";//TODO: Will be replaced
    std::string urlPath = getLeoPolyURL().toStdString();
    const size_t last_slash_idx = urlPath.find_last_of("\\/");
    if (std::string::npos != last_slash_idx)
    {
        urlPath.erase(0, last_slash_idx + 1);
    }
    if (std::string::npos != urlPath.find("atp:/"))
    {
        urlPath.erase(0, 5);
    }
    if (std::string::npos == urlPath.find(".obj"))
    {
        urlPath = urlPath + ".obj";
    }
    LeoPolyPlugin::Instance().SculptApp_exportFile((uploadPath + urlPath).c_str());

    while (LeoPolyPlugin::Instance().getAppState() == LeoPlugin::SculptApp_AppState::APPSTATE_WAIT)
    {
        LeoPolyPlugin::Instance().SculptApp_Frame();
    }
   // doUploadViaATP(urlPath);
}

// This will take the _modelResource and convert it into a "flattened form" that can be used by the LeoPoly DLL
void RenderableLeoPolyEntityItem::importToLeoPoly() {

    // we need to have a _modelResource and make sure it's fully loaded before we can parse out it's contents
    // and flatten it into a LeoPoly friendly format.
    if (_modelResource && _modelResource->isLoaded()) {

        QVector<glm::vec3> vertices;
        QVector<glm::vec3> normals;
        QVector<glm::vec2> texCoords;
        QVector<int> indices;
        QVector<std::string> matStringsPerTriangles;
        QVector<unsigned short> matIndexesPerTriangles;

        auto geometry = _modelResource->getFBXGeometry();


        // FIXME -- use the correct URL from the property
        std::string baseUrl; //  = act->getURL().toString().toStdString().substr(0, act->getURL().toString().toStdString().find_last_of("\\/"));

        std::vector<LeoPlugin::IncomingMaterial> materialsToSend;
        foreach(const FBXMaterial mat, geometry.materials)
        {
            LeoPlugin::IncomingMaterial actMat;

            // FIXME - texture support?
            
            if (!mat.albedoTexture.filename.isEmpty() && mat.albedoTexture.content.isEmpty())
            {
                actMat.diffuseTextureUrl = baseUrl + "//" + mat.albedoTexture.filename.toStdString();
            }
            
            /*if (!mat.normalTexture.filename.isEmpty() && mat.normalTexture.content.isEmpty() &&
            !_textures.contains(mat.normalTexture.filename))
            {

            _texturesURLs.push_back(baseUrl + "\\" + mat.normalTexture.filename.toStdString());
            }
            if (!mat.specularTexture.filename.isEmpty() && mat.specularTexture.content.isEmpty() &&
            !_textures.contains(mat.specularTexture.filename))
            {
            _texturesURLs.push_back(baseUrl + "\\" + mat.specularTexture.filename.toStdString());
            }
            if (!mat.emissiveTexture.filename.isEmpty() && mat.emissiveTexture.content.isEmpty() &&
            !_textures.contains(mat.emissiveTexture.filename))
            {
            _texturesURLs.push_back(baseUrl + "\\" + mat.emissiveTexture.filename.toStdString());
            }*/
            for (int i = 0; i < 3; i++)
            {
                actMat.diffuseColor[i] = mat.diffuseColor[i];
                actMat.emissiveColor[i] = mat.emissiveColor[i];
                actMat.specularColor[i] = mat.specularColor[i];
            }
            actMat.diffuseColor[3] = 1;
            actMat.emissiveColor[3] = 0;
            actMat.specularColor[3] = 0;
            actMat.materialID = mat.materialID.toStdString();
            actMat.name = mat.name.toStdString();
            materialsToSend.push_back(actMat);
        }
        for (auto actmesh : geometry.meshes)
        {
            vertices.append(actmesh.vertices);
            normals.append(actmesh.normals);
            texCoords.append(actmesh.texCoords);
            for (auto subMesh : actmesh.parts)
            {
                int startIndex = indices.size();
                if (subMesh.triangleIndices.size() > 0)
                {
                    indices.append(subMesh.triangleIndices);
                }
                if (subMesh.quadTrianglesIndices.size() > 0)
                {
                    indices.append(subMesh.quadTrianglesIndices);
                }
                else
                    if (subMesh.quadIndices.size() > 0)
                    {
                        assert(subMesh.quadIndices.size() % 4 == 0);
                        for (int i = 0; i < subMesh.quadIndices.size() / 4; i++)
                        {
                            indices.push_back(subMesh.quadIndices[i * 4]);
                            indices.push_back(subMesh.quadIndices[i * 4 + 1]);
                            indices.push_back(subMesh.quadIndices[i * 4 + 2]);

                            indices.push_back(subMesh.quadIndices[i * 4 + 3]);
                            indices.push_back(subMesh.quadIndices[i * 4]);
                            indices.push_back(subMesh.quadIndices[i * 4 + 1]);
                        }
                    }
                for (int i = 0; i < (indices.size() - startIndex) / 3; i++)
                {
                    matStringsPerTriangles.push_back(subMesh.materialID.toStdString());
                }

            }
        }
        float* verticesFlattened = new float[vertices.size() * 3];
        float *normalsFlattened = new float[normals.size() * 3];
        float *texCoordsFlattened = new float[texCoords.size() * 2];
        int *indicesFlattened = new int[indices.size()];
        for (int i = 0; i < vertices.size(); i++)
        {

            verticesFlattened[i * 3] = vertices[i].x;
            verticesFlattened[i * 3 + 1] = vertices[i].y;
            verticesFlattened[i * 3 + 2] = vertices[i].z;

            normalsFlattened[i * 3] = normals[i].x;
            normalsFlattened[i * 3 + 1] = normals[i].y;
            normalsFlattened[i * 3 + 2] = normals[i].z;

            texCoordsFlattened[i * 2] = texCoords[i].x;
            texCoordsFlattened[i * 2 + 1] = texCoords[i].y;
        }
        for (int i = 0; i < indices.size(); i++)
        {
            indicesFlattened[i] = indices[i];
        }
        matIndexesPerTriangles.resize(matStringsPerTriangles.size());
        for (unsigned int matInd = 0; matInd < materialsToSend.size(); matInd++)
        {
            for (int i = 0; i < matStringsPerTriangles.size(); i++)
            {
                if (matStringsPerTriangles[i] == materialsToSend[matInd].materialID)
                {
                    matIndexesPerTriangles[i] = matInd;
                }
            }
        }

         
        // Creates a sculptable mesh for the Leoengine from the given
        //   vertices array
        //   number of vertices passed
        //   indices array
        //   number of indices passed
        //   normals array
        //   number of normals passed
        //   Texture coordinates array
        //   number of texcoords passed
        //        world matrix??
        //   Materials
        //   Indices connecting the triangles
        // LEOPLUGIN_DLL_API void importFromRawData(
        //         float* vertices, unsigned int numVertices, int* indices, unsigned int numIndices, 
        //         float* normals, unsigned int numNormals, float* texCoords, unsigned int numTexCoords, 
        //         float worldMat[16], std::vector<IncomingMaterial> metrials, 
        //         std::vector<unsigned short> triangleMatInds);


        LeoPolyPlugin::Instance().importFromRawData(
            verticesFlattened, vertices.size(),
            indicesFlattened, indices.size(),
            normalsFlattened, normals.size(),
            texCoordsFlattened, texCoords.size(),
            const_cast<float*>(glm::value_ptr(glm::transpose(getTransform().getMatrix()))),
            materialsToSend, matIndexesPerTriangles.toStdVector());
        delete[] verticesFlattened;
        delete[] indicesFlattened;
        delete[] normalsFlattened;
        delete[] texCoordsFlattened;
    }

}

// If the entity is currently under edit with LeoPoly, this method will ask the DLL for it's current mesh state
// and re-create our mesh for rendering...
void RenderableLeoPolyEntityItem::updateGeometryFromLeoPlugin() {

    // get the vertices, normals, and indices from LeoPoly
    float* vertices, *normals, *texCoords;
    int *indices;
    unsigned int numVertices = 0;
    unsigned int numNormals = 0;
    unsigned int numIndices = 0;
    unsigned int numMaterials = 0;
    LeoPolyPlugin::Instance().getSculptMeshNumberDatas(numVertices, numIndices, numNormals, numMaterials);
    vertices = new float[numVertices * 3];
    normals = new float[numNormals * 3];
    texCoords = new float[numVertices * 2];
    indices = new int[numIndices];
    LeoPolyPlugin::Instance().getRawSculptMeshData(vertices, indices, normals, texCoords);

    LeoPlugin::IncomingMaterial* materials= new LeoPlugin::IncomingMaterial[numMaterials];
    LeoPolyPlugin::Instance().getCurrentUsedMaterials(materials);
    for (unsigned int i = 0; i < numMaterials; i++)
    {
        LeoPlugin::IncomingMaterial actMat = LeoPlugin::IncomingMaterial(materials[i]);
        _materials.push_back(actMat);
    }
    delete[] materials;

    // Create a model::Mesh from the flattened mesh from LeoPoly
    model::MeshPointer mesh(new model::Mesh());
    // bool needsMaterialLibrary = false;

    std::vector<VertexNormalTexCoord> verticesNormalsMaterials;
    QVector<glm::vec3> verticesss;
    for (unsigned int i = 0; i < numVertices; i++)
    {
        glm::vec3 actVert = glm::vec3(vertices[i * 3], vertices[i * 3 + 1], vertices[i * 3 + 2]);
        glm::vec3 actNorm = glm::vec3(normals[i * 3], normals[i * 3 + 1], normals[i * 3 + 2]);
        glm::vec2 actTexCoord = glm::vec2(texCoords[i * 2], texCoords[i * 2 + 1]);
        verticesNormalsMaterials.push_back(VertexNormalTexCoord{ actVert, actNorm, actTexCoord });
        verticesss.push_back(actVert);
    }
    auto diffs = getDiffFromPreviousState(verticesss);
    if (diffs.size() > 0)
    {
#ifdef ONE_DIFF_FILE
        QFile file("Temp\\" + QString("diff.txt"));
#else
        static int number = 0;
        QFile file("Temp\\" + QString(std::to_string(number).c_str()) + QString("diff.txt"));
        number++;
#endif
        file.open(QFile::WriteOnly);
        for (auto it : diffs)
        {
            file.write(("Index: "+std::to_string(it.index) + " Type: " + std::to_string(it.type)).c_str());
            file.write((" X: " + std::to_string(it.newValue.x) + " Y: " + std::to_string(it.newValue.y) + " Z: " + std::to_string(it.newValue.z)+"\n").c_str());
        }
        file.close();

    }
    auto vertexBuffer = std::make_shared<gpu::Buffer>(verticesNormalsMaterials.size() * sizeof(VertexNormalTexCoord),
                                                        (gpu::Byte*)verticesNormalsMaterials.data());
    auto vertexBufferPtr = gpu::BufferPointer(vertexBuffer);

    gpu::BufferView vertexBufferView(vertexBufferPtr, 0, 
        vertexBufferPtr->getSize(),
        sizeof(VertexNormalTexCoord),
        gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::XYZ));

    mesh->setVertexBuffer(vertexBufferView);

    mesh->addAttribute(gpu::Stream::NORMAL,
        gpu::BufferView(vertexBufferPtr,
        offsetof(VertexNormalTexCoord, normal),
        vertexBufferPtr->getSize(),
        sizeof(VertexNormalTexCoord),
        gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::XYZ)));
    mesh->addAttribute(gpu::Stream::TEXCOORD0,
        gpu::BufferView(vertexBufferPtr,
        offsetof(VertexNormalTexCoord, texCoord),
        vertexBufferPtr->getSize(),
        sizeof(VertexNormalTexCoord),
        gpu::Element(gpu::VEC2, gpu::FLOAT, gpu::UV)));



    std::vector<uint32_t> vecIndices;

    for (unsigned int i = 0; i < numIndices; i++) {
        vecIndices.push_back(indices[i]);
    }


    auto indexBuffer = std::make_shared<gpu::Buffer>(vecIndices.size() * sizeof(uint32_t), (gpu::Byte*)vecIndices.data());
    auto indexBufferPtr = gpu::BufferPointer(indexBuffer);

    gpu::BufferView indexBufferView(indexBufferPtr, gpu::Element(gpu::SCALAR, gpu::UINT32, gpu::RAW));
    mesh->setIndexBuffer(indexBufferView);

    vecIndices.clear();

    // get the bounds of the mesh, so we can scale it into the bounds of the entity
    auto properties = getProperties();
    withWriteLock([&] {
        if (_mesh)
        {
            model::Box meshBounds = evalMeshBound(_mesh);
            model::Box meshBoundsnew = evalMeshBound(mesh);
            
            if (meshBoundsnew.getScale() != meshBounds.getScale())
            {
                properties.setLastEdited(usecTimestampNow()); // we must set the edit time since we're editing it
                properties.setDimensions(properties.getDimensions()*(meshBoundsnew.getScale() / meshBounds.getScale()));
                QMetaObject::invokeMethod(DependencyManager::get<EntityScriptingInterface>().data(), "editEntity",
                    Qt::QueuedConnection,
                    Q_ARG(QUuid, getEntityItemID()),
                    Q_ARG(EntityItemProperties, properties));
            }
        }
    });
    
    if (_materials.size() == 0)
    {
        LeoPlugin::IncomingMaterial defaultMat;
        defaultMat.materialID = "default";
        defaultMat.name = "Default";
        defaultMat.indexOffset = 0;
        defaultMat.numIndices = static_cast<unsigned int>(mesh->getNumIndices());
        _materials.push_back(defaultMat);
    }
    setMesh(mesh);

    delete[] texCoords;
    delete[] vertices;
    delete[] normals;
    delete[] indices;
}


void RenderableLeoPolyEntityItem::setMesh(model::MeshPointer mesh) {
    // this catches the payload from getMesh
    withWriteLock([&] {
        _mesh = mesh;
    });
}


void RenderableLeoPolyEntityItem::sendToLeoEngine(ModelPointer model)
{
    QVector<glm::vec3> vertices;
    QVector<glm::vec3> normals;
    QVector<glm::vec2> texCoords;
    QVector<int> indices;
    QVector<std::string> matStringsPerTriangles;
    QVector<unsigned short> matIndexesPerTriangles;
    //ModelPointer act = getModel(_myRenderer);
    auto geometry = model->getFBXGeometry();
    
    std::string baseUrl = model->getURL().toString().toStdString().substr(0, model->getURL().toString().toStdString().find_last_of("\\/"));
    if (baseUrl.find("file:") != std::string::npos || baseUrl.find("http:") != std::string::npos)
    {
        baseUrl = std::string(baseUrl.begin() + 8, baseUrl.end());
    }
    std::vector<LeoPlugin::IncomingMaterial> materialsToSend;
    foreach(const FBXMaterial mat, geometry.materials)
    {
        LeoPlugin::IncomingMaterial actMat;
        if (!mat.albedoTexture.filename.isEmpty())
        {
            actMat.diffuseTextureUrl = baseUrl + "//" + mat.albedoTexture.filename.toStdString();
        }

        memcpy(actMat.diffuseColor,glm::value_ptr( mat.diffuseColor), 3 * sizeof (float));
        memcpy(actMat.emissiveColor, glm::value_ptr(mat.emissiveColor), 3 * sizeof(float));
        memcpy(actMat.specularColor, glm::value_ptr(mat.specularColor), 3 * sizeof(float));
       
        actMat.diffuseColor[3] = 1;
        actMat.emissiveColor[3] = 0;
        actMat.specularColor[3] = 0;
        actMat.materialID = mat.materialID.toStdString();
        actMat.name = mat.name.toStdString();
        materialsToSend.push_back(actMat);
    }
    for (auto actmesh : geometry.meshes)
    {
        vertices.append(actmesh.vertices);
        normals.append(actmesh.normals);
        texCoords.append(actmesh.texCoords);
        for (auto subMesh : actmesh.parts)
        {
            int startIndex = indices.size();
            if (subMesh.triangleIndices.size() > 0)
            {
                indices.append(subMesh.triangleIndices);
            }
            if (subMesh.quadTrianglesIndices.size() > 0)
            {
                indices.append(subMesh.quadTrianglesIndices);
            }
            else
                if (subMesh.quadIndices.size() > 0)
                {
                    assert(subMesh.quadIndices.size() % 4 == 0);
                    for (int i = 0; i < subMesh.quadIndices.size() / 4; i++)
                    {
                        indices.push_back(subMesh.quadIndices[i * 4]);
                        indices.push_back(subMesh.quadIndices[i * 4 + 1]);
                        indices.push_back(subMesh.quadIndices[i * 4 + 2]);

                        indices.push_back(subMesh.quadIndices[i * 4 + 3]);
                        indices.push_back(subMesh.quadIndices[i * 4]);
                        indices.push_back(subMesh.quadIndices[i * 4 + 1]);
                    }
                }
            for (int i = 0; i < (indices.size() - startIndex) / 3; i++)
            {
                matStringsPerTriangles.push_back(subMesh.materialID.toStdString());
            }

        }
    }
    float* verticesFlattened = new float[vertices.size() * 3];
    float *normalsFlattened = new float[normals.size() * 3];
    float *texCoordsFlattened = new float[texCoords.size() * 2];
    int *indicesFlattened = new int[indices.size()];
    for (int i = 0; i < vertices.size(); i++)
    {

        verticesFlattened[i * 3] = vertices[i].x;
        verticesFlattened[i * 3 + 1] = vertices[i].y;
        verticesFlattened[i * 3 + 2] = vertices[i].z;

        normalsFlattened[i * 3] = normals[i].x;
        normalsFlattened[i * 3 + 1] = normals[i].y;
        normalsFlattened[i * 3 + 2] = normals[i].z;

        texCoordsFlattened[i * 2] = texCoords[i].x;
        texCoordsFlattened[i * 2 + 1] = texCoords[i].y;
    }
    for (int i = 0; i < indices.size(); i++)
    {
        indicesFlattened[i] = indices[i];
    }
    matIndexesPerTriangles.resize(matStringsPerTriangles.size());
    for (unsigned int matInd = 0; matInd < materialsToSend.size(); matInd++)
    {
        for (int i = 0; i < matStringsPerTriangles.size(); i++)
        {
            if (matStringsPerTriangles[i] == materialsToSend[matInd].materialID)
            {
                matIndexesPerTriangles[i] = matInd;
            }
        }
    }
    Transform transform(getTransform());

    model::Box meshBounds;
    for (int i = 0; i < geometry.meshes.size(); i++)
    {
        meshBounds +=evalMeshBound(geometry.meshes[i]._mesh);
    }
    // determine the correct scale to fit mesh into entity bounds, set transform accordingly
    auto entityScale = getScale();
    auto meshBoundsScale = meshBounds.getScale();
    auto fitInBounds = entityScale / meshBoundsScale;
    setScale(fitInBounds);
    transform.setScale(fitInBounds);

    // make sure the registration point on the model aligns with the registration point in the entity. 
    auto registrationPoint = getRegistrationPoint(); // vec3(0) to vec3(1) for the entity space
    auto lowestBounds = meshBounds.getMinimum();
    glm::vec3 adjustLowestBounds = ((registrationPoint * meshBoundsScale) + lowestBounds) * -1.0f;
    transform.postTranslate(adjustLowestBounds);

    LeoPolyPlugin::Instance().importFromRawData(verticesFlattened, vertices.size(), indicesFlattened, indices.size(), normalsFlattened, normals.size(),
        texCoordsFlattened, texCoords.size(), const_cast<float*>(glm::value_ptr(glm::transpose(transform.getMatrix()))), materialsToSend, matIndexesPerTriangles.toStdVector());
    delete[] verticesFlattened;
    delete[] indicesFlattened;
    delete[] normalsFlattened;
    delete[] texCoordsFlattened;
}

model::Box RenderableLeoPolyEntityItem::evalMeshBound(const model::MeshPointer mesh)
{
    model::Box meshBounds;
    if (mesh)
    {
        auto vertices = mesh->getVertexBuffer();
        gpu::BufferView::Iterator<const VertexNormalTexCoord> vertexItr = vertices.cbegin<const VertexNormalTexCoord>();
        while (vertexItr != vertices.cend<const VertexNormalTexCoord>()) {
            meshBounds += (*vertexItr).vertex;
            ++vertexItr;
        }
    }
    return meshBounds;
}

std::vector<RenderableLeoPolyEntityItem::VertexStateChange> RenderableLeoPolyEntityItem::getDiffFromPreviousState(QVector<glm::vec3> newVerts)const
{
    std::vector<RenderableLeoPolyEntityItem::VertexStateChange> diffs;
   
    withWriteLock([&] {
        if (_mesh && _mesh->hasVertexData())
        {
            auto vertices = _mesh->getVertexBuffer();
            uint oldVertNum = ((uint)vertices._size) / sizeof(VertexNormalTexCoord);
            uint newVertNum = (uint)newVerts.size();
            gpu::BufferView::Iterator<const VertexNormalTexCoord> vertexItr = vertices.cbegin<const VertexNormalTexCoord>();
            uint i = 0;
            for (; i < oldVertNum; i++)
            {
                if (i < newVertNum && (*vertexItr).vertex != newVerts[i])
                {
                    VertexStateChange actChange;
                    actChange.type = VertexStateChange::VertexStateChangeType::Modified;
                    actChange.index = i;
                    actChange.newValue = newVerts[i];
                    diffs.push_back(actChange);
                }
                else if (i >= newVertNum)
                {
                    VertexStateChange actChange;
                    actChange.type = VertexStateChange::VertexStateChangeType::Deleted;
                    actChange.index = i;
                    diffs.push_back(actChange);
                }
                ++vertexItr;
            }
            if (newVertNum > oldVertNum)
            {
				for (; i < newVertNum; i++)
				{
					VertexStateChange actChange;
					actChange.type = VertexStateChange::VertexStateChangeType::Added;
					actChange.index = i;
					actChange.newValue = newVerts[i];
					diffs.push_back(actChange);
				}
            }
        }
    });
    return diffs;
}

bool RenderableLeoPolyEntityItem::doUploadViaFTP(std::string fileName)
{
#ifdef WIN32
    CURL *curl;
    CURLcode res;
    FILE *hd_src;
    struct stat file_info;
    

#ifdef LeoFTP_Inside
    // convert URL and username and password to connect to remote server
    std::string urlPath = "ftp://Anonymus@192.168.8.8/" + fileName; //"ftp://86.101.231.173//" + fileName;
#else
    std::string urlPath = "ftp://Anonymus@86.101.231.173/" + fileName; //"ftp://86.101.231.173//" + fileName;
#endif
    fileName = "Temp\\" + fileName;
    // stat the local file
    if (stat(fileName.c_str(), &file_info)){
        qDebug() << "couldn't open file\n";
        return false;
    }

    char *url = new char[urlPath.size() + 1];
    std::copy(urlPath.begin(), urlPath.end(), url);
    url[urlPath.size()] = '\0';

    std::string userAndPassString = "Anonymus";
    char* usernameAndPassword = new char[userAndPassString.size() + 1];
    std::copy(userAndPassString.begin(), userAndPassString.end(), usernameAndPassword);
    usernameAndPassword[userAndPassString.size()] = '\0';

    // get the file to open
    hd_src = fopen(fileName.c_str(), "rb");

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    if (curl){

        /* specify target */
        curl_easy_setopt(curl, CURLOPT_URL, url);
#ifdef LeoFTP_Inside
        curl_easy_setopt(curl, CURLOPT_PORT, 21);
#else
        curl_easy_setopt(curl, CURLOPT_PORT, 2121);
#endif
        //curl_easy_setopt(curl, CURLOPT_USERPWD, usernameAndPassword);
        curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
        /* we want to use our own read function */
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);

        /* enable uploading */
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 2L);

        /* now specify which file to upload */
        curl_easy_setopt(curl, CURLOPT_READDATA, hd_src);

        /* Now run off and do what you've been told! */
        res = curl_easy_perform(curl);

        if (res != CURLE_OK){
            qDebug()<<"Upload file failed!Error code:"<<std::to_string(res).c_str();
            delete url;
            delete usernameAndPassword;
            return false;
        }
        curl_easy_cleanup(curl);
    }
    fclose(hd_src);

    delete url;
    delete usernameAndPassword;
#endif
    return true;
}
