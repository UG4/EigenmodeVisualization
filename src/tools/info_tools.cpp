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

#include <vector>
#include "app.h"
#include "standard_tools.h"
#include "tooltips.h"

using namespace std;
using namespace ug;

class ToolPrintGeometryInfo : public ITool
{
	public:
		void execute(LGObject* obj, QWidget*){

			ug::Grid& grid = obj->grid();
			vector3 vMin, vMax;
			vector3 vDim;
			obj->get_bounding_box(vMin, vMax);
			VecSubtract(vDim, vMax, vMin);

			UG_LOG("Geometry Info:\n");
			UG_LOG("  pivot:\t\t" << obj->pivot() << endl);
			UG_LOG("  bounding box:\t" << vMin << ", " << vMax << endl);
			UG_LOG("  dimensions:\t" << vDim << endl);

			UG_LOG("  vertices:\t" << grid.num<Vertex>() << endl);
			UG_LOG("  edges:\t" << grid.num<Edge>() << endl);
			UG_LOG("  faces:\t" << grid.num<Face>() << endl);
			UG_LOG("  volumes:\t " << grid.num<Volume>() << endl);

			UG_LOG(endl);
		}

		const char* get_name()		{return "Print Geometry Info";}
		const char* get_tooltip()	{return TOOLTIP_PRINT_GEOMETRY_INFO;}
		const char* get_group()		{return "Info";}
};

class ToolPrintSelectionInfo : public ITool
{
	public:
		void execute(LGObject* obj, QWidget*){
			using namespace ug;
			ug::Grid& grid = obj->grid();
			ug::Selector& sel = obj->selector();
			UG_LOG("Selection Info:\n");
			PrintElementNumbers(sel.get_grid_objects());

		//	count the number of selected boundary faces
			if(grid.num_volumes() > 0 && sel.num<Face>() > 0){
				int numBndFaces = 0;
				for(FaceIterator iter = sel.faces_begin(); iter != sel.faces_end(); ++iter){
					if(IsVolumeBoundaryFace(grid, *iter))
						++numBndFaces;
				}
				UG_LOG("  selected boundary faces: " << numBndFaces << endl);
			}
			UG_LOG(endl);
		}

		const char* get_name()		{return "Print Selection Info";}
		const char* get_tooltip()	{return TOOLTIP_PRINT_SELECTION_INFO;}
		const char* get_group()		{return "Info";}
};


template <class TGeomObj>
static bool SubsetContainsSelected(SubsetHandler& sh, Selector& sel, int si)
{
	typedef typename geometry_traits<TGeomObj>::iterator GeomObjIter;
	for(GeomObjIter iter = sh.begin<TGeomObj>(si);
	iter != sh.end<TGeomObj>(si); ++iter)
	{
		if(sel.is_selected(*iter)){
			return true;
		}
	}

	return false;
}


void RegisterInfoTools(ToolManager* toolMgr)
{
	toolMgr->register_tool(new ToolPrintGeometryInfo, Qt::Key_I);
	toolMgr->register_tool(new ToolPrintSelectionInfo);
}

