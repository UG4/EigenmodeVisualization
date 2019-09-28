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
#include <QPlainTextEdit>
#include <QScreen>
#include <QFileDialog>
#include <math.h>
#include <stdlib.h>
#include "app.h"
#include "standard_tools.h"
#include "tooltips.h"
#include "UG_LogParser.h"

using namespace std;
using namespace ug;

class ToolSolutionRefinement : public ITool
{
public:
	void execute(LGObject* obj, QWidget* widget){
		ToolWidget* dlg = dynamic_cast<ToolWidget*>(widget);

		QPixmap originalPixmap;

		unsigned ref_idx = static_cast<unsigned>(dlg->to_int(0));
		unsigned dis_idx_min = static_cast<unsigned>(dlg->to_int(1));
		unsigned dis_idx_max = static_cast<unsigned>(dlg->to_int(2));
		double step_size = static_cast<double>(dlg->to_double(3));
		bool take_screenshots = static_cast<bool>(dlg->to_bool(4));
		QString Qmetadata = static_cast<QString>(dlg->to_string(5));
		std::string metadata = Qmetadata.toStdString();

		//load metadata
		std::vector<std::vector<ug::vector3> > initial_displacements;

		LGScene* scene = app::getActiveScene();


		if(metadata.size() > 0){
			UG_LogParser LP(metadata);
			LP.do_it();


		}
#ifdef NO
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

			LGObject* pObj = app::createEmptyObject("reference", SOT_LG);
			bool bLoadSuccessful = LoadLGObjectFromFile(pObj, (path+line).c_str(), false);

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
				LGObject* pObj = app::createEmptyObject(name.c_str(), SOT_LG);
				bool bLoadSuccessful = LoadLGObjectFromFile(pObj, (path+name).c_str(), false);

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
#endif

		if(dis_idx_max >= (unsigned)scene->num_objects() || dis_idx_min > dis_idx_max){
			UG_LOG("ERROR: illegal index combination\n");
			return;
		}

		for(unsigned i = 0; i < (unsigned)scene->num_objects(); ++i){
			LGObject* o = scene->get_object(i);
			o->set_visibility(false);

		}

		LGObject* ref = scene->get_object(ref_idx);
		ref->set_visibility(true);
		ref->set_color(QColor(Qt::red));
		ref->set_subset_color(0, QColor(Qt::red));

		for(unsigned i = dis_idx_min; i < dis_idx_max; ++i){
			LGObject* o = scene->get_object(i);

			for(unsigned j = 0; j < 10; ++j){
				o->set_visibility(true);
				o->geometry_changed();
				scene->object_changed(o);
				QCoreApplication::processEvents();
			}

			o->set_visibility(false);
			scene->object_changed(o);
		}

		//app::getMainWindow()->m_statisticsLog->insertPlainText("hallo");
	}

	const char* get_name()		{return "Visualize";}
	const char* get_tooltip()	{return "";}
	const char* get_group()		{return "SolutionRefinement";}

	ToolWidget* get_dialog(QWidget* parent){
		ToolWidget *dlg = new ToolWidget(get_name(), parent, this,
								IDB_APPLY | IDB_OK | IDB_CLOSE);

		dlg->addSpinBox("reference grid idx: ", 0, 10, 0, 1, 0);
		dlg->addSpinBox("first displacement grid idx: ", 0, 10, 0, 1, 0);
		dlg->addSpinBox("last displacement grid idx: ", 1, 10, 0, 1, 0);
		dlg->addSpinBox("step size: ", 0.01, 0.50, 0.05, 0.01, 2);
		dlg->addCheckBox("capture screenshots", false);
		dlg->addFileBrowser("", FWT_OPEN, "*.txt");

		return dlg;
	}
};

void RegisterSolutionRefinementTools(ToolManager* toolMgr)
{
	toolMgr->register_tool(new ToolSolutionRefinement);
}

