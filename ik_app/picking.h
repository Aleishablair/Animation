#ifndef _PICKING_H
#define _PICKING_H

#include <maths/vector2.h>
#include <maths/matrix44.h>
#include <maths/plane.h>

#include "picking.h"
#include <math.h>

void GetScreenPosRay(const gef::Vector2& screen_position, const gef::Matrix44& projection, const gef::Matrix44& view, gef::Vector4& start_point, gef::Vector4& direction, float screen_width, float screen_height, float ndc_z_min);
bool RaySphereIntersect(const gef::Vector4& start_point, const gef::Vector4& direction, const gef::Vector4& sphere_centre, float sphere_radius, gef::Vector4& hitpoint);
bool RayPlaneIntersect(gef::Vector4& start_point, gef::Vector4& direction, const gef::Vector4& point_on_plane, const gef::Vector4& plane_normal, gef::Vector4& hitpoint);

#endif // _PICKING_H