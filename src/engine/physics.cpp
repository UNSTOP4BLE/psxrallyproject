#include "physics.hpp"

namespace ENGINE::PHYSICS {

void actGravity3D() {
/*   chatgpt craps
     acceleration.y = -9.8f; // downward gravity
    acceleration.x = 0.0f;   // no horizontal acceleration
    acceleration.z = 0.0f;

    // 2. Update velocity
    velocity.x += acceleration.x * dt;
    velocity.y += acceleration.y * dt;
    velocity.z += acceleration.z * dt;

    // 3. Update position
    position.x += velocity.x * dt;
    position.y += velocity.y * dt;
    position.z += velocity.z * dt;

    // 4. Handle floor collision
    if (position.y < 0.0f) {   // floor at y = 0
        position.y = 0.0f;     // clamp to ground
        velocity.y = 0.0f;     // stop downward velocity
    }

    // 5. Set the render position (if needed)
    pos.y = position.y; // this is the Y coordinate your renderer uses
    pos.x = position.x;
    pos.z = position.z;
    */
}

}