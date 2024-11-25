#ifndef PROPERTY_MANAGER_H
#define PROPERTY_MANAGER_H

#include <unordered_map>
#include <string>
#include <memory>
#include "Property.h"

/**
 * @brief The PropertyManager class is responsible for managing properties of game objects.
 */
class PropertyManager {
public:
    /**
     * @brief Get the singleton instance of PropertyManager.
     * @return The singleton instance of PropertyManager.
     */
    static PropertyManager& getInstance() {
        static PropertyManager instance;
        return instance;
    }

    /**
     * @brief Add a property to an object by its ID and property type.
     * @param objectID The ID of the object.
     * @param key The key of the property.
     * @param property The property to add.
     */
    void addProperty(int objectID, const std::string& key, std::shared_ptr<Property> property) {
        properties[objectID][key] = property;
    }

    /**
     * @brief Get a property from an object by its ID and property type.
     * @param objectID The ID of the object.
     * @param key The key of the property.
     * @return The property if found, nullptr otherwise.
     */
    std::shared_ptr<Property> getProperty(int objectID, const std::string& key) {
        if (properties.count(objectID) && properties[objectID].count(key)) {
            return properties[objectID][key];
        }
        return nullptr;
    }

    /**
     * @brief Check if an object exists in the properties map.
     * @param objectID The ID of the object.
     * @return True if the object exists, false otherwise.
     */
    bool hasObject(int objectID) const {
        return properties.count(objectID) > 0;
    }

    /**
     * @brief Create a new game object and return its unique ID.
     * @return The unique ID of the new game object.
     */
    int createObject() {
        static int nextID = 0;
        return nextID++;  // Generate and return a unique object ID
    }

    /**
     * @brief Remove a game object and all its associated properties.
     * @param objectID The ID of the object to remove.
     */
    void destroyObject(int objectID) {
        properties.erase(objectID);
    }

    /**
     * @brief Accessor for the entire properties map.
     * @return The entire properties map.
     */
    const std::unordered_map<int, std::unordered_map<std::string, std::shared_ptr<Property>>>& getAllProperties() const {
        return properties;
    }

private:
    PropertyManager() = default;
    ~PropertyManager() = default;

    // Prevent copy constructor and assignment
    PropertyManager(const PropertyManager&) = delete;
    PropertyManager& operator=(const PropertyManager&) = delete;

    std::unordered_map<int, std::unordered_map<std::string, std::shared_ptr<Property>>> properties;
};

#endif // PROPERTY_MANAGER_H
