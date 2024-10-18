#ifndef PROPERTY_H
#define PROPERTY_H

#include <string>

class Property {
public:
	virtual ~Property() {}  // Virtual destructor for proper cleanup
	virtual std::string getType() const = 0;

};

class RectProperty : public Property {
public:

	RectProperty(int x, int y, int w, int h) : x(x), y(y), w(w), h(h) {}

	std::string getType() const override {
		return "Rect";
	}

	int x;
	int y;
	int w;
	int h;

};

class RenderProperty : public Property {
public:

	RenderProperty(int r, int g, int b) : r(r), g(g), b(b) {}

	std::string getType() const override {
		return "Render";
	}

	int r;
	int g;
	int b;

};

class PhysicsProperty : public Property {
public:

	PhysicsProperty(float gravity) : gravity(gravity) {}

	std::string getType() const override {
		return "Physics";
	}

	float gravity;

};

class CollisionProperty : public Property {
public:

	CollisionProperty(bool hasCollision) : hasCollision(hasCollision) {}

	std::string getType() const override {
		return "Collision";
	}

	bool hasCollision;

};

class VelocityProperty : public Property {
public:

	VelocityProperty(float vx, float vy) : vx(vx), vy(vy) {}

	std::string getType() const override {
		return "Velocity";
	}

	float vx;
	float vy;

};

#endif // PROPERTY_H
