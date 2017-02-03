//
//  LeoPolyEntityItem.h
//  libraries/entities/src
//
//  Created by Seth Alves on 2016-11-5.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_LeoPolyEntityItem_h
#define hifi_LeoPolyEntityItem_h

#include "EntityItem.h"

class Model;
using ModelPointer = std::shared_ptr<Model>;

class LeoPolyEntityItem : public EntityItem {
 public:
    static EntityItemPointer factory(const EntityItemID& entityID, const EntityItemProperties& properties);

    LeoPolyEntityItem(const EntityItemID& entityItemID);

    ALLOW_INSTANTIATION // This class can be instantiated

        virtual void setLeoPolyURL(QString leoPolyURL) { _leoPolyURL = leoPolyURL; }
    QString getLeoPolyURL() const {  return _leoPolyURL; }

    virtual void setLeoPolyModelVersion(QUuid value) { _modelVersion = value; /*if (_needReload)update(usecTimestampNow());TEMPORARY DISABLED*/ }
    QUuid getLeoPolyModelVersion() const 
    {
        if (!_modelVersion.isNull())
        return _modelVersion; 
        return QUuid();
    }

    virtual void setLeoPolyControllerPos(glm::vec3 pos){ _leoPolyControllerPos = pos; }
    glm::vec3 getLeoPolyControllerPos()const{ return _leoPolyControllerPos; }

    virtual void setLeoPolyControllerRot(glm::quat rot){ _leoPolyControllerRot = rot; }
    glm::quat getLeoPolyControllerRot()const{ return _leoPolyControllerRot; }

    virtual void setLeoPolyTriggerState(float value){ _leoPolyTriggerState = value; }
    float getLeoPolyTriggerState()const{ return _leoPolyTriggerState; }

    // methods for getting/setting all properties of an entity
    virtual EntityItemProperties getProperties(EntityPropertyFlags desiredProperties = EntityPropertyFlags()) const override;
    virtual bool setProperties(const EntityItemProperties& properties) override;

    // TODO: eventually only include properties changed since the params.lastViewFrustumSent time
    virtual EntityPropertyFlags getEntityProperties(EncodeBitstreamParams& params) const override;

    virtual void appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params,
                                    EntityTreeElementExtraEncodeDataPointer modelTreeElementExtraEncodeData,
                                    EntityPropertyFlags& requestedProperties,
                                    EntityPropertyFlags& propertyFlags,
                                    EntityPropertyFlags& propertiesDidntFit,
                                    int& propertyCount,
                                    OctreeElement::AppendState& appendState) const override;

    virtual int readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead,
                                                 ReadBitstreamToTreeParams& args,
                                                 EntityPropertyFlags& propertyFlags, bool overwriteLocalData,
                                                 bool& somethingChanged) override;

    // never have a ray intersection pick a LeoPolyEntityItem.
    virtual bool supportsDetailedRayIntersection() const override { return true; }
    virtual bool findDetailedRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                                             bool& keepSearching, OctreeElementPointer& element, float& distance,
                                             BoxFace& face, glm::vec3& surfaceNormal,
                                             void** intersectedObject, bool precisionPicking) const override { return false; }

    virtual void debugDump() const override;

    static const QString DEFAULT_LEOPOLY_URL;
    static const QUuid DEFAULT_LEOPOLY_MODEL_VERSION;

    virtual void doExportCurrentState(){};

    //Sends the actual geometry data to the LeoPolyEngine
    virtual void sendToLeoEngine(ModelPointer model){};

 protected:

    QString _leoPolyURL;
    QUuid _modelVersion;
    glm::vec3 _leoPolyControllerPos;
    glm::quat _leoPolyControllerRot;
    float _leoPolyTriggerState;
};

#endif // hifi_LeoPolyEntityItem_h
