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

#include <QtWidgets>
#include <vector>
#include "app.h"
#include "standard_tools.h"

using namespace std;
using namespace ug;

void RegisterStandardTools(ToolManager* toolMgr)
{

	toolMgr->set_group_icon("Camera", ":images/tool_camera.png");
	toolMgr->set_group_icon("Oscillation", ":images/tool_transform.png");
	toolMgr->set_group_icon("Info", ":images/tool_info.png");
	toolMgr->set_group_icon("Wave", ":images/tool_transform.png");
	toolMgr->set_group_icon("Helmholtz", ":images/tool_transform.png");
	toolMgr->set_group_icon("SolutionRefinement", ":images/tool_transform.png");

//	camera
	RegisterCameraTools(toolMgr);
	RegisterInfoTools(toolMgr);
	RegisterOscillationTools(toolMgr);
	RegisterWaveTools(toolMgr);
	RegisterHelmholtzTools(toolMgr);
	RegisterSolutionRefinementTools(toolMgr);
}
