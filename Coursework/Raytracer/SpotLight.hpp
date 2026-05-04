#pragma once
#include "Light.hpp"
#include <cmath>

class SpotLight : public Light
{
private:
	Eigen::Vector3f location_;
	Eigen::Vector3f intensity_;
	Eigen::Vector3f direction_;
	float cosAngle_;

public:
	SpotLight(const Eigen::Vector3f& location, const Eigen::Vector3f& intensity,
		const Eigen::Vector3f& direction, float angle)
		:location_(location), intensity_(intensity), direction_(direction), cosAngle_(cosf(angle))
	{}

	virtual bool visibilityCheck(const Eigen::Vector3f& location, const Renderable* renderable) const override
	{
		Ray shadowRay;
		shadowRay.origin = location;
		shadowRay.direction = (location_ - location).normalized();
		float maxT = (location_ - location).norm();
		HitInfo info;
		return !renderable->intersect(shadowRay, 1e-4f, maxT, info, SHADOW_BITMASK);
	}

	virtual Eigen::Vector3f getIntensity(const Eigen::Vector3f& location) const override
	{
		auto surfaceDir = (location - location_).normalized();
		if (surfaceDir.dot(direction_) < cosAngle_) {
			return Eigen::Vector3f::Zero();
		}

		float softness = (surfaceDir.dot(direction_) - cosAngle_) / (1.0f - cosAngle_);
		float dist = (location_ - location).norm();
		return (intensity_ * softness) / (dist * dist);
	}

	virtual Eigen::Vector3f getVecToLight(const Eigen::Vector3f& location) const override
	{
		return (location_ - location).normalized();
	}
};

