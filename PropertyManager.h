#ifndef PROPERTY_MANAGER_H
#define PROPERTY_MANAGER_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>
#include "Property.h"

class PropertyManager {
public:
    static PropertyManager& getInstance() {
        static PropertyManager instance;
        return instance;
    }

    // Add a property to an object by its ID and property type
    void addProperty(int objectID, const std::string& key, std::shared_ptr<Property> property) {
        properties[objectID][key] = property;
        if (key == "Collision") {
            collisionObjects.push_back(objectID);  // Store object IDs that have CollisionProperty
        }
    }

    // Get a property from an object by its ID and property type
    std::shared_ptr<Property> getProperty(int objectID, const std::string& key) {
        if (properties.count(objectID) && properties[objectID].count(key)) {
            return properties[objectID][key];
        }
        return nullptr;
    }

    // Get all objects that have a collision property
    const std::vector<int>& getCollisionObjects() const {
        return collisionObjects;
    }

    // Create a new game object and return its unique ID
    int createObject() {
        static int nextID = 0;
        return nextID++;  // Generate and return a unique object ID
    }

private:
    PropertyManager() = default;
    ~PropertyManager() = default;

    // Prevent copy constructor and assignment
    PropertyManager(const PropertyManager&) = delete;
    PropertyManager& operator=(const PropertyManager&) = delete;

    std::unordered_map<int, std::unordered_map<std::string, std::shared_ptr<Property>>> properties;
    std::vector<int> collisionObjects;  // Store IDs of objects with CollisionProperty
};

#endif // PROPERTY_MANAGER_H
