/*
 * Copyright (c) 2008-2015:  G-CSC, Goethe University Frankfurt
 * Copyright (c) 2006-2008:  Steinbeis Forschungszentrum (STZ Ölbronn)
 * Copyright (c) 2006-2015:  Sebastian Reiter
 * Author: Sebastian Reiter
 *
 * This file is part of ProMesh.
 * 
 * ProMesh is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License version 3 (as published by the
 * Free Software Foundation) with the following additional attribution
 * requirements (according to LGPL/GPL v3 §7):
 * 
 * (1) The following notice must be displayed in the Appropriate Legal Notices
 * of covered and combined works: "Based on ProMesh (www.promesh3d.com)".
 * 
 * (2) The following bibliography is recommended for citation and must be
 * preserved in all covered files:
 * "Reiter, S. and Wittum, G. ProMesh -- a flexible interactive meshing software
 *   for unstructured hybrid grids in 1, 2, and 3 dimensions. In preparation."
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 */

#include <string>
#include "promesh_plugin.h"
#include "app.h"
#include "standard_tools.h"
#include "tooltips.h"

using namespace std;
using namespace ug;
using namespace ug::promesh;
using namespace ug::bridge;

class ToolCenterObject : public ITool
{
	public:
		void execute(LGObject* obj, QWidget* widget){
			ug::Sphere3 s = obj->get_bounding_sphere();

			app::getMainWindow()->getView3D()->fly_to(
								cam::vector3(s.get_center().x(),
											s.get_center().y(),
											s.get_center().z()),
								s.get_radius() * 4.f + 0.001);
		}

		const char* get_name()		{return "Center Object";}
		const char* get_tooltip()	{return TOOLTIP_CENTER_OBJECT;}
		const char* get_group()		{return "Camera";}
};

class ToolCenterSelection : public ITool
{
	public:
		void execute(LGObject* obj, QWidget* widget){
			ug::Selector& sel = obj->selector();
			ug::Grid::VertexAttachmentAccessor<ug::APosition> aaPos(obj->grid(), ug::aPosition);

			View3D* view = app::getMainWindow()->getView3D();
			cam::SCameraState oldCam = view->camera().get_camera_state();

			ug::vector3 center;

		//	calculate and focus the center
			if(ug::CalculateCenter(center, sel, aaPos)){
				view->fly_to(cam::vector3(center.x(), center.y(),center.z()),
							 oldCam.fDistance);
			}
		}

		const char* get_name()		{return "Center Selection";}
		const char* get_tooltip()	{return TOOLTIP_CENTER_SELECTION;}
		const char* get_group()		{return "Camera";}
};

class ToolTopView : public ITool
{
	public:
		void execute(LGObject* obj, QWidget* widget){
			View3D* view = app::getMainWindow()->getView3D();
			cam::SCameraState oldCam = view->camera().get_camera_state();
		//	construct a new state
			cam::SCameraState newCam;
			newCam.fDistance = oldCam.fDistance;
			newCam.vTo = oldCam.vTo;
			newCam.vFrom = newCam.vTo;
			newCam.vFrom.z() += newCam.fDistance;
			newCam.quatOrientation.set_values(0, 0, -1, 0);

			view->camera().set_camera_state(newCam);
			view->update();
		}

		const char* get_name()		{return "Top View";}
		const char* get_tooltip()	{return TOOLTIP_TOP_VIEW;}
		const char* get_group()		{return "Camera";}
};

void FlyTo (Mesh* msh, const vector3& to)
{
	app::getMainWindow()->getView3D()->fly_to (to);
}


void RegisterCameraTools(ToolManager* toolMgr)
{
	toolMgr->register_tool(new ToolCenterObject);
	toolMgr->register_tool(new ToolCenterSelection);
	toolMgr->register_tool(new ToolTopView);

	ProMeshRegistry& reg = GetProMeshRegistry();

	string grp = "ug4/promesh/Camera";
	reg.add_function("FlyTo", &FlyTo, grp, "",
				"mesh # target position",
				"Flies to the specified point. The specified point will be the "
				"new focus point of the camera. ");

}
