//
//  LeoPolyObjectEntityItem.cpp
//  libraries/entities/src
//
//  Created by Seth Alves on 5/11/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#include <QByteArray>
#include <QDebug>
#include <QWriteLocker>

#include <ByteCountCoding.h>

#include "EntitiesLogging.h"
#include "EntityItemProperties.h"
#include "EntityTree.h"
#include "EntityTreeElement.h"
#include "LeoPolyObjectEntityItem.h"

const glm::vec3 LeoPolyObjectEntityItem::DEFAULT_VOXEL_VOLUME_SIZE = glm::vec3(32, 32, 32);
const float LeoPolyObjectEntityItem::MAX_VOXEL_DIMENSION = 128.0f;
const QByteArray LeoPolyObjectEntityItem::DEFAULT_VOXEL_DATA(LeoPolyObjectEntityItem::makeEmptyVoxelData());
const LeoPolyObjectEntityItem::PolyVoxSurfaceStyle LeoPolyObjectEntityItem::DEFAULT_VOXEL_SURFACE_STYLE =
    LeoPolyObjectEntityItem::SURFACE_EDGED_CUBIC;
const QString LeoPolyObjectEntityItem::DEFAULT_X_TEXTURE_URL = QString("");
const QString LeoPolyObjectEntityItem::DEFAULT_Y_TEXTURE_URL = QString("");
const QString LeoPolyObjectEntityItem::DEFAULT_Z_TEXTURE_URL = QString("");

EntityItemPointer LeoPolyObjectEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    EntityItemPointer entity { new LeoPolyObjectEntityItem(entityID) };
    entity->setProperties(properties);
    return entity;
}

QByteArray LeoPolyObjectEntityItem::makeEmptyVoxelData(quint16 voxelXSize, quint16 voxelYSize, quint16 voxelZSize) {
    int rawSize = voxelXSize * voxelYSize * voxelZSize;

    QByteArray uncompressedData = QByteArray(rawSize, '\0');
    QByteArray newVoxelData;
    QDataStream writer(&newVoxelData, QIODevice::WriteOnly | QIODevice::Truncate);
    writer << voxelXSize << voxelYSize << voxelZSize;

    QByteArray compressedData = qCompress(uncompressedData, 9);
    writer << compressedData;

    return newVoxelData;
}

LeoPolyObjectEntityItem::LeoPolyObjectEntityItem(const EntityItemID& entityItemID) :
    EntityItem(entityItemID),
    _voxelVolumeSize(LeoPolyObjectEntityItem::DEFAULT_VOXEL_VOLUME_SIZE),
    _LeoPolyURLData(LeoPolyObjectEntityItem::DEFAULT_VOXEL_DATA),
    _LeoPolyURLDataDirty(true),
    _voxelSurfaceStyle(LeoPolyObjectEntityItem::DEFAULT_VOXEL_SURFACE_STYLE),
    _xTextureURL(LeoPolyObjectEntityItem::DEFAULT_X_TEXTURE_URL),
    _yTextureURL(LeoPolyObjectEntityItem::DEFAULT_Y_TEXTURE_URL),
    _zTextureURL(LeoPolyObjectEntityItem::DEFAULT_Z_TEXTURE_URL) {
    _type = EntityTypes::LeoPolyObject;
}

void LeoPolyObjectEntityItem::setVoxelVolumeSize(glm::vec3 voxelVolumeSize) {
    withWriteLock([&] {
        assert((int)_voxelVolumeSize.x == _voxelVolumeSize.x);
        assert((int)_voxelVolumeSize.y == _voxelVolumeSize.y);
        assert((int)_voxelVolumeSize.z == _voxelVolumeSize.z);

        _voxelVolumeSize = glm::vec3(roundf(voxelVolumeSize.x), roundf(voxelVolumeSize.y), roundf(voxelVolumeSize.z));
        if (_voxelVolumeSize.x < 1) {
            qDebug() << "LeoPolyObjectEntityItem::setVoxelVolumeSize clamping x of" << _voxelVolumeSize.x << "to 1";
            _voxelVolumeSize.x = 1;
        }
        if (_voxelVolumeSize.x > MAX_VOXEL_DIMENSION) {
            qDebug() << "LeoPolyObjectEntityItem::setVoxelVolumeSize clamping x of" << _voxelVolumeSize.x << "to max";
            _voxelVolumeSize.x = MAX_VOXEL_DIMENSION;
        }

        if (_voxelVolumeSize.y < 1) {
            qDebug() << "LeoPolyObjectEntityItem::setVoxelVolumeSize clamping y of" << _voxelVolumeSize.y << "to 1";
            _voxelVolumeSize.y = 1;
        }
        if (_voxelVolumeSize.y > MAX_VOXEL_DIMENSION) {
            qDebug() << "LeoPolyObjectEntityItem::setVoxelVolumeSize clamping y of" << _voxelVolumeSize.y << "to max";
            _voxelVolumeSize.y = MAX_VOXEL_DIMENSION;
        }

        if (_voxelVolumeSize.z < 1) {
            qDebug() << "LeoPolyObjectEntityItem::setVoxelVolumeSize clamping z of" << _voxelVolumeSize.z << "to 1";
            _voxelVolumeSize.z = 1;
        }
        if (_voxelVolumeSize.z > MAX_VOXEL_DIMENSION) {
            qDebug() << "LeoPolyObjectEntityItem::setVoxelVolumeSize clamping z of" << _voxelVolumeSize.z << "to max";
            _voxelVolumeSize.z = MAX_VOXEL_DIMENSION;
        }
    });
}

glm::vec3 LeoPolyObjectEntityItem::getVoxelVolumeSize() const {
    glm::vec3 voxelVolumeSize;
    withReadLock([&] {
        voxelVolumeSize = _voxelVolumeSize;
    });
    return voxelVolumeSize;
}


EntityItemProperties LeoPolyObjectEntityItem::getProperties(EntityPropertyFlags desiredProperties) const {
    EntityItemProperties properties = EntityItem::getProperties(desiredProperties); // get the properties from our base class

    COPY_ENTITY_PROPERTY_TO_PROPERTIES(xTextureURL, getXTextureURL);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(yTextureURL, getYTextureURL);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(zTextureURL, getZTextureURL);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(xNNeighborID, getXNNeighborID);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(yNNeighborID, getYNNeighborID);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(zNNeighborID, getZNNeighborID);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(xPNeighborID, getXPNeighborID);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(yPNeighborID, getYPNeighborID);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(zPNeighborID, getZPNeighborID);

    return properties;
}

bool LeoPolyObjectEntityItem::setProperties(const EntityItemProperties& properties) {
    bool somethingChanged = EntityItem::setProperties(properties); // set the properties in our base class

    SET_ENTITY_PROPERTY_FROM_PROPERTIES(xTextureURL, setXTextureURL);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(yTextureURL, setYTextureURL);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(zTextureURL, setZTextureURL);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(xNNeighborID, setXNNeighborID);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(yNNeighborID, setYNNeighborID);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(zNNeighborID, setZNNeighborID);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(xPNeighborID, setXPNeighborID);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(yPNeighborID, setYPNeighborID);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(zPNeighborID, setZPNeighborID);

    if (somethingChanged) {
        bool wantDebug = false;
        if (wantDebug) {
            uint64_t now = usecTimestampNow();
            int elapsed = now - getLastEdited();
            qCDebug(entities) << "LeoPolyObjectEntityItem::setProperties() AFTER update... edited AGO=" << elapsed <<
                "now=" << now << " getLastEdited()=" << getLastEdited();
        }
        setLastEdited(properties._lastEdited);
    }
    return somethingChanged;
}

int LeoPolyObjectEntityItem::readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead, 
                                                        ReadBitstreamToTreeParams& args,
                                                        EntityPropertyFlags& propertyFlags, bool overwriteLocalData,
                                                        bool& somethingChanged) {

    int bytesRead = 0;
    const unsigned char* dataAt = data;

    READ_ENTITY_PROPERTY(PROP_VOXEL_VOLUME_SIZE, glm::vec3, setVoxelVolumeSize);
    READ_ENTITY_PROPERTY(PROP_VOXEL_DATA, QByteArray, setLeoPolyURLData);
    READ_ENTITY_PROPERTY(PROP_VOXEL_SURFACE_STYLE, uint16_t, setVoxelSurfaceStyle);
    READ_ENTITY_PROPERTY(PROP_X_TEXTURE_URL, QString, setXTextureURL);
    READ_ENTITY_PROPERTY(PROP_Y_TEXTURE_URL, QString, setYTextureURL);
    READ_ENTITY_PROPERTY(PROP_Z_TEXTURE_URL, QString, setZTextureURL);
    READ_ENTITY_PROPERTY(PROP_X_N_NEIGHBOR_ID, EntityItemID, setXNNeighborID);
    READ_ENTITY_PROPERTY(PROP_Y_N_NEIGHBOR_ID, EntityItemID, setYNNeighborID);
    READ_ENTITY_PROPERTY(PROP_Z_N_NEIGHBOR_ID, EntityItemID, setZNNeighborID);
    READ_ENTITY_PROPERTY(PROP_X_P_NEIGHBOR_ID, EntityItemID, setXPNeighborID);
    READ_ENTITY_PROPERTY(PROP_Y_P_NEIGHBOR_ID, EntityItemID, setYPNeighborID);
    READ_ENTITY_PROPERTY(PROP_Z_P_NEIGHBOR_ID, EntityItemID, setZPNeighborID);

    return bytesRead;
}


// TODO: eventually only include properties changed since the params.lastViewFrustumSent time
EntityPropertyFlags LeoPolyObjectEntityItem::getEntityProperties(EncodeBitstreamParams& params) const {
    EntityPropertyFlags requestedProperties = EntityItem::getEntityProperties(params);
    requestedProperties += PROP_VOXEL_VOLUME_SIZE;
    requestedProperties += PROP_VOXEL_DATA;
    requestedProperties += PROP_VOXEL_SURFACE_STYLE;
    requestedProperties += PROP_X_TEXTURE_URL;
    requestedProperties += PROP_Y_TEXTURE_URL;
    requestedProperties += PROP_Z_TEXTURE_URL;
    requestedProperties += PROP_X_N_NEIGHBOR_ID;
    requestedProperties += PROP_Y_N_NEIGHBOR_ID;
    requestedProperties += PROP_Z_N_NEIGHBOR_ID;
    requestedProperties += PROP_X_P_NEIGHBOR_ID;
    requestedProperties += PROP_Y_P_NEIGHBOR_ID;
    requestedProperties += PROP_Z_P_NEIGHBOR_ID;
    return requestedProperties;
}

void LeoPolyObjectEntityItem::appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params,
                                           EntityTreeElementExtraEncodeData* modelTreeElementExtraEncodeData,
                                           EntityPropertyFlags& requestedProperties,
                                           EntityPropertyFlags& propertyFlags,
                                           EntityPropertyFlags& propertiesDidntFit,
                                           int& propertyCount,
                                           OctreeElement::AppendState& appendState) const {
    bool successPropertyFits = true;

    APPEND_ENTITY_PROPERTY(PROP_VOXEL_VOLUME_SIZE, getVoxelVolumeSize());
    APPEND_ENTITY_PROPERTY(PROP_VOXEL_DATA, getLeoPolyURLData());
    APPEND_ENTITY_PROPERTY(PROP_VOXEL_SURFACE_STYLE, (uint16_t) getVoxelSurfaceStyle());
    APPEND_ENTITY_PROPERTY(PROP_X_TEXTURE_URL, getXTextureURL());
    APPEND_ENTITY_PROPERTY(PROP_Y_TEXTURE_URL, getYTextureURL());
    APPEND_ENTITY_PROPERTY(PROP_Z_TEXTURE_URL, getZTextureURL());
    APPEND_ENTITY_PROPERTY(PROP_X_N_NEIGHBOR_ID, getXNNeighborID());
    APPEND_ENTITY_PROPERTY(PROP_Y_N_NEIGHBOR_ID, getYNNeighborID());
    APPEND_ENTITY_PROPERTY(PROP_Z_N_NEIGHBOR_ID, getZNNeighborID());
    APPEND_ENTITY_PROPERTY(PROP_X_P_NEIGHBOR_ID, getXPNeighborID());
    APPEND_ENTITY_PROPERTY(PROP_Y_P_NEIGHBOR_ID, getYPNeighborID());
    APPEND_ENTITY_PROPERTY(PROP_Z_P_NEIGHBOR_ID, getZPNeighborID());
}

void LeoPolyObjectEntityItem::debugDump() const {
    quint64 now = usecTimestampNow();
    qCDebug(entities) << "   POLYVOX EntityItem id:" << getEntityItemID() << "---------------------------------------------";
    qCDebug(entities) << "            position:" << debugTreeVector(getPosition());
    qCDebug(entities) << "          dimensions:" << debugTreeVector(getDimensions());
    qCDebug(entities) << "       getLastEdited:" << debugTime(getLastEdited(), now);
}

void LeoPolyObjectEntityItem::setLeoPolyURLData(QByteArray LeoPolyURLData) {
    withWriteLock([&] {
        _LeoPolyURLData = LeoPolyURLData;
        _LeoPolyURLDataDirty = true;
    });
}

const QByteArray LeoPolyObjectEntityItem::getLeoPolyURLData() const {
    QByteArray voxelDataCopy;
    withReadLock([&] {
        voxelDataCopy = _LeoPolyURLData;
    });
    return voxelDataCopy;
}
