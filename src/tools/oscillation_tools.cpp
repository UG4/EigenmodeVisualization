/*
 * Copyright (c) 2008-2015:  G-CSC, Goethe University Frankfurt
 * Copyright (c) 2006-2008:  Steinbeis Forschungszentrum (STZ Ölbronn)
 * Copyright (c) 2006-2015:  Sebastian Reiter
 * Copyright (c) 2019: Lukas Larisch
 * Author: Sebastian Reiter, Lukas Larisch
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
#include "standard_tools.h"
#include "tooltips.h"

using namespace std;
using namespace ug;

class ToolOscillation : public ITool
{
public:
	void execute(LGObject* obj, QWidget* widget){
		ToolWidget* dlg = dynamic_cast<ToolWidget*>(widget);

		QPixmap originalPixmap;

		unsigned ref_idx = static_cast<unsigned>(dlg->to_int(0));
		unsigned dis_idx_min = static_cast<unsigned>(dlg->to_int(1));
		unsigned dis_idx_max = static_cast<unsigned>(dlg->to_int(2));
		unsigned num_periods = static_cast<unsigned>(dlg->to_int(3));
		double step_size = static_cast<double>(dlg->to_double(4));
		bool take_screenshots = static_cast<bool>(dlg->to_bool(5));
		bool freq_scale = static_cast<bool>(dlg->to_bool(6));
		bool adj_amplitude = static_cast<bool>(dlg->to_bool(7));
		double scale = static_cast<double>(dlg->to_double(8));
		QString Qmetadata = static_cast<QString>(dlg->to_string(9));
		std::string metadata = Qmetadata.toStdString();

		//UG_LOG("ref idx: " << ref_idx << "\n");
		//UG_LOG("dis idx min: " << dis_idx_min << "\n");
		//UG_LOG("dis idx max: " << dis_idx_max << "\n");

		//load metadata
		std::vector<std::vector<ug::vector3> > initial_displacements;

		std::vector<double> freqs;
		std::vector<double> phases;

		LGScene* scene = app::getActiveScene();

		if(metadata.size() > 0){
			unsigned k = (unsigned)scene->num_objects();
			for(unsigned i = 0; i < k; ++i){
				scene->remove_object(0);
			}
		}

		if(metadata.size() > 0){
			std::ifstream fin(metadata);
			if(!fin){
				UG_LOG("ERROR: could not open metadata file " << metadata << "\n");
			}
			std::string line;
			std::string freq;
			std::string phase;
			unsigned i = 0;
			getline(fin,line);

			std::string path = metadata.substr(0, metadata.size()-12);

			LGObject* pObj = app::createEmptyObject("reference", SOT_LG, 1, 0);
			bool bLoadSuccessful = LoadLGObjectFromFile(pObj, (path+line).c_str(), false, 1, 0);

			if(!bLoadSuccessful){
				UG_LOG("ERROR: could not open reference file " << path+line << "\n");
				return;
			}

			pObj->geometry_changed();

			while(getline(fin,line)){
				++i;
				if(i < dis_idx_min){
					continue;
				}

				unsigned pos = line.find(" ");
				std::string name = line.substr(0, pos);
				LGObject* pObj = app::createEmptyObject(name.c_str(), SOT_LG, 1, 0);
				bool bLoadSuccessful = LoadLGObjectFromFile(pObj, (path+name).c_str(), false, 1, 0);

				if(!bLoadSuccessful){
					UG_LOG("ERROR: could not open displacement file " << path+name << "\n");
					return;
				}

				pObj->geometry_changed();
				line = line.substr(pos+1, line.size());

				pos = line.find(" ");
				freq = line.substr(0, pos);
				phase = line.substr(pos+1, line.size());
				UG_LOG(i << ":" << name << " " << freq << " " << phase << std::endl);

				std::cout.precision(freq.size());
				freqs.push_back(std::stod(freq));

				std::cout.precision(phase.size());
				phases.push_back(std::stod(phase));
			}
		}

		if(ref_idx >= (unsigned)scene->num_objects() || dis_idx_max >= (unsigned)scene->num_objects() || dis_idx_min > dis_idx_max){
			UG_LOG("ERROR: illegal index combination\n");
			scene->remove_object(0);
			return;
		}

		for(unsigned i = 0; i < (unsigned)scene->num_objects(); ++i){
			LGObject* o = scene->get_object(i);
			o->set_visibility(false);

		}

		LGObject* ref = scene->get_object(ref_idx);
		Grid& refgrid = ref->grid();

		double max_freq;

		if(freq_scale && metadata.size() > 0){
			max_freq = freqs[dis_idx_max-dis_idx_min];
		}

		//compute displacement
		for(unsigned dis_idx = dis_idx_min; dis_idx <= dis_idx_max; ++dis_idx){
			LGObject* dis = scene->get_object(dis_idx);
			dis->set_visibility(false);

			Grid& disgrid = dis->grid();

			initial_displacements.push_back(std::vector<ug::vector3>());

			AVertex aVrt;

			Grid::AttachmentAccessor<Vertex, APosition> aaPosREF(refgrid, aPosition);

			Grid::AttachmentAccessor<Vertex, AVertex> aaVrtDIS(disgrid, aVrt, true);
			Grid::AttachmentAccessor<Vertex, APosition> aaPosDIS(disgrid, aPosition);


			VertexIterator iterDIS = disgrid.begin<Vertex>();
			VertexIterator iterREF = refgrid.begin<Vertex>();

			ug::vector3 point_dis;

			for(; iterDIS != disgrid.end<Vertex>();){
				VecSubtract(point_dis, aaPosDIS[*iterDIS], aaPosREF[*iterREF]);
				
				if(adj_amplitude){
					point_dis[0] *= phases[dis_idx-dis_idx_min];
					point_dis[1] *= phases[dis_idx-dis_idx_min];
					point_dis[2] *= phases[dis_idx-dis_idx_min];
				}
				point_dis[0] *= scale;
				point_dis[1] *= scale;
				point_dis[2] *= scale;

				initial_displacements[dis_idx-dis_idx_min].push_back(point_dis);
				++iterREF;
				++iterDIS;
			}
		}

		//create new grid which is a copy of disgrid
		AVertex aVrt;

		LGObject* dis = scene->get_object(dis_idx_min);
		Grid& disgrid = dis->grid();

		Grid::AttachmentAccessor<Vertex, APosition> aaPosREF(refgrid, aPosition);

		Grid::AttachmentAccessor<Vertex, AVertex> aaVrtDIS(disgrid, aVrt, true);
		Grid::AttachmentAccessor<Vertex, APosition> aaPosDIS(disgrid, aPosition);

		LGObject* work = app::createEmptyObject("oscillation", SOT_LG, 1, 0);
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

		ref->set_visibility(false);
		work->set_visibility(true);

		scene->object_changed(work);
		work->geometry_changed();

		ug::Sphere3 s = work->get_bounding_sphere();

		app::getMainWindow()->getView3D()->fly_to(
							cam::vector3(s.get_center().x(),
										s.get_center().y(),
										s.get_center().z()),
							s.get_radius() * 4.f + 0.001);

		//start oscillation
		double arg_sine = 0.0;
		unsigned k = 0;

		if(take_screenshots){
			std::string cmd = "rm ../screenshots/*";
			system(cmd.c_str());
		}

		while(unsigned(arg_sine/3.1415) < num_periods*2){
			unsigned i = 0;
			for(VertexIterator iter = workgrid.begin<Vertex>();
						iter != workgrid.end<Vertex>(); ++iter){

				ug::vector3 tmp;
				ug::vector3 tmp2;

				VecAdd(aaPosWORK[*iter], aaPosREF[*iter], ug::vector3());

				for(unsigned j = 0; j < initial_displacements.size(); ++j){
					ug::vector3 scaled_point_dis;

					if(freq_scale && metadata.size() > 0){
						//VecScale(scaled_point_dis, initial_displacements[j][i], arg_sine*(freqs[j]/freq_max));
						//VecScale(scaled_point_dis, initial_displacements[j][i], sin(arg_sine));
						VecScale(scaled_point_dis, initial_displacements[j][i], sin(arg_sine*freqs[j]/max_freq));
					}
					else{
						VecScale(scaled_point_dis, initial_displacements[j][i], sin(arg_sine));
					}
					VecAdd(aaPosWORK[*iter], scaled_point_dis, aaPosWORK[*iter]);
				}
				++i;
				
			}

			arg_sine += step_size;

			//screenshot
			if(take_screenshots){
				//std::string tfilename = "./../screenshots/screenshot_" + std::string(k) + ".png";
				QString filename = QString("./../screenshots/screenshot_%1.png").arg(k);
				QScreen *screen = QGuiApplication::primaryScreen();
				originalPixmap = screen->grabWindow(0, 370, 130, 1290, 730); //use, TODO screengeometry
				originalPixmap.save(filename);
			}

			k++;

			scene->object_changed(work);		
			work->geometry_changed();

			QCoreApplication::processEvents();
		}

		if(take_screenshots){
			QString path = "./../videos/";
			QString fileName = QFileDialog::getSaveFileName(
										widget,
										tr("Save Video"),
										path,
										tr("videos (.mp4)"));

			std::string fin = "../screenshots/";
			std::string fout = fileName.toStdString();
			std::string cmd = "python ./../video_converter/video_converter_mp4.py " + fin + " " + fout;
			system(cmd.c_str());
		}


		scene->object_changed(work);
		work->geometry_changed();

		QCoreApplication::processEvents();

		if(metadata.size() > 0){
			unsigned k = (unsigned)scene->num_objects();
			for(unsigned i = 0; i < k; ++i){
				scene->remove_object(0);
			}
		}
		else{
			scene->remove_object((int)scene->num_objects()-1);
		}
	}

	const char* get_name()		{return "Visualize";}
	const char* get_tooltip()	{return "";}
	const char* get_group()		{return "Oscillation";}

	ToolWidget* get_dialog(QWidget* parent){
		ToolWidget *dlg = new ToolWidget(get_name(), parent, this,
								IDB_APPLY | IDB_OK | IDB_CLOSE);

		dlg->addSpinBox("reference grid: ", 0, 10, 0, 1, 0);
		dlg->addSpinBox("first displacement grid idx: ", 1, 10, 0, 1, 0);
		dlg->addSpinBox("last displacement grid idx: ", 1, 10, 0, 1, 0);
		dlg->addSpinBox("num periods: ", 1, 10, 1, 1, 0);
		dlg->addSpinBox("step size: ", 0.01, 0.50, 0.05, 0.01, 2);
		dlg->addCheckBox("capture screenshots", false);
		dlg->addCheckBox("scale with freq", false);
		dlg->addCheckBox("adjust amplitude", false);
		dlg->addSpinBox("scale: ", 0.1, 10000.0, 1.0, 0.1, 1);
		dlg->addFileBrowser("", FWT_OPEN, "*.txt");
;

		return dlg;
	}
};

void RegisterOscillationTools(ToolManager* toolMgr)
{
	toolMgr->register_tool(new ToolOscillation);
}

