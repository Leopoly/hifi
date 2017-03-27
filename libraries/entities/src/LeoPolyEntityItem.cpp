//
//  LeoPolyEntityItem.cpp
//  libraries/entities/src
//
//  Created by Seth Alves on 2016-11-5.
//  Copyright 2016 High Fidelity, Inc.
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
#include "LeoPolyEntityItem.h"


const QString LeoPolyEntityItem::DEFAULT_LEOPOLY_URL = "";
const QUuid LeoPolyEntityItem::DEFAULT_LEOPOLY_MODEL_VERSION = QUuid::createUuid();

EntityItemPointer LeoPolyEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    EntityItemPointer entity { new LeoPolyEntityItem(entityID) };
    entity->setProperties(properties);
    return entity;
}

LeoPolyEntityItem::LeoPolyEntityItem(const EntityItemID& entityItemID) :
    EntityItem(entityItemID)
{
    _type = EntityTypes::LeoPoly;
}



EntityItemProperties LeoPolyEntityItem::getProperties(EntityPropertyFlags desiredProperties) const {
    EntityItemProperties properties = EntityItem::getProperties(desiredProperties); // get the properties from our base class

    COPY_ENTITY_PROPERTY_TO_PROPERTIES(leoPolyURL, getLeoPolyURL);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(leoPolyModelVersion, getLeoPolyModelVersion);

    COPY_ENTITY_PROPERTY_TO_PROPERTIES(leoPolyControllerPos, getLeoPolyControllerPos);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(leoPolyControllerRot, getLeoPolyControllerRot);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(leoPolyTriggerState, getLeoPolyTriggerState);

    return properties;
}

bool LeoPolyEntityItem::setProperties(const EntityItemProperties& properties) {
    bool somethingChanged = EntityItem::setProperties(properties); // set the properties in our base class

    SET_ENTITY_PROPERTY_FROM_PROPERTIES(leoPolyURL, setLeoPolyURL);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(leoPolyModelVersion, setLeoPolyModelVersion);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(leoPolyControllerPos, setLeoPolyControllerPos);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(leoPolyControllerRot, setLeoPolyControllerRot);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(leoPolyTriggerState, setLeoPolyTriggerState);

    if (somethingChanged) {
        bool wantDebug = false;
        if (wantDebug) {
            uint64_t now = usecTimestampNow();
            int elapsed = now - getLastEdited();
            qCDebug(entities) << "LeoPolyEntityItem::setProperties() AFTER update... edited AGO=" << elapsed <<
                "now=" << now << " getLastEdited()=" << getLastEdited();
        }
        setLastEdited(properties._lastEdited);
    }
    return somethingChanged;
}

int LeoPolyEntityItem::readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead,
                                                        ReadBitstreamToTreeParams& args,
                                                        EntityPropertyFlags& propertyFlags, bool overwriteLocalData,
                                                        bool& somethingChanged) {

    int bytesRead = 0;
    const unsigned char* dataAt = data;

    READ_ENTITY_PROPERTY(PROP_LEOPOLY_URL, QString, setLeoPolyURL);
    READ_ENTITY_PROPERTY(PROP_LEOPOLY_MODEL_VERSION, QUuid, setLeoPolyModelVersion);

    READ_ENTITY_PROPERTY(PROP_LEOPOLY_CONTROLLER_POS, glm::vec3, setLeoPolyControllerPos);
    READ_ENTITY_PROPERTY(PROP_LEOPOLY_CONTROLLER_ROT, glm::quat, setLeoPolyControllerRot);
    READ_ENTITY_PROPERTY(PROP_LEOPOLY_TRIGGER_STATE, float, setLeoPolyTriggerState);

    return bytesRead;
}


// TODO: eventually only include properties changed since the params.lastViewFrustumSent time
EntityPropertyFlags LeoPolyEntityItem::getEntityProperties(EncodeBitstreamParams& params) const {
    EntityPropertyFlags requestedProperties = EntityItem::getEntityProperties(params);
    requestedProperties += PROP_LEOPOLY_URL;
    requestedProperties += PROP_LEOPOLY_MODEL_VERSION;
    requestedProperties += PROP_LEOPOLY_CONTROLLER_POS;
    requestedProperties += PROP_LEOPOLY_CONTROLLER_ROT;
    requestedProperties += PROP_LEOPOLY_TRIGGER_STATE;
    return requestedProperties;
}

void LeoPolyEntityItem::appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params,
                                           EntityTreeElementExtraEncodeDataPointer modelTreeElementExtraEncodeData,
                                           EntityPropertyFlags& requestedProperties,
                                           EntityPropertyFlags& propertyFlags,
                                           EntityPropertyFlags& propertiesDidntFit,
                                           int& propertyCount,
                                           OctreeElement::AppendState& appendState) const {
    bool successPropertyFits = true;
    APPEND_ENTITY_PROPERTY(PROP_LEOPOLY_URL, getLeoPolyURL());
    APPEND_ENTITY_PROPERTY(PROP_LEOPOLY_MODEL_VERSION, getLeoPolyModelVersion());

    APPEND_ENTITY_PROPERTY(PROP_LEOPOLY_CONTROLLER_POS, getLeoPolyControllerPos());
    APPEND_ENTITY_PROPERTY(PROP_LEOPOLY_CONTROLLER_ROT, getLeoPolyControllerRot());
    APPEND_ENTITY_PROPERTY(PROP_LEOPOLY_TRIGGER_STATE, getLeoPolyTriggerState());

}

void LeoPolyEntityItem::debugDump() const {
    quint64 now = usecTimestampNow();
    qCDebug(entities) << "   LEOPOLY EntityItem id:" << getEntityItemID() << "---------------------------------------------";
    qCDebug(entities) << "            position:" << debugTreeVector(getPosition());
    qCDebug(entities) << "          dimensions:" << debugTreeVector(getDimensions());
    qCDebug(entities) << "       getLastEdited:" << debugTime(getLastEdited(), now);
    qCDebug(entities) << "    leoPoly URL:" << getLeoPolyURL();
    qCDebug(entities) << "    leoPoly model version:" << getLeoPolyModelVersion();
    qCDebug(entities) << "    leoPoly Controller pos:" << getLeoPolyControllerPos();
    qCDebug(entities) << "    leoPoly Controller pos:" << getLeoPolyControllerRot();
    qCDebug(entities) << "    leoPoly trigger state:" << getLeoPolyTriggerState();
}
