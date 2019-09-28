/*
 * Copyright (c) 2017:  G-CSC, Goethe University Frankfurt
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

#include <QApplication>
#include <QDesktopServices>
#include <QDockWidget>
#include <QFileDialog>
#include <QKeyEvent>

#include "app.h"

#include "mesh_module.h"
#include "scene_inspector.h"
#include "scene/lg_scene.h"
#include "tools/standard_tools.h"
#include "tools/tool_manager.h"
#include "widgets/property_widget.h"
#include "widgets/tool_browser_widget.h"
#include "widgets/widget_list.h"
#include "tools/coordinate_transform_tools.h"

using namespace std;
using namespace ug;

MeshModule::
MeshModule ()
{}

MeshModule::
MeshModule (QWidget* parent) :
	IModule(parent),
	m_sceneInspector(NULL),
	m_scene(NULL)
{}

MeshModule::
~MeshModule ()
{}

void MeshModule::
activate(SceneInspector* sceneInspector, LGScene* scene)
{
	if(m_sceneInspector == sceneInspector && m_scene == scene)
		return;
	
	if(m_sceneInspector || m_scene){
		deactivate();
	}

	m_sceneInspector = sceneInspector;

	if(m_dockWidgets.empty()){
		m_toolManager = new ToolManager(parentWidget());
		try{
			RegisterStandardTools(m_toolManager);
		}
		catch(UGError& err){
			UG_LOG("ERROR: ")
			for(size_t i = 0; i < err.num_msg(); ++i){
				if(i > 0){
					UG_LOG("       ");
				}
				UG_LOG(err.get_msg(i) << endl);
			}
			UG_LOG("------------------------------------------------------------------------------------------\n")
		}


	//	tool browser dock
		QDockWidget* toolBrowserDock = new QDockWidget(tr("Tool Browser"), parentWidget());
		toolBrowserDock->setFeatures(QDockWidget::NoDockWidgetFeatures);
		toolBrowserDock->setObjectName(tr("tool_browser_dock"));

		m_toolBrowser = new ToolBrowser(parentWidget());
		m_toolBrowser->refresh(m_toolManager);
		m_toolBrowser->setObjectName(tr("tool_browser"));
		toolBrowserDock->setWidget(m_toolBrowser);
		m_dockWidgets.push_back(make_pair(Qt::LeftDockWidgetArea, toolBrowserDock));
	}

	if(m_scene != scene){
		m_scene = scene;
	}
}

void MeshModule::
deactivate()
{
	if(m_sceneInspector){
		m_sceneInspector->disconnect(this);
		m_sceneInspector = NULL;
	}

	if(m_scene){
		m_scene->disconnect(this);
		m_scene = NULL;
	}
}


MeshModule::dock_list_t
MeshModule::
getDockWidgets()
{
	return m_dockWidgets;
}

std::vector<QToolBar*>
MeshModule::
getToolBars()
{
	return std::vector<QToolBar*>();
}

std::vector<QMenu*>
MeshModule::
getMenus()
{
	return m_menus;
}


void MeshModule::
keyPressEvent(QKeyEvent* event)
{
	Qt::KeyboardModifiers qtMods = QApplication::keyboardModifiers();
	uint mods = 0;

	if(qtMods.testFlag(Qt::ShiftModifier))
		mods |= SMK_SHIFT;
	if(qtMods.testFlag(Qt::ControlModifier))
		mods |= SMK_CTRL;
	if(qtMods.testFlag(Qt::AltModifier))
		mods |= SMK_ALT;

	m_toolManager->execute_shortcut(event->key(), mods);
}


void MeshModule::refreshCoordinates()
{
	LGObject* obj = app::getActiveObject();
	vector3 center(0, 0, 0);
	if(obj){
	//	calculate the center of the current selection
		Grid::VertexAttachmentAccessor<APosition> aaPos(obj->grid(), aPosition);
		CalculateCenter(center, obj->selector(), aaPos);
	}
}

void MeshModule::coordinatesChanged()
{
	LGObject* obj = app::getActiveObject();
	if(obj){
		vector3 c(0, 0, 0);
		promesh::MoveSelectionTo(obj, c);
		obj->write_selection_to_action_log ();
		obj->log_action(QString("MoveSelectionTo (mesh, Vec3d(%1,%2,%3))\n")
								.arg(c[0], 0, 'g', 12)
								.arg(c[1], 0, 'g', 12)
								.arg(c[2], 0, 'g', 12));
		obj->geometry_changed();
	}
}
