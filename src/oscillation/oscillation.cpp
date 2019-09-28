/*
 * Copyright (c) 2019: Lukas Larisch
 * Author: Lukas Larisch
 *
 * This file is part of EmVis.
 * 
 * EmVis is free software: you can redistribute it and/or modify it under the
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
#include <QCoreApplication>
#include <QGuiApplication>
#include <QScreen>
#include <QFileDialog>
#include <math.h>
#include <stdlib.h>
#include "app.h"
#include "tooltips.h"

using namespace std;
using namespace ug;



void compute_displacements(std::vector<ug::vector3>& displacements, Grid& mode_grid, Grid& ref_grid, double scale=1.0){
	AVertex aVrt;

	Grid::AttachmentAccessor<Vertex, APosition> aaPosRef(ref_grid, aPosition);

	Grid::AttachmentAccessor<Vertex, AVertex> aaVrtMode(mode_grid, aVrt, true);
	Grid::AttachmentAccessor<Vertex, APosition> aaPosMode(mode_grid, aPosition);


	VertexIterator iterMode = mode_grid.begin<Vertex>();
	VertexIterator iterRef = ref_grid.begin<Vertex>();

	ug::vector3 point_dis;

	for(; iterMode != mode_grid.end<Vertex>();){
		VecSubtract(point_dis, aaPosMode[*iterMode], aaPosRef[*iterRef]);
				
		point_dis[0] *= scale;
		point_dis[1] *= scale;
		point_dis[2] *= scale;

		displacements.push_back(point_dis);
		++iterRef;
		++iterMode;		
	}
}

LGObject* create_copy_of(Grid& disgrid, unsigned idx){
	AVertex aVrt;
	Grid::AttachmentAccessor<Vertex, AVertex> aaVrtDIS(disgrid, aVrt, true);
	Grid::AttachmentAccessor<Vertex, APosition> aaPosDIS(disgrid, aPosition);

	LGObject* work = app::createEmptyObject("oscillation", SOT_LG, 2, idx);
	Grid& workgrid = work->grid();
	SubsetHandler& workSH = work->subset_handler();


	Grid::AttachmentAccessor<Vertex, APosition> aaPosWORK(workgrid, aPosition);

	int subsetBaseInd = 0;

	//	copy vertices
	for(VertexIterator iter = disgrid.begin<Vertex>();
				iter != disgrid.end<Vertex>(); ++iter){
		Vertex* nvrt = *workgrid.create_by_cloning(*iter);
		aaPosWORK[nvrt] = aaPosDIS[*iter];
		aaVrtDIS[*iter] = nvrt;
		workSH.assign_subset(nvrt, subsetBaseInd);
	}

	//	copy edges
	EdgeDescriptor ed;
	for(EdgeIterator iter = disgrid.begin<Edge>();
				iter != disgrid.end<Edge>(); ++iter){
		Edge* eSrc = *iter;
		ed.set_vertices(aaVrtDIS[eSrc->vertex(0)], aaVrtDIS[eSrc->vertex(1)]);
		Edge* e = *workgrid.create_by_cloning(eSrc, ed);
		workSH.assign_subset(e, subsetBaseInd);
	}

	//	copy faces
	FaceDescriptor fd;
	for(FaceIterator iter = disgrid.begin<Face>();
			iter != disgrid.end<Face>(); ++iter){
		Face* fSrc = *iter;
		fd.set_num_vertices((uint)fSrc->num_vertices());
		for(size_t i = 0; i < fd.num_vertices(); ++i)
			fd.set_vertex((uint)i, aaVrtDIS[fSrc->vertex(i)]);

		Face* f = *workgrid.create_by_cloning(fSrc, fd);
		workSH.assign_subset(f, subsetBaseInd);
	}

	//	copy volumes
	VolumeDescriptor vd;
	for(VolumeIterator iter = disgrid.begin<Volume>();
			iter != disgrid.end<Volume>(); ++iter){
		Volume* vSrc = *iter;
		vd.set_num_vertices((uint)vSrc->num_vertices());
		for(size_t i = 0; i < vd.num_vertices(); ++i)
			vd.set_vertex((uint)i, aaVrtDIS[vSrc->vertex(i)]);

		Volume* v = *workgrid.create_by_cloning(vSrc, vd);
		workSH.assign_subset(v, subsetBaseInd);
	}

	disgrid.detach_from_vertices(aVrt);

	return work;
}

void oscillation(){
	LGScene* base_scene = app::getActiveScene();

	std::vector<LGScene*> mode_scenes;
	std::vector<LGObject*> mode_objs;

	for(unsigned i = 0; i < app::numObjects(); ++i){
		mode_scenes.push_back(app::getScene(i));
		mode_objs.push_back(mode_scenes[i]->get_object(0));
	}

	LGObject* ref_obj = base_scene->get_object(0);
	Grid& ref_grid = ref_obj->grid();

	std::vector<std::vector<ug::vector3> > displacements;

	for(unsigned i = 0; i < app::numObjects(); ++i){
		displacements.push_back(std::vector<ug::vector3>());
		compute_displacements(displacements[i], mode_objs[i]->grid(), ref_grid);
	}

	std::vector<LGObject*> works;
	for(unsigned i = 0; i < app::numObjects(); ++i){
		works.push_back(create_copy_of(ref_grid, i));
	}

	for(unsigned i = 0; i < app::numObjects(); ++i){
		mode_objs[i]->set_visibility(false);
		works[i]->set_visibility(true);
	}

	Grid::AttachmentAccessor<Vertex, APosition> aaPosREF(ref_grid, aPosition);

	double arg_sine = 0.0;

	while(app::continue_oscillation()){
		double step_size = 0.2;
		unsigned slow_down = 1;

		for(unsigned k = 0; k < app::numObjects(); ++k){
			Grid& workgrid = works[k]->grid();
			Grid::AttachmentAccessor<Vertex, APosition> aaPosWORK(workgrid, aPosition);

			for(unsigned j = 0; j < slow_down; ++j){
				unsigned i = 0;
				for(VertexIterator iter = workgrid.begin<Vertex>();
							iter != workgrid.end<Vertex>(); ++iter){

					ug::vector3 scaled_point_dis;
					VecScale(scaled_point_dis, displacements[k][i], sin(arg_sine));
					VecAdd(aaPosWORK[*iter], scaled_point_dis, aaPosREF[*iter]);
					++i;
				}

				
				works[k]->geometry_changed();
				mode_scenes[k]->object_changed(works[i]);
				QCoreApplication::processEvents();
			}
		}
		arg_sine += step_size;

		QCoreApplication::processEvents();			
	}

	for(unsigned i = 0; i < app::numObjects(); ++i){
		mode_objs[i]->set_visibility(true);
		works[i]->set_visibility(false);
		mode_scenes[i]->remove_object(1);
	}

	std::cout << "oscillation ended." << std::endl;
}

/*
		for(unsigned i = 0; i < app::numScenes(); ++i){
			LGScene* additional_scene = app::getScene(i);
		}
*/
//scene->remove_object((int)scene->num_objects()-1);

