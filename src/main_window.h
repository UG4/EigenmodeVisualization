/*
 * Copyright (c) 2008-2015:  G-CSC, Goethe University Frankfurt
 * Copyright (c) 2006-2008:  Steinbeis Forschungszentrum (STZ Ölbronn)
 * Copyright (c) 2006-2015:  Sebastian Reiter
 * Author: Sebastian Reiter, Lukas Larisch
 *
 * This file is part of EmVis.
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

#ifndef __H__MAIN_WINDOW__
#define __H__MAIN_WINDOW__

#include <QMainWindow>
#include <QModelIndex>
#include <QSplitter>
#include <QRadioButton>
#include <QLabel>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QGroupBox>
#include <QComboBox>
#include <QCheckBox>
#include <QSettings>
#include <QSpinBox>
#include "color_widget.h"
#include "scene/lg_object.h"
#include "view3d/view3d.h"
#include "scene/lg_scene.h"
#include "scene_inspector.h"
#include "modules/module_interface.h"
#include <boost/filesystem.hpp>

////////////////////////////////////////////////////////////////////////
//	predeclarations
class View3D;
class LGScene;
class ISceneObject;

class QAction;
class QComboBox;
class QFileDialog;
class QHelpBrowser;
class QPushButton;
class QPoint;
class QPlainTextEdit;
class QTreeView;
class QToolBar;
class QToolButton;
class PropertyWidget;
class SceneInspector;
class ToolManager;
class ToolBrowser;
class QScriptEditor;
class TruncatedDoubleSpinBox;


enum SceneObjectType {
	SOT_LG,
	SOT_CSG
};

////////////////////////////////////////////////////////////////////////
///	the main window.
/**
 * This class organizes the toolbars, menus, views and tool-windows
 * of the application.
 */
class MainWindow : public QMainWindow
{
	Q_OBJECT

	protected:
		enum MouseMoveAction{
			MMA_DEFAULT = TT_NONE,
			MMA_GRAB = TT_GRAB,
			MMA_ROTATE = TT_ROTATE,
			MMA_SCALE = TT_SCALE
		};

		enum Axis{
			X_AXIS = 1,
			Y_AXIS = 1<<1,
			Z_AXIS = 1<<2
		};

	public:
		MainWindow();
		~MainWindow();

		void init();

		LGScene* get_scene()	{return m_scene;}
		LGScene* get_scene(unsigned idx)	{return m_scenes[idx];}

		bool load_grid_from_file(const char* filename, unsigned screen=1, unsigned idx=0);
        LGObject* create_empty_object(const char* name, SceneObjectType sot, unsigned screen=1, unsigned idx=0);
		inline QSettings& settings()	{return m_settings;}

		LGObject* getActiveObject();
		View3D*	getView3D()					{return m_pView;}
		View3D*	getViews3D(unsigned idx)					{return m_pViews[idx];}
		View3D*	getView3DIteration(unsigned idx)					{return m_pView_iterations;}
		SceneInspector* getSceneInspector()	{return m_sceneInspector;}

		void launchHelpBrowser(const QString& pageName);
		
		void check_options() const;
		
		const char* log_text();
		
	signals:
		void activeObjectChanged();
		void refreshToolDialogs();

	public slots:
		void setActiveObject(int index);
		void newGeometry();
		int openFile();///< returns the number of successfully opened files.
		bool openDataset();///< returns the number of successfully opened files.
		void oscillating_toggled(bool b);
		void eigenmodeSpinBox_activated(int);
		void iterationSpinBox_activated(int);
		void quit();

	protected slots:
		void frontDrawModeChanged(int newMode);
		void backDrawModeChanged(int newMode);
		void backgroundColorChanged(const QColor& color);
		void view3dKeyReleased(QKeyEvent* event);
		void elementDrawModeChanged();
		void sceneInspectorClicked(QMouseEvent* event);

	protected:
		void closeEvent(QCloseEvent *event);

		void dragEnterEvent(QDragEnterEvent* event);
		void dropEvent(QDropEvent* event);

		void mousePressEvent(QMouseEvent* event);
		void mouseMoveEvent(QMouseEvent* event);
		void mouseReleaseEvent(QMouseEvent* event);
		void keyPressEvent(QKeyEvent* event);

		QToolBar* createVisibilityToolbar();

		uint getLGElementMode();

		void beginMouseMoveAction(MouseMoveAction mma);
		void updateMouseMoveAction();
		void endMouseMoveAction(bool bApply);

	///	casts a ray to the camera-focus plane and places the vertex there
	/**	The method can be fine-tuned through options.drawPath.*/
		void insertVertexAtScreenCoord(number x, number y);

		void populateMenuBar ();
		void activateModule (IModule* mod);
		
	public: //protected:
	//	3d view
		View3D*		m_pView;
		LGScene*	m_scene;

		std::vector<View3D*> m_pViews;
		std::vector<LGScene*> m_scenes;

		View3D*		m_pView_iterations;
		LGScene*	m_scene_iterations;

	//	Modules
		IModule*				m_activeModule;
		IModule::dock_list_t	m_moduleDockWidgets;
		std::vector<QMenu*>		m_moduleMenus;

	//	tools
		ColorWidget*	m_bgColor;
		QSettings		m_settings;

	//	state-variables
		int m_elementModeListIndex;
		int m_mouseMoveAction;

	//	important for selection etc
		QPoint m_mouseDownPos;
		QPoint m_mouseMoveActionStart;
		LGObject* m_mouseMoveActionObject;///< Only valid if m_mouseMoveAction != MMA_DEFAULT
		unsigned int m_activeAxis;

	//	use it as seldom as possible. It is mainly used to trigger the signal activeObjectChanged.
		LGObject*	m_activeObject;
		LGObject*	m_actionLogSender;

	//	dialogs
		QFileDialog*				m_dlgGeometryFiles;
		SceneInspector*				m_sceneInspector;
		std::vector<SceneInspector*> m_sceneInspectors;

		QDialog*					m_dlgAbout;
		QDockWidget*				m_pLog;
		QDockWidget* 				actionLogDock;
		QPlainTextEdit*				m_pLogText;
		QPlainTextEdit*				m_actionLog;

		QSpinBox*					iterationSpinBox;
		QSpinBox*					eigenmodeSpinBox;

		QLabel*						iterationSpinBoxLabel;
		QLabel*						eigenmodeSpinBoxLabel;

		std::vector<QGridLayout*>	gridLayouts;
		QHBoxLayout*				m_hbox;

 		QStackedWidget*				stackedWidget;
		QVBoxLayout*				layout;
		QComboBox*					pageComboBox;
		QComboBox*					dataComboBox;

		QCheckBox*					oscillatingCheckBox;

		QLabel*				m_picture;
		QLabel*				m_picture2;

		unsigned			m_modus;
		unsigned 			m_num_objects;
		unsigned 			m_num_iters;
		unsigned 			m_num_evs;
		bool				m_oscillate;
		bool				m_dataset_loaded;
		unsigned 			m_eigenmode_selection;
		unsigned 			m_iteration_selection;
		unsigned			m_last_screen3_obj_idx;

		std::vector<std::vector<double> > m_lambdas;
		std::vector<std::vector<double> > m_defects;
		std::vector<double> m_frequencies;


	//	menus
		QMenu* m_fileMenu;
	//	actions
		QAction*	m_actOpen;
		QAction*	m_actOpenDataset;
		QAction*	m_actExport;
		QAction*	m_actQuit;

		std::vector<QWidget*>	gridWidgets;
};

#endif // __H__MAIN_WINDOW__
