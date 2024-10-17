#ifndef PROPERTY_H
#define PROPERTY_H

#include <string>
#include <memory>

// Base class for all properties
class Property {
public:
    // Virtual destructor for base class
    virtual ~Property() {}

    // Each property should return its unique type as a string
    virtual std::string getType() const = 0;
};

// Example of a specific property: position
class PositionProperty : public Property {
public:
    int x, y;

    PositionProperty(int x = 0, int y = 0) : x(x), y(y) {}

    std::string getType() const override {
        return "Position";
    }
};

// Example of a specific property: velocity
class VelocityProperty : public Property {
public:
    int vx, vy;

    VelocityProperty(int vx = 0, int vy = 0) : vx(vx), vy(vy) {}

    std::string getType() const override {
        return "Velocity";
    }
};

#endif // PROPERTY_H
