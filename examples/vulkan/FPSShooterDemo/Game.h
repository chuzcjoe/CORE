#pragma once

#include <algorithm>
#include <random>
#include <vector>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"

namespace core {

// A single projectile travelling in a straight line until it hits a target or
// expires.
struct Bullet {
  glm::vec3 position{0.0f};
  glm::vec3 velocity{0.0f};
  float life = 0.0f;
  bool active = false;
};

// A shootable cube. It rests at base_position until hit, then becomes a
// dynamic rigid-ish body (knock-back + tumble + floor bounce) before respawning.
struct Target {
  glm::vec3 base_position{0.0f};
  glm::vec3 position{0.0f};
  glm::vec3 velocity{0.0f};
  glm::vec3 angular_velocity{0.0f};
  glm::quat orientation{1.0f, 0.0f, 0.0f, 0.0f};
  glm::vec3 color{0.2f, 0.6f, 0.9f};
  bool hit = false;
  float hit_flash = 0.0f;      // 0..1 white/red flash right after a hit
  float respawn_timer = 0.0f;  // seconds since being hit
};

// Owns the gameplay state (bullets + targets) and steps it forward. Rendering
// reads the public accessors and turns them into instance transforms.
class Game {
 public:
  static constexpr float kGravity = -14.0f;
  static constexpr float kTargetHalf = 0.5f;  // cube spans 1 unit
  static constexpr float kBulletRadius = 0.08f;
  static constexpr float kBulletSpeed = 40.0f;     // muzzle velocity
  static constexpr float kBulletGravity = -16.0f;  // ballistic drop
  static constexpr float kBulletDrag = 0.04f;      // per-second velocity loss
  static constexpr float kBulletLife = 3.0f;
  static constexpr float kRestitution = 0.45f;
  static constexpr float kRespawnSeconds = 2.5f;

  Game() : rng_(std::random_device{}()) {
    const glm::vec3 colors[] = {
        {0.92f, 0.30f, 0.30f}, {0.95f, 0.70f, 0.25f}, {0.40f, 0.85f, 0.35f},
        {0.30f, 0.70f, 0.95f}, {0.65f, 0.45f, 0.92f}, {0.90f, 0.45f, 0.80f},
    };
    // LEGO-like stacked layout: each cell is {x, level, z}; level n rests
    // directly on level n-1, so the player can jump up the structures.
    const struct {
      float x;
      int level;
      float z;
    } layout[] = {
        // Staircase (column heights 1, 2, 3).
        {-6.0f, 0, -12.0f},
        {-5.0f, 0, -12.0f},
        {-5.0f, 1, -12.0f},
        {-4.0f, 0, -12.0f},
        {-4.0f, 1, -12.0f},
        {-4.0f, 2, -12.0f},
        // Brick-bond pyramid: two base cubes with one straddling on top.
        {2.0f, 0, -9.0f},
        {3.0f, 0, -9.0f},
        {2.5f, 1, -9.0f},
        // Two-cube tower.
        {7.0f, 0, -12.0f},
        {7.0f, 1, -12.0f},
        // Loose singles.
        {-1.0f, 0, -14.0f},
        {6.0f, 0, -7.0f},
    };
    int i = 0;
    for (const auto& cell : layout) {
      Target t;
      t.base_position = glm::vec3(cell.x, kTargetHalf + cell.level * 2.0f * kTargetHalf, cell.z);
      t.color = colors[i++ % 6];
      ResetTarget(t);
      targets_.push_back(t);
    }
    bullets_.resize(kMaxBullets);
  }

  // Spawn a bullet from origin travelling along dir.
  void Fire(const glm::vec3& origin, const glm::vec3& dir) {
    for (auto& b : bullets_) {
      if (b.active) continue;
      b.active = true;
      b.position = origin;
      b.velocity = glm::normalize(dir) * kBulletSpeed;
      b.life = kBulletLife;
      return;
    }
  }

  void Update(float dt) {
    UpdateBullets(dt);
    UpdateTargets(dt);
  }

  const std::vector<Bullet>& bullets() const { return bullets_; }
  const std::vector<Target>& targets() const { return targets_; }

 private:
  static constexpr size_t kMaxBullets = 64;

  void ResetTarget(Target& t) {
    t.position = t.base_position;
    t.velocity = glm::vec3(0.0f);
    t.angular_velocity = glm::vec3(0.0f);
    t.orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    t.hit = false;
    t.respawn_timer = 0.0f;
  }

  void UpdateBullets(float dt) {
    for (auto& b : bullets_) {
      if (!b.active) continue;
      // Ballistic motion: gravity drop + light air drag, integrated with
      // semi-implicit Euler so the path is a realistic downward arc.
      b.velocity.y += kBulletGravity * dt;
      b.velocity -= b.velocity * (kBulletDrag * dt);
      b.position += b.velocity * dt;
      b.life -= dt;
      if (b.life <= 0.0f || b.position.y < 0.0f) {
        b.active = false;
        continue;
      }
      for (auto& t : targets_) {
        if (t.hit) continue;
        const glm::vec3 d = b.position - t.position;
        const float r = kTargetHalf + kBulletRadius;
        if (glm::dot(d, d) <= r * r) {
          b.active = false;
          OnTargetHit(t, b.velocity);
          break;
        }
      }
    }
  }

  void OnTargetHit(Target& t, const glm::vec3& bullet_velocity) {
    t.hit = true;
    t.hit_flash = 1.0f;
    t.respawn_timer = 0.0f;
    // Knock the target back along the bullet direction with an upward pop.
    const glm::vec3 dir = glm::normalize(bullet_velocity);
    t.velocity = dir * 6.0f + glm::vec3(0.0f, 7.0f, 0.0f);
    std::uniform_real_distribution<float> spin(-9.0f, 9.0f);
    t.angular_velocity = glm::vec3(spin(rng_), spin(rng_), spin(rng_));
  }

  void UpdateTargets(float dt) {
    for (auto& t : targets_) {
      if (t.hit_flash > 0.0f) t.hit_flash = std::max(0.0f, t.hit_flash - dt * 2.5f);
      if (!t.hit) continue;

      // Dynamic phase: integrate gravity + spin and bounce off the floor.
      t.velocity.y += kGravity * dt;
      t.position += t.velocity * dt;
      if (glm::length(t.angular_velocity) > 0.0f) {
        const glm::quat omega(0.0f, t.angular_velocity);
        t.orientation = glm::normalize(t.orientation + 0.5f * dt * omega * t.orientation);
      }

      if (t.position.y < kTargetHalf) {
        t.position.y = kTargetHalf;
        if (t.velocity.y < 0.0f) t.velocity.y = -t.velocity.y * kRestitution;
        t.velocity.x *= 0.8f;
        t.velocity.z *= 0.8f;
        t.angular_velocity *= 0.8f;
      }

      t.respawn_timer += dt;
      if (t.respawn_timer > kRespawnSeconds) ResetTarget(t);
    }
  }

  std::vector<Bullet> bullets_;
  std::vector<Target> targets_;
  std::mt19937 rng_;
};

}  // namespace core
