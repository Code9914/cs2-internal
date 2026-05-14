#pragma once
#include <cmath>

struct Vector2 {
    float x, y;

    Vector2() : x(0), y(0) {}
    Vector2(float x, float y) : x(x), y(y) {}
};

struct Vector3 {
    float x, y, z;

    Vector3() : x(0), y(0), z(0) {}
    Vector3(float x, float y, float z) : x(x), y(y), z(z) {}

    Vector3 operator-(const Vector3& v) const { return Vector3(x - v.x, y - v.y, z - v.z); }
    Vector3 operator+(const Vector3& v) const { return Vector3(x + v.x, y + v.y, z + v.z); }
    Vector3 operator*(float s) const { return Vector3(x * s, y * s, z * s); }
};

struct ViewMatrix {
    float data[16];

    float operator()(int row, int col) const { return data[row * 4 + col]; }
};

inline bool WorldToScreen(const Vector3& world, Vector2& screen, const ViewMatrix& vm, int screenW, int screenH) {
    float w = vm(3, 0) * world.x + vm(3, 1) * world.y + vm(3, 2) * world.z + vm(3, 3);
    if (w < 0.001f) return false;

    float invW = 1.f / w;
    float x = vm(0, 0) * world.x + vm(0, 1) * world.y + vm(0, 2) * world.z + vm(0, 3);
    float y = vm(1, 0) * world.x + vm(1, 1) * world.y + vm(1, 2) * world.z + vm(1, 3);

    screen.x = (screenW / 2.f) * (1.f + x * invW);
    screen.y = (screenH / 2.f) * (1.f - y * invW);

    return true;
}
