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

typedef std::vector<double> dom1d;
typedef std::vector<std::vector<double> > dom2d;
typedef std::vector<std::vector<std::vector<double> > > dom3d;
typedef std::vector<std::vector<std::vector<std::vector<double> > > > dom4d;

inline unsigned myatoi(std::string line, unsigned& v, char end=0){
	unsigned idx = 0;
	v = line[idx]-'0';
	++idx;
	while(line[idx]!=end && idx < line.size()){
		v *= 10;
		v += line[idx]-'0';
		++idx;
	}
	return idx;
}

class ToolHelmholtz : public ITool
{
public:
	void execute(LGObject* obj, QWidget* widget){
		ToolWidget* dlg = dynamic_cast<ToolWidget*>(widget);

		unsigned ref_idx = static_cast<unsigned>(dlg->to_int(0));
		double scale = static_cast<double>(dlg->to_double(1));
		QString Qdata_file = static_cast<QString>(dlg->to_string(2));
		std::string data_file = Qdata_file.toStdString();

		if(data_file.size() == 0){
			LGScene* scene = app::getActiveScene();
			obj = scene->get_object(ref_idx);
			Grid& grid = obj->grid();

			Grid::AttachmentAccessor<Vertex, APosition> aaPos(grid, aPosition);

			for(VertexIterator iter = grid.begin<Vertex>();
						iter != grid.end<Vertex>(); ++iter)
			{
				aaPos[*iter][2] *= scale;
			}

			scene->object_changed(obj);		
			obj->geometry_changed();
			return;
		}

		//load data
		std::ifstream fin(data_file);
		if(!fin){
			UG_LOG("ERROR: could not open data file " << data_file << "\n");
		}
		std::string line;
		getline(fin,line);

		unsigned pos = line.find(" ");
		std::string s_dim = line.substr(0, pos);
		line = line.substr(pos+1, line.size()-1);
		pos = line.find(" ");
		pos = line.find("\n");
		std::string s_num_space = line.substr(0, pos);

		unsigned dim;
		unsigned num_space;

		myatoi(s_dim, dim);
		myatoi(s_num_space, num_space);

		if(!(dim == 1 || dim == 2)){
			UG_LOG("ERROR: dim must be 1 or 2\n");
			return;
		}

		std::vector<double> data(num_space);

		std::string token;
		unsigned k = 0;
		getline(fin,line);
		for(unsigned j = 0; j < num_space; ++j){
			pos = line.find(" ");
			token = line.substr(0, pos);
			line = line.substr(pos+1, line.size()-1);
			std::cout.precision(token.size());
			data[k] = std::stod(token);
			++k;
		}

		LGScene* scene = app::getActiveScene();
		obj = scene->get_object(ref_idx);
		Grid& grid = obj->grid();

		Grid::AttachmentAccessor<Vertex, APosition> aaPos(grid, aPosition);

		unsigned j = 0;
		for(VertexIterator iter = grid.begin<Vertex>();
					iter != grid.end<Vertex>(); ++iter){

			switch(dim){
				case 1:
					aaPos[*iter][1] = scale*data[j];
					break;
				case 2: //TODO
					aaPos[*iter][2] = scale*data[j];
					break;
			}
			++j;
		}

		scene->object_changed(obj);		
		obj->geometry_changed();

		QCoreApplication::processEvents();
	}

	const char* get_name()		{return "Visualize";}
	const char* get_tooltip()	{return "";}
	const char* get_group()		{return "Helmholtz";}

	ToolWidget* get_dialog(QWidget* parent){
		ToolWidget *dlg = new ToolWidget(get_name(), parent, this,
								IDB_APPLY | IDB_OK | IDB_CLOSE);

		dlg->addSpinBox("reference grid: ", 0, 10, 0, 1, 0);
		dlg->addSpinBox("scale: ", 0.1, 10000.0, 1.0, 0.1, 1);
		dlg->addFileBrowser("data", FWT_OPEN, "*.txt");

		return dlg;
	}
};

void RegisterHelmholtzTools(ToolManager* toolMgr)
{
	toolMgr->register_tool(new ToolHelmholtz);
}

