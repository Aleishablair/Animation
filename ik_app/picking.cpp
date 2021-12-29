#include "picking.h"
#include <math.h>

// http://antongerdelan.net/opengl/raycasting.html
// https://forum.libcinder.org/topic/picking-ray-from-mouse-coords
void GetScreenPosRay(const gef::Vector2& screen_position, const gef::Matrix44& projection, const gef::Matrix44& view, gef::Vector4& start_point, gef::Vector4& direction, float screen_width, float screen_height, float ndc_z_min)
{
	gef::Vector2 ndc;

	float hw =screen_width * 0.5f;
	float hh = screen_height * 0.5f;

	ndc.x = (static_cast<float>(screen_position.x) - hw) / hw;
	ndc.y = (hh - static_cast<float>(screen_position.y)) / hh;

	gef::Matrix44 projectionInverse;
	projectionInverse.Inverse(view * projection);

	gef::Vector4 nearPoint, farPoint;

	nearPoint = gef::Vector4(ndc.x, ndc.y, ndc_z_min, 1.0f).TransformW(projectionInverse);
	farPoint = gef::Vector4(ndc.x, ndc.y, 1.0f, 1.0f).TransformW(projectionInverse);

	nearPoint /= nearPoint.w();
	farPoint /= farPoint.w();

	start_point = gef::Vector4(nearPoint.x(), nearPoint.y(), nearPoint.z());
	direction = farPoint - nearPoint;
	direction.Normalise();
}

// modified from https://gamedev.stackexchange.com/questions/96459/fast-ray-sphere-collision-code
bool RaySphereIntersect(const gef::Vector4& start_point, const gef::Vector4& direction, const gef::Vector4& sphere_centre, float sphere_radius, gef::Vector4& hitpoint)
{
	gef::Vector4 m = start_point - sphere_centre;
	float b = m.DotProduct(direction);
	float c = m.LengthSqr() - sphere_radius * sphere_radius;

	// Exit if rays origin outside sphere (c > 0) and ray pointing away from sphere (b > 0) 
	if (c > 0.0f && b > 0.0f)
		return false;
	float discr = b * b - c;

	// A negative discriminant corresponds to ray missing sphere 
	if (discr < 0.0f)
		return false;

	// Ray now found to intersect sphere, compute smallest t value of intersection
	float t = -b - sqrtf(discr);

	// If t is negative, ray started inside sphere so clamp t to zero 
	if (t < 0.0f)
		t = 0.0f;

	hitpoint = start_point + direction * t;

	return true;
}


// modified and fixed from https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-plane-and-ray-disk-intersection
bool RayPlaneIntersect(gef::Vector4& start_point, gef::Vector4& direction, const gef::Vector4& point_on_plane, const gef::Vector4& plane_normal, gef::Vector4& hitpoint)
{
	gef::Vector4 p0, l0, n, l;
	l0 = start_point;
	l = direction;
	p0 = point_on_plane;
	n = plane_normal;
	float t = 0.0f;



	// assuming vectors are all normalized
	float denom = n.DotProduct(l);
	if (fabsf(denom) > 1e-6) {
		gef::Vector4 p0l0 = p0 - l0;
		t = p0l0.DotProduct(n) / denom;

		if(t >= 0)
			hitpoint = start_point + direction * t;

		return (t >= 0);
	}

	return false;
}

