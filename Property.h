#ifndef PROPERTY_H
#define PROPERTY_H

#include <string>
#include "SDL2/SDL.h"

/**
 * @brief The base class for all properties.
 */
class Property {
public:
	virtual ~Property() {}  // Virtual destructor for proper cleanup

	/**
  * @brief Get the type of the property.
  * @return The type of the property as a string.
  */
	virtual std::string getType() const = 0;

};

/**
 * @brief A property representing a rectangle.
 */
class RectProperty : public Property {
public:

	/**
  * @brief Construct a new RectProperty object.
  * @param x The x-coordinate of the rectangle.
  * @param y The y-coordinate of the rectangle.
  * @param w The width of the rectangle.
  * @param h The height of the rectangle.
  */
	RectProperty(int x, int y, int w, int h) : x(x), y(y), w(w), h(h) {}

	std::string getType() const override {
		return "Rect";
	}

	int x; /**< The x-coordinate of the rectangle. */
	int y; /**< The y-coordinate of the rectangle. */
	int w; /**< The width of the rectangle. */
	int h; /**< The height of the rectangle. */

};

/**
 * @brief A property representing a render color.
 */
class RenderProperty : public Property {
public:

	/**
  * @brief Construct a new RenderProperty object.
  * @param r The red component of the color.
  * @param g The green component of the color.
  * @param b The blue component of the color.
  */
	RenderProperty(int r, int g, int b) : r(r), g(g), b(b) {}

	std::string getType() const override {
		return "Render";
	}

	int r; /**< The red component of the color. */
	int g; /**< The green component of the color. */
	int b; /**< The blue component of the color. */

};

/**
 * @brief A property representing physics properties.
 */
class PhysicsProperty : public Property {
public:

	/**
  * @brief Construct a new PhysicsProperty object.
  * @param gravity The gravity value.
  */
	PhysicsProperty(int gravity) : gravity(gravity) {}

	std::string getType() const override {
		return "Physics";
	}

	int gravity; /**< The gravity value. */

};

/**
 * @brief A property representing collision properties.
 */
class CollisionProperty : public Property {
public:

	/**
  * @brief Construct a new CollisionProperty object.
  * @param hasCollision Whether the object has collision or not.
  */
	CollisionProperty(bool hasCollision) : hasCollision(hasCollision) {}

	std::string getType() const override {
		return "Collision";
	}

	bool hasCollision; /**< Whether the object has collision or not. */

};

/**
 * @brief A property representing velocity properties.
 */
class VelocityProperty : public Property {
public:

	/**
  * @brief Construct a new VelocityProperty object.
  * @param vx The velocity in the x-axis.
  * @param vy The velocity in the y-axis.
  */
	VelocityProperty(int vx, int vy) : vx(vx), vy(vy) {}

	std::string getType() const override {
		return "Velocity";
	}

	int vx; /**< The velocity in the x-axis. */
	int vy; /**< The velocity in the y-axis. */

};

/**
 * @brief A property representing input properties.
 */
class InputProperty : public Property {
public:

	/**
  * @brief Construct a new InputProperty object.
  * @param hasInput Whether the object has input or not.
  */
	InputProperty(bool hasInput) : hasInput(hasInput) {}

	std::string getType() const override {
		return "Input";
	}

	bool hasInput; /**< Whether the object has input or not. */

};

/**
 * @brief A property representing scrolling properties.
 */
class ScrollingProperty : public Property {
public:

	/**
  * @brief Construct a new ScrollingProperty object.
  * @param scrolls Whether the object scrolls or not.
  */
	ScrollingProperty(bool scrolls) : scrolls(scrolls) {}

	std::string getType() const override {
		return "Scrolling";
	}

	bool scrolls; /**< Whether the object scrolls or not. */

};

#endif // PROPERTY_H
