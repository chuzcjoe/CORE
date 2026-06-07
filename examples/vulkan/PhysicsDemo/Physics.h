#pragma once

#include <random>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"

namespace core {

// Rigid-body sim for a unit cube bouncing on the y=0 floor.
//
// Integration: semi-implicit Euler.
// Collision: per-corner with the floor plane y=0. On contact, applies a normal
// impulse (Newton coefficient of restitution) plus a tangential friction
// impulse (Coulomb with infinite static friction within the cone).
//
// When the kinetic energy stays below a threshold for a short window, the
// state auto-resets to the spawn position so the demo loops forever.
class PhysicsBody {
 public:
  // Cube half-extent (so cube spans [-half_size, +half_size] in body space).
  static constexpr float kHalfSize = 0.5f;
  // World gravity.
  static constexpr glm::vec3 kGravity = glm::vec3(0.0f, -9.81f, 0.0f);
  // Bounciness.
  static constexpr float kRestitution = 0.55f;
  // Coulomb friction coefficient at contacts.
  static constexpr float kFriction = 0.35f;
  // Below this combined kinetic energy, we start counting toward sleep.
  static constexpr float kSleepEnergy = 0.02f;
  // After this many seconds of being "near-still", reset to spawn.
  static constexpr float kSleepResetSeconds = 1.2f;

  // Random spawn region (so the cube lands somewhere new each cycle).
  static constexpr float kSpawnXZRange = 2.0f;
  static constexpr float kSpawnYMin = 4.5f;
  static constexpr float kSpawnYMax = 6.5f;

  PhysicsBody() : rng_(std::random_device{}()) { Reset(); }

  void Reset() {
    std::uniform_real_distribution<float> xz(-kSpawnXZRange, kSpawnXZRange);
    std::uniform_real_distribution<float> y(kSpawnYMin, kSpawnYMax);
    std::uniform_real_distribution<float> v_lin(-1.5f, 1.5f);
    std::uniform_real_distribution<float> v_ang(-3.0f, 3.0f);

    position_ = glm::vec3(xz(rng_), y(rng_), xz(rng_));
    // Tossed in with a random horizontal nudge and random tumble so each drop
    // hits the floor at a different attitude/velocity.
    velocity_ = glm::vec3(v_lin(rng_), 0.0f, v_lin(rng_));
    angular_velocity_ = glm::vec3(v_ang(rng_), v_ang(rng_), v_ang(rng_));
    orientation_ = RandomUnitQuat();
    sleep_timer_ = 0.0f;
  }

  // Advance the simulation by dt seconds. dt is clamped to keep the impulse
  // solver stable when the host hitches.
  void Step(float dt) {
    dt = glm::clamp(dt, 0.0f, 1.0f / 60.0f);

    // Integrate linear & angular velocity (gravity + no angular damping).
    velocity_ += kGravity * dt;
    position_ += velocity_ * dt;

    // Quaternion integration: q' = 0.5 * omega_quat * q
    if (glm::length(angular_velocity_) > 0.0f) {
      glm::quat omega(0.0f, angular_velocity_);
      orientation_ = glm::normalize(orientation_ + 0.5f * dt * omega * orientation_);
    }

    ResolveFloorContacts();

    // Auto-reset when settled.
    const float lin_e = 0.5f * mass_ * glm::dot(velocity_, velocity_);
    const float ang_e = 0.5f * glm::dot(angular_velocity_, InertiaTimes(angular_velocity_));
    if (lin_e + ang_e < kSleepEnergy && position_.y < kHalfSize * 1.05f) {
      sleep_timer_ += dt;
      if (sleep_timer_ >= kSleepResetSeconds) {
        Reset();
      }
    } else {
      sleep_timer_ = 0.0f;
    }
  }

  // World-space model matrix for the cube.
  glm::mat4 ModelMatrix() const {
    glm::mat4 t = glm::translate(glm::mat4(1.0f), position_);
    return t * glm::mat4_cast(orientation_);
  }

 private:
  // Resolve contacts between cube corners and the floor (y = 0).
  // Iterates a few times so simultaneous corner contacts settle in one frame.
  void ResolveFloorContacts() {
    const glm::vec3 normal(0.0f, 1.0f, 0.0f);
    const glm::mat3 rot = glm::mat3_cast(orientation_);
    constexpr int kIterations = 4;

    for (int iter = 0; iter < kIterations; ++iter) {
      // Find deepest penetrating corner this iteration.
      int deepest = -1;
      float deepest_depth = 0.0f;
      glm::vec3 deepest_r{0.0f};

      for (int i = 0; i < 8; ++i) {
        const glm::vec3 corner_local((i & 1) ? kHalfSize : -kHalfSize,
                                     (i & 2) ? kHalfSize : -kHalfSize,
                                     (i & 4) ? kHalfSize : -kHalfSize);
        const glm::vec3 r = rot * corner_local;
        const glm::vec3 world = position_ + r;
        const float depth = -world.y;
        if (depth > deepest_depth) {
          deepest_depth = depth;
          deepest = i;
          deepest_r = r;
        }
      }
      if (deepest < 0) return;

      // Positional correction so we don't sink into the floor over time.
      position_.y += deepest_depth;

      // Relative velocity at the contact point.
      const glm::vec3 contact_vel = velocity_ + glm::cross(angular_velocity_, deepest_r);
      const float vn = glm::dot(contact_vel, normal);

      // Only apply normal impulse if the contact is closing.
      if (vn < 0.0f) {
        // Effective mass along the normal: 1/m + n . (I^-1 (r x n)) x r
        const glm::vec3 rxn = glm::cross(deepest_r, normal);
        const glm::vec3 i_rxn = InverseInertiaTimes(rxn);
        const float k_n = 1.0f / mass_ + glm::dot(normal, glm::cross(i_rxn, deepest_r));
        const float jn = -(1.0f + kRestitution) * vn / k_n;
        const glm::vec3 impulse_n = jn * normal;
        velocity_ += impulse_n / mass_;
        angular_velocity_ += InverseInertiaTimes(glm::cross(deepest_r, impulse_n));

        // Tangential (friction) impulse.
        const glm::vec3 contact_vel2 = velocity_ + glm::cross(angular_velocity_, deepest_r);
        const glm::vec3 tangent_vel = contact_vel2 - glm::dot(contact_vel2, normal) * normal;
        const float t_speed = glm::length(tangent_vel);
        if (t_speed > 1e-4f) {
          const glm::vec3 tangent = tangent_vel / t_speed;
          const glm::vec3 rxt = glm::cross(deepest_r, tangent);
          const glm::vec3 i_rxt = InverseInertiaTimes(rxt);
          const float k_t = 1.0f / mass_ + glm::dot(tangent, glm::cross(i_rxt, deepest_r));
          float jt = -t_speed / k_t;
          // Coulomb cone clamp.
          jt = glm::clamp(jt, -kFriction * jn, kFriction * jn);
          const glm::vec3 impulse_t = jt * tangent;
          velocity_ += impulse_t / mass_;
          angular_velocity_ += InverseInertiaTimes(glm::cross(deepest_r, impulse_t));
        }
      }
    }
  }

  // Inertia tensor for a solid cube of side 2*kHalfSize and mass mass_:
  // I = (m/6) * side^2 * I3, but we precompute scalar I = m * side^2 / 6.
  // Inverse in world space: I_w^-1 = R * I_body^-1 * R^T, but for a scalar
  // inertia tensor that simplifies to (1/I) * v in any frame.
  glm::vec3 InertiaTimes(const glm::vec3& v) const {
    const float side = 2.0f * kHalfSize;
    const float i = mass_ * side * side / 6.0f;
    return i * v;
  }
  glm::vec3 InverseInertiaTimes(const glm::vec3& v) const {
    const float side = 2.0f * kHalfSize;
    const float i = mass_ * side * side / 6.0f;
    return v / i;
  }

  // Uniformly-distributed random rotation (Shoemake 1992).
  glm::quat RandomUnitQuat() {
    std::uniform_real_distribution<float> u(0.0f, 1.0f);
    const float u1 = u(rng_);
    const float u2 = u(rng_);
    const float u3 = u(rng_);
    const float s1 = std::sqrt(1.0f - u1);
    const float s2 = std::sqrt(u1);
    const float t1 = 2.0f * static_cast<float>(M_PI) * u2;
    const float t2 = 2.0f * static_cast<float>(M_PI) * u3;
    return glm::quat(s2 * std::cos(t2), s1 * std::sin(t1), s1 * std::cos(t1), s2 * std::sin(t2));
  }

  glm::vec3 position_{0.0f};
  glm::vec3 velocity_{0.0f};
  glm::quat orientation_{1.0f, 0.0f, 0.0f, 0.0f};
  glm::vec3 angular_velocity_{0.0f};
  float mass_ = 1.0f;
  float sleep_timer_ = 0.0f;
  std::mt19937 rng_;
};

}  // namespace core
