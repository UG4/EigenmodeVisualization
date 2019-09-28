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

#include <iostream>
#include <QtWidgets>
#include <QDesktopServices>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include "main_window.h"
#include "view3d/view3d.h"
#include "scene/lg_scene.h"
#include "scene/csg_object.h"
#include "scene_inspector.h"
#include "scene_item_model.h"
#include "QDebugStream.h"
#include "lib_grid/lib_grid.h"
#include "lib_grid/file_io/file_io_ug.h"
#include "lib_grid/file_io/file_io_ugx.h"
#include "lib_grid/file_io/file_io_lgb.h"
#include <string>
#include "app.h"
#include "common/util/file_util.h"
#include "bridge/bridge.h"
#include "common/util/path_provider.h"
#include "common/util/plugin_util.h"
#include "util/file_util.h"
#include "modules/mesh_module.h"
#include "widgets/property_widget.h"
#include "widgets/truncated_double_spin_box.h"
#include "widgets/widget_list.h"
#include "tools/UG_LogParser.h"
#include <boost/filesystem.hpp>
#include "oscillation/oscillation.cpp"

//tmp
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPixmap>

using namespace std;
using namespace ug;
using namespace boost::filesystem;

#define EXTRASCENES 4
#define MAXITERS 5
#define MAXEVS 4

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
//	implementation of MainWindow

////////////////////////////////////////////////////////////////////////
//	constructor
MainWindow::MainWindow() :
	m_activeModule (NULL),
	m_settings(),
	m_elementModeListIndex(3),
	m_mouseMoveAction(MMA_DEFAULT),
	m_activeAxis(X_AXIS | Y_AXIS | Z_AXIS),
	m_activeObject(NULL),
	m_actionLogSender(NULL),
	#ifdef PROMESH_USE_WEBKIT
		m_helpBrowser(NULL),
	#endif
	m_dlgAbout(NULL),
	m_modus(1),
	m_num_objects(0),
	m_oscillate(false),
	m_dataset_loaded(false),
	m_eigenmode_selection(0),
	m_iteration_selection(0),
	m_last_screen3_obj_idx(0)
{
}

void MainWindow::init()
{ 
	setObjectName(tr("main_window"));
	setAcceptDrops(true);
	// setGeometry(0, 0, 1024, 600);


//	create view and scene
	m_pView = new View3D;

	for(unsigned i = 0; i < 8; ++i){
		m_pViews.push_back(new View3D);
	}

	m_pView_iterations = new View3D;

	setCorner( Qt::TopLeftCorner, Qt::LeftDockWidgetArea );
    setCorner( Qt::TopRightCorner, Qt::RightDockWidgetArea );
    setCorner( Qt::BottomLeftCorner, Qt::LeftDockWidgetArea );
    setCorner( Qt::BottomRightCorner, Qt::RightDockWidgetArea );

	m_scene = new LGScene;

	for(unsigned i = 0; i < EXTRASCENES; ++i){
		m_scenes.push_back(new LGScene);
	}

	m_scene_iterations = new LGScene;

	m_pView->set_renderer(m_scene);
	connect(m_scene, SIGNAL(visuals_updated()),
			m_pView, SLOT(update()));

	for(unsigned i = 0; i < EXTRASCENES; ++i){
		m_pViews[i]->set_renderer(m_scenes[i]);
		m_pViews[i]->set_background_color(QColor(Qt::white));
		connect(m_scenes[i], SIGNAL(visuals_updated()),
				m_pViews[i], SLOT(update()));
	}

	m_pView_iterations->set_renderer(m_scene_iterations);
	connect(m_scene_iterations, SIGNAL(visuals_updated()),
			m_pView_iterations, SLOT(update()));


	gridLayouts.push_back(new QGridLayout);
    gridLayouts[0]->addWidget(m_pViews[0],0,0,1,1);
    gridLayouts[0]->addWidget(m_pViews[1],0,1,1,1);
    gridLayouts[0]->addWidget(m_pViews[2],1,0,1,1);
    gridLayouts[0]->addWidget(m_pViews[3],1,1,1,1);

    gridWidgets.push_back(new QWidget());
    gridWidgets[0]->setLayout(gridLayouts[0]);

	stackedWidget = new QStackedWidget;
    stackedWidget->addWidget(m_pView);
    stackedWidget->addWidget(gridWidgets[0]);
    stackedWidget->addWidget(m_pView_iterations);


  	pageComboBox = new QComboBox;
    pageComboBox->addItem(tr("Single View"));
    pageComboBox->addItem(tr("Split View"));
    pageComboBox->addItem(tr("Iteration View"));
    connect(pageComboBox, QOverload<int>::of(&QComboBox::activated),
            stackedWidget, &QStackedWidget::setCurrentIndex);

	oscillatingCheckBox = new QCheckBox("Oscillate", this);
	oscillatingCheckBox->setChecked(false);

  	dataComboBox = new QComboBox;
    dataComboBox->addItem(tr("Eigenmode"));
    dataComboBox->addItem(tr("Residuum"));
    dataComboBox->addItem(tr("Correction"));
    dataComboBox->addItem(tr("Modal analysis"));

    connect(oscillatingCheckBox, SIGNAL(toggled(bool)), this, SLOT(oscillating_toggled(bool)));

	eigenmodeSpinBox = new QSpinBox;

    eigenmodeSpinBox->setRange(1, 1);
    eigenmodeSpinBox->setValue(1);

	connect(eigenmodeSpinBox, SIGNAL(valueChanged(int)), this, SLOT(eigenmodeSpinBox_activated(int)));

	iterationSpinBox = new QSpinBox;

    iterationSpinBox->setRange(0, 0);
    iterationSpinBox->setValue(0);

	connect(iterationSpinBox, SIGNAL(valueChanged(int)), this, SLOT(iterationSpinBox_activated(int)));	

	iterationSpinBoxLabel = new QLabel;
	iterationSpinBoxLabel->setText(tr(" iteration: "));

	eigenmodeSpinBoxLabel = new QLabel;
	eigenmodeSpinBoxLabel->setText(tr(" eigenmode: "));

	setCentralWidget(stackedWidget);

	setTabPosition(Qt::BottomDockWidgetArea, QTabWidget::West);


//	create the log widget
	m_pLog = new QDockWidget(tr("log"), this);
	m_pLog->setFeatures(QDockWidget::NoDockWidgetFeatures);
	m_pLog->setObjectName(tr("log"));

	QFont logFont("unknown");
	logFont.setStyleHint(QFont::Monospace);
	logFont.setPointSize(10);
	m_pLogText = new QPlainTextEdit(m_pLog);
	m_pLogText->setReadOnly(true);
	m_pLogText->setWordWrapMode(QTextOption::NoWrap);
	m_pLogText->setFont(logFont);
	m_pLog->setWidget(m_pLogText);

	addDockWidget(Qt::BottomDockWidgetArea, m_pLog);

	m_picture = new QLabel();
	//QPixmap pic("/home/idot/projects/emvis/images/a.png");
	//pic = pic.scaled(100, 100);
	//m_picture->setFixedHeight(100);
	//m_picture->setFixedWidth(800);
	//m_picture->setScaledContents(true); 	
	//m_picture->setPixmap(pic);


	QDockWidget* statisticsDock = new QDockWidget(tr("statistics"), this);
	statisticsDock->setFeatures(QDockWidget::NoDockWidgetFeatures);
	statisticsDock->setObjectName(tr("statisticsLog"));

	statisticsDock->setWidget(m_picture);
	addDockWidget(Qt::BottomDockWidgetArea, statisticsDock);
	tabifyDockWidget(statisticsDock, m_pLog);



//	redirect cout
	Q_DebugStream* pDebugStream = new Q_DebugStream(GetLogAssistant().logger(), m_pLogText);
	pDebugStream->enable_file_output(app::UserDataDir().path() + QString("/log.txt"));

	try{
		ug::bridge::InitBridge();
		if(!ug::LoadPlugins(ug::PathProvider::get_path(PLUGIN_PATH).c_str(), "ug4/", ug::bridge::GetUGRegistry()))
		{
			UG_LOG("ERROR during initialization of plugins: LoadPlugins failed!\n");
		}
	}
	catch(ug::UGError& err){
		UG_LOG("ERROR during initialization of ug::bridge:\n");
		for(size_t i = 0; i < err.num_msg(); ++i){
			UG_LOG("  " << err.get_msg(i) << std::endl);
		}
	}


//	file-menu
	m_actOpen = new QAction(tr("&Open"), this);
	m_actOpen->setIcon(QIcon(":images/fileopen.png"));
	m_actOpen->setShortcut(tr("Ctrl+O"));
	m_actOpen->setToolTip(tr("Load a geometry from file."));
	connect(m_actOpen, SIGNAL(triggered()), this, SLOT(openFile()));

	m_actOpenDataset = new QAction(tr("&Open Eigenmode Dataset"), this);
	m_actOpenDataset->setIcon(QIcon(":images/fileopen.png"));
	m_actOpenDataset->setShortcut(tr("Ctrl+P"));
	m_actOpenDataset->setToolTip(tr("Load an Eigenmode dataset from directory."));
	connect(m_actOpenDataset, SIGNAL(triggered()), this, SLOT(openDataset()));

	m_actQuit = new QAction(tr("Quit"), this);
	connect(m_actQuit, SIGNAL(triggered()), this, SLOT(quit()));

	m_fileMenu = new QMenu("&File", menuBar());
	m_fileMenu->addAction(m_actOpen);
	m_fileMenu->addAction(m_actOpenDataset);
	m_fileMenu->addSeparator();
	m_fileMenu->addSeparator();
	m_fileMenu->addAction(m_actQuit);

//	create a tool bar for file handling
	QToolBar* fileToolBar = addToolBar(tr("&File"));
	fileToolBar->setObjectName(tr("file_toolbar"));
	fileToolBar->addAction(m_actOpen);
	fileToolBar->addAction(m_actOpenDataset);

//	create a tool bar for visibility
	createVisibilityToolbar();

//	create the file dialog.
	m_dlgGeometryFiles = new QFileDialog(this);


//////// DOCK WIDGETS
	setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);
	setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);

//	create the scene inspector
	QDockWidget* pSceneInspectorDock = new QDockWidget(tr("Scene Inspector"), this);
	pSceneInspectorDock->setFeatures(QDockWidget::NoDockWidgetFeatures);
	pSceneInspectorDock->setObjectName(tr("scene_inspector_dock"));

	m_sceneInspector = new SceneInspector(pSceneInspectorDock);
	m_sceneInspector->setObjectName(tr("scene_inspector"));
	m_sceneInspector->setScene(m_scene);
	//m_sceneInspector->setScene(m_scene_iterations);

	pSceneInspectorDock->setWidget(m_sceneInspector);
	addDockWidget(Qt::RightDockWidgetArea, pSceneInspectorDock);

	connect(m_sceneInspector, SIGNAL(mouseClicked(QMouseEvent*)),
			this, SLOT(sceneInspectorClicked(QMouseEvent*)));

	populateMenuBar ();
	activateModule(new MeshModule(this));

	resize(settings().value("mainWindow/size", QSize(1024, 768)).toSize());
	move(settings().value("mainWindow/pos", QPoint(10, 10)).toPoint());
	restoreState(settings().value("mainWindow/windowState").toByteArray());

	m_pLog->raise();
	
//	init the status bar
	// statusBar()->show();

	show();
}

MainWindow::~MainWindow()
{
}

QToolBar* MainWindow::createVisibilityToolbar()
{
	QToolBar* visToolBar = addToolBar(tr("&Visibility"));
	visToolBar->setObjectName(tr("visibility_toolbar"));

//	visBack->setCurrentIndex(3);

//	add a color-picker for the background color
	visToolBar->addSeparator();

//	layer for color:
	QLabel* lblColor = new QLabel(visToolBar);
	lblColor->setText(tr(" bg-color: "));
	visToolBar->addWidget(lblColor);

	m_bgColor = new ColorWidget(visToolBar);
	visToolBar->addWidget(m_bgColor);
	connect(m_bgColor, SIGNAL(colorChanged(QColor)),
			this, SLOT(backgroundColorChanged(QColor)));

	m_bgColor->setFixedWidth(24);
	m_bgColor->setFixedHeight(24);

	QString strDefColor("#666666");
	if(settings().contains("bg-color")){
		QVariant value = settings().value("bg-color", strDefColor);
		m_bgColor->setColor(QColor(value.toString()));
	}
	else
		m_bgColor->setColor(QColor(strDefColor));

	visToolBar->addSeparator();
	visToolBar->addSeparator();

	visToolBar->addWidget(pageComboBox);

	visToolBar->addSeparator();
	visToolBar->addSeparator();

	visToolBar->addWidget(oscillatingCheckBox);

	visToolBar->addSeparator();
	visToolBar->addSeparator();

	visToolBar->addWidget(dataComboBox);

	visToolBar->addSeparator();
	visToolBar->addSeparator();

	visToolBar->addWidget(eigenmodeSpinBoxLabel);

	visToolBar->addWidget(eigenmodeSpinBox);

	visToolBar->addSeparator();
	visToolBar->addSeparator();

	visToolBar->addWidget(iterationSpinBoxLabel);

	visToolBar->addWidget(iterationSpinBox);

	return visToolBar;
}

uint MainWindow::getLGElementMode()
{
	switch(m_elementModeListIndex){
		case 0: return LGEM_VERTEX;
		case 1: return LGEM_EDGE;
		case 2: return LGEM_FACE;
		case 3: return LGEM_VOLUME;
		default: return LGEM_NONE;
	}
}

bool MainWindow::load_grid_from_file(const char* filename, unsigned screen, unsigned idx)
{
	try{
		LGObject* pObj = CreateLGObjectFromFile(filename, screen, idx);

	//	add it to the scene
		if(pObj)
		{
			bool bFirstLoad;

			if(screen == 1){
				bFirstLoad = m_scene->num_objects() == 0;
			}
			else if(screen == 2){
				bFirstLoad = m_scenes[idx]->num_objects() == 0;
			}
			else if(screen == 3){
				bFirstLoad = m_scene_iterations->num_objects() == 0;
			}
			else{
				std::cout << "invalid screen " << screen << std::endl;
			}

			pObj->set_element_mode(getLGElementMode());

			int index;
			if(screen == 1){
				index = m_scene->add_object(pObj);
			}
			else if(screen == 2){
				//TODO cleanup. 
				m_sceneInspector->setScene(m_scenes[idx]);
				index = m_scenes[idx]->add_object(pObj);
				pObj->set_visibility(false);
				pObj->geometry_changed();
				m_scenes[idx]->object_changed(pObj);
				pObj->set_visibility(true);
				pObj->geometry_changed();
				m_scenes[idx]->object_changed(pObj);

				m_scenes[idx]->update_visuals();
				m_scenes[idx]->update_visuals(pObj);
				m_scenes[idx]->object_changed(pObj);

				for(unsigned i = 0; i < m_num_objects; ++i){
					setActiveObject(i);
					emit activeObjectChanged();
				}

				setActiveObject(index);
			
				//this->repaint();
				//w->repaint();
				//stackedWidget->repaint();
			}
			else if(screen == 3){
				m_sceneInspector->setScene(m_scene_iterations);
				index = m_scene_iterations->add_object(pObj);
				m_scene_iterations->update_visuals(pObj);
				m_scene_iterations->object_changed(pObj);
				std::cout << "added object to m_scene_iterations" << std::endl;
			}
			else{
				std::cout << "invalid screen " << screen << std::endl;
			}

			pObj->set_visibility(true);
			pObj->geometry_changed();


			if(index != -1)
			{
				setActiveObject(index);

			//	if this is the first object loaded, we will focus it.
				if(bFirstLoad)
				{
					ug::Sphere3 s = pObj->get_bounding_sphere();
					if(screen == 1){
						m_pView->fly_to(cam::vector3(s.get_center().x(),
														s.get_center().y(),
														s.get_center().z()),
										s.get_radius() * 3.f);
					}
					else if(screen == 2){
						m_pViews[idx]->fly_to(cam::vector3(s.get_center().x(),
														s.get_center().y(),
														s.get_center().z()),
										s.get_radius() * 3.f);
					}
					else if(screen == 3){
						m_pView_iterations->fly_to(cam::vector3(s.get_center().x(),
														s.get_center().y(),
														s.get_center().z()),
										s.get_radius() * 3.f);
					}
					else{
						std::cout << "invalid screen " << screen << std::endl;
					}
				}

				return true;
			}
		}
	}
	catch(UGError err){
		UG_LOG("ERROR: " << err.get_msg() << endl);
		return false;
	}
	catch(std::runtime_error err){
		UG_LOG("ERROR: " << err.what() << endl);
		return false;
	}

	return false;
}

LGObject* MainWindow::create_empty_object(const char* name, SceneObjectType sot, unsigned screen, unsigned idx)
{
//	create a new object
	LGObject* pObj = NULL;

	switch(sot){
		case SOT_LG:
    		pObj = CreateEmptyLGObject(name);
    		break;

    	case SOT_CSG:
    		pObj = CreateEmptyCSGObject(name);
	}

	UG_COND_THROW(!pObj, "Invalid SceneObjectType specified!");

    pObj->set_element_mode(getLGElementMode());

//	add it to the scene

	int index;
	if(screen == 1){
		index = m_scene->add_object(pObj);
	}
	else if(screen == 2){
		index = m_scenes[idx]->add_object(pObj);
	}
	else if(screen == 3){
		index = m_scene_iterations->add_object(pObj);
	}
	else{
		std::cout << "invalid screen " << screen << std::endl;
	}

/*
	for(unsigned i = 0; i < m_scenes.size(); ++i){
		index = m_scenes[i]->add_object(pObj);
	}
*/
	if(index != -1)
		setActiveObject(index);

	pObj->geometry_changed();
	return pObj;
}


////////////////////////////////////////////////////////////////////////
//	public slots
void MainWindow::newGeometry()
{
	create_empty_object("newObject", SOT_LG);
}

void MainWindow::iterationSpinBox_activated(int a){
	if(a < 0 || a >= (int)m_num_iters){
		return;
	}

	m_iteration_selection = a;

	if(m_dataset_loaded){
		LGObject* ref_obj = m_scene_iterations->get_object(m_last_screen3_obj_idx);
		ref_obj->set_visibility(false);
		ref_obj->geometry_changed();
		m_scene_iterations->object_changed(ref_obj);

		unsigned obj_number =(m_iteration_selection*m_num_evs)+m_eigenmode_selection;
		ref_obj = m_scene_iterations->get_object(obj_number);
		ref_obj->set_visibility(true);
		ref_obj->geometry_changed();
		m_scene_iterations->object_changed(ref_obj);
		m_last_screen3_obj_idx = obj_number;

	}
}

void MainWindow::eigenmodeSpinBox_activated(int a){
	if(a < 1 || a > (int)m_num_evs){
		return;
	}

	m_eigenmode_selection = a-1;

	if(m_dataset_loaded){
		LGObject* ref_obj = m_scene_iterations->get_object(m_last_screen3_obj_idx);
		ref_obj->set_visibility(false);
		ref_obj->geometry_changed();
		m_scene_iterations->object_changed(ref_obj);
		
		unsigned obj_number =(m_iteration_selection*m_num_evs)+m_eigenmode_selection;
		ref_obj = m_scene_iterations->get_object(obj_number);
		ref_obj->set_visibility(true);
		ref_obj->geometry_changed();
		m_scene_iterations->object_changed(ref_obj);
		m_last_screen3_obj_idx = obj_number;
	}
}

void MainWindow::oscillating_toggled(bool b){
	if(m_dataset_loaded){
		m_oscillate = b;
	}
	else{
		QMessageBox msgBox;
		msgBox.setText("Load dataset first.");
		msgBox.exec();
		std::cout << "Load dataset first." << std::endl;
		oscillatingCheckBox->setChecked(false);
	}
	if(m_oscillate){
		oscillation();
	}
}

int MainWindow::openFile()
{
	int numOpened = 0;

	QString path = settings().value("file-path", ".").toString();

	QStringList fileNames = QFileDialog::getOpenFileNames(
								this,
								tr("Load Geometry"),
								path,
								tr("geometry files (").append(LG_SUPPORTED_FILE_FORMATS_OPEN).append(")"));

	for(QStringList::iterator iter = fileNames.begin();
		iter != fileNames.end(); ++iter)
	{
		settings().setValue("file-path", QFileInfo(*iter).absolutePath());
	//	load the object
		if(load_grid_from_file((*iter).toLocal8Bit().constData())){
			++numOpened;
			++m_num_objects;
		}
		else
		{
			QMessageBox msg(this);
			QString str = tr("Load failed: ");
			str.append(*iter);
			msg.setText(str);
			msg.exec();
		}
	}

	return numOpened;
}

unsigned minimum(unsigned a, unsigned b){
	return (a<b)?a:b;
}

bool MainWindow::openDataset()
{
	QString path = settings().value("file-path", ".").toString();

	QString q_dir = QFileDialog::getExistingDirectory(
								this,
								tr("Load Eigenmode Dataset"),
								path);

	std::string dir = q_dir.toUtf8().constData();

	if(!dir.size()){
		return false;
	}

	boost::filesystem::path p(dir);

	bool has_log_file = false;
	bool has_solutions_folder = false;
	bool has_debug_folder = false;

	std::string log_file;
	std::vector<std::string> solution_files; //ev list, solution entry
	std::vector<std::vector<std::string> > iteration_solution_files; //ev list, iteration list, solution entry
	std::vector<std::vector<std::string> > iteration_defect_files; //ev list, iteration list, solution entry
	std::vector<std::vector<std::string> > iteration_correction_files; //ev list, iteration list, solution entry

	for (auto i = directory_iterator(p); i != directory_iterator(); i++){
		if(i->path().filename().string() == "solutions"){
			std::cout << "found solutions folder" << std::endl;
			has_solutions_folder = true;
		}
		else if(i->path().filename().string() == "debug"){
			std::cout << "found debug folder" << std::endl;
			has_debug_folder = true;
		}
		else if(i->path().filename().string() == "log.txt"){
			std::cout << "found logfile: " << i->path().filename().string() << std::endl;
			std::cout << "found logfile: " << i->path() << std::endl;
			log_file = i->path().string();
			std::cout << "found logfile: " << log_file << std::endl;
			has_log_file = true;
		}
    }

	if(!has_log_file || !has_solutions_folder){
		return false;
	}


	UG_LogParser LP(log_file);
	LP.do_it();

	unsigned numevs = LP.num_evs();
	unsigned numiters = LP.num_iterations();
	unsigned numrefs = LP.num_refs();
	unsigned numprerefs = LP.num_prerefs();
	unsigned numprocs = LP.num_procs();
	//unsigned nummaxiterations = LP.num_max_iterations();
	std::string material = LP.material();
	double evprecision = LP.ev_precision();
	unsigned baselevel = LP.baselevel();
	double timeassembly = LP.time_assembly();
	double timesolver = LP.time_solver();
	double timetotal = LP.time_total();
	std::string pinvit = LP.pinvit();
	std::string smoother = LP.smoother();
	std::string basesolver = LP.basesolver();
	std::string additionalevs = LP.additional_evs();
	std::string preconditioner = LP.preconditioner();

	LP.lambdas(m_lambdas);
	LP.defects(m_defects);
	LP.frequencies(m_frequencies);
	

	std::string str_setup = "Number of Eigenpairs: " + std::to_string(numevs) + "     ";
	str_setup += "Additional Eigenpairs: " + additionalevs + "\n";
	str_setup += "Number of iterations: " + std::to_string(numiters) + "     ";
	str_setup += "Refinements: " + std::to_string(numrefs) + "     ";
	str_setup += "PreRefinements: " + std::to_string(numprerefs) + "\n";
	str_setup += "Number of Procs: " + std::to_string(numprocs) + "     ";
	str_setup += "Material: " + material + "     ";
	str_setup += "Precision: " + std::to_string(evprecision) + "     ";
	str_setup += "Baselevel: " + std::to_string(baselevel) + "\n";
	str_setup += "PINVIT: " + pinvit + "     ";
	str_setup += "Preconditioner: " + preconditioner + "\n";
	str_setup += "Smoother: " + smoother + "     ";
	str_setup += "Basesolver: " + basesolver + "\n";
	str_setup += "Time assembly: " + std::to_string(timeassembly) + "s     ";
	str_setup += "Time solver: " + std::to_string(timesolver) + "s     ";
	str_setup += "Time total: " + std::to_string(timetotal) + "s\n";

	m_num_iters = minimum(numiters-1, MAXITERS);
	m_num_evs = minimum(numevs, MAXEVS);

	m_picture->setText(QString::fromStdString(str_setup));

	boost::filesystem::path solutions_path(dir+"/solutions/");

	std::vector<bool> sol_file_existent(numevs, false);

	for (auto i = directory_iterator(solutions_path); i != directory_iterator(); i++){
		auto name = i->path().filename().string();
		if(name.find("_ascii.ugxc") != std::string::npos){
			std::cout << "sol file: " << name << std::endl;
			//ev_1.vtu
			std::string s_ev = name.substr(3, name.find("_ascii.ugxc")-3);
			std::cout << s_ev << std::endl;
			unsigned ev;
			myatoi(s_ev, ev);
			sol_file_existent[ev-1] = true;
		}
    }

	for(unsigned i = 0; i < numevs; ++i){
		if(!sol_file_existent[i]){
			std::cerr << "solution file for ev " << i+1 << " is missing!" << std::endl;
			return false;
		}
	}

	if(has_debug_folder){
		boost::filesystem::path debug_path(dir+"/debug/");

		std::vector<std::vector<bool> > iter_solution_file_existent(numiters, std::vector<bool>(numevs, false));
		std::vector<std::vector<bool> > iter_correction_file_existent(numiters, std::vector<bool>(numevs, false));
		std::vector<std::vector<bool> > iter_defect_file_existent(numiters, std::vector<bool>(numevs, false));

		for (auto i = directory_iterator(debug_path); i != directory_iterator(); i++){
			auto name = i->path().filename().string();

			if(name.find("_ascii.ugxc") != std::string::npos){
				if(name.find("_ev_") != std::string::npos){
					//std::cout << "ev iter file: " << name << std::endl;

					std::string s_iter = name.substr(10, name.find("_ev")-10);
					unsigned iter;
					myatoi(s_iter, iter);

					std::string s_ev = name.substr(name.find("_ev_")+4, name.find("_ascii")-(name.find("_ev_")+4));
					unsigned ev;
					myatoi(s_ev, ev);

					iter_solution_file_existent[iter][ev] = true;
					
				}
				else if(name.find("_corr_") != std::string::npos){
					//std::cout << "corr iter file: " << i->path().filename().string() << std::endl;

					std::string s_iter = name.substr(10, name.find("_corr")-10);
					unsigned iter;
					myatoi(s_iter, iter);

					std::string s_ev = name.substr(name.find("_corr_")+6, name.find("_ascii")-(name.find("_corr_")+6));
					unsigned ev;
					myatoi(s_ev, ev);

					iter_correction_file_existent[iter][ev] = true;
				}
				else if(name.find("_defect_") != std::string::npos){
					//std::cout << "defect iter file: " << i->path().filename().string() << std::endl;

					std::string s_iter = name.substr(10, name.find("_defect")-10);
					unsigned iter;
					myatoi(s_iter, iter);

					std::string s_ev = name.substr(name.find("_defect_")+8, name.find("_ascii")-(name.find("_defect_")+8));
					unsigned ev;
					myatoi(s_ev, ev);

					iter_defect_file_existent[iter][ev] = true;
				}
			}
		}


		bool status = true;

		for(unsigned i = 0; i < numiters-1; ++i){
			for(unsigned j = 0; j < numevs; ++j){
				if(!iter_solution_file_existent[i][j]){
					std::cerr << "iteration solution file for iteration " << i << " and ev " << j+1 << " is missing!" << std::endl;
					status = false;
				}
				//if(!iter_correction_file_existent[i][j]){
				//	std::cerr << "iteration corr file for iteration " << i << " and ev " << j+1 << " is missing!" << std::endl;
				//	status = false;
				//}
				if(!iter_defect_file_existent[i][j]){
					std::cerr << "iteration defect file for iteration " << i << " and ev " << j+1 << " is missing!" << std::endl;
					status = false;
				}
			}
		}

		if(!status){
			return false;
		}

	}

	std::string geometry_file = dir + "/solutions/" + "ev_" + std::to_string(1) + "_ascii.ugx";
	if(!load_grid_from_file(geometry_file.c_str())){
		std::cerr << "error loading geometry file: " << geometry_file << std::endl;
		return false; 
	}

	for(unsigned i = 0; i < minimum(numevs, EXTRASCENES); ++i){
		std::string name = dir + "/solutions/" + "ev_" + std::to_string(i+1) + "_ascii.ugxc";
		std::cout << "load " << name << " to split screen " << i << std::endl;

		if(!load_grid_from_file(name.c_str(), 2, i)){
			std::cerr << "error loading " << name << std::endl;
			return false; 
		}
		++m_num_objects;
	}

	for(unsigned i = 0; i < minimum(numiters-1, MAXITERS); ++i){
		for(unsigned j = 0; j < minimum(numevs, MAXEVS); ++j){ //pinvit_it_0_ev_1_ascii.ugxc
			std::string name = dir + "/debug/" + "pinvit_it_" + std::to_string(i) + "_ev_" + std::to_string(j) + "_ascii.ugxc";
			std::cout << "load " << name << " to iteration screen " << i << std::endl;

			if(!load_grid_from_file(name.c_str(), 3, 0)){
				std::cerr << "error loading " << name << std::endl;
				return false; 
			}
		}
	}

	eigenmodeSpinBox->setRange(1, numevs);
	iterationSpinBox->setRange(0, numiters-1);

/*
	for(unsigned i = 0; i < ceil(numevs/4); ++i){
		gridLayouts.push_back(new QGridLayout);
		for(unsigned j = 0; j < 4; ++j){
			gridLayouts[i+1]->addWidget(m_pViews[4+],0,0,1,1);
			gridLayouts[i+1]->addWidget(m_pViews[1],0,1,1,1);
			gridLayouts[i+1]->addWidget(m_pViews[2],1,0,1,1);
			gridLayouts[i+1]->addWidget(m_pViews[3],1,1,1,1);
		}
		gridWidgets.push_back(new QWidget());
		gridWidgets[i+1]->setLayout(gridLayouts[i+1]);
		stackedWidget->addWidget(gridWidgets[i+1]);
	}
*/

	m_dataset_loaded = true;

	return true;
}

LGObject* MainWindow::getActiveObject()
{
	return dynamic_cast<LGObject*>(m_sceneInspector->getActiveObject());
}

void MainWindow::setActiveObject(int index)
{
	m_sceneInspector->setActiveObject(index);
	if(getActiveObject() != m_activeObject){
		m_activeObject = getActiveObject();
		emit activeObjectChanged();
	}
}

void MainWindow::frontDrawModeChanged(int newMode)
{
	m_scene->set_draw_mode_front(newMode);
	for(unsigned i = 0; i < m_scenes.size(); ++i){
		m_scenes[i]->set_draw_mode_front(newMode);
	}
}

void MainWindow::backDrawModeChanged(int newMode)
{
	m_scene->set_draw_mode_back(newMode);
	for(unsigned i = 0; i < m_scenes.size(); ++i){
		m_scenes[i]->set_draw_mode_front(newMode);
	}

}

void MainWindow::backgroundColorChanged(const QColor& color)
{
	m_pView->set_background_color(color);
	for(unsigned i = 0; i < m_scenes.size(); ++i){
		m_pViews[i]->set_background_color(color);
	}
	m_pView_iterations->set_background_color(color);

	settings().setValue("bg-color", color.name());
}


void MainWindow::elementDrawModeChanged()
{
	m_scene->set_element_draw_mode(true, true, true, true);
	m_scene->update_visuals();

	for(unsigned i = 0; i < m_scenes.size(); ++i){
		m_scenes[i]->set_element_draw_mode(true, true, true, true);
		m_scenes[i]->update_visuals();
	}

	m_scene_iterations->set_element_draw_mode(true, true, true, true);
	m_scene_iterations->update_visuals();
}

////////////////////////////////////////////////////////////////////////
//	events
void MainWindow::closeEvent(QCloseEvent *event)
{
	if(m_scene->num_objects() == 0)
		event->accept();
	else{
		QMessageBox::StandardButton reply;
		reply = QMessageBox::question(this, "Quit?", "Quit?",
									  QMessageBox::Yes | QMessageBox::No);
		if (reply == QMessageBox::Yes){
			QMainWindow::closeEvent(event);
			event->accept();
		}
		else{
			event->ignore();
		}
	}

	if(event->isAccepted()){
		// settings().setValue("mainWindow/geometry", saveGeometry());
		settings().setValue("mainWindow/size", size());
    	settings().setValue("mainWindow/pos", pos());
		settings().setValue("mainWindow/windowState", saveState());
	}
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event)
{
	event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent* event)
{
	QList<QUrl> urls = event->mimeData()->urls();
	if (urls.isEmpty())
	   return;

	for(QList<QUrl>::iterator iter = urls.begin();
		iter != urls.end(); ++iter)
	{
		settings().setValue("file-path", QFileInfo((*iter).toLocalFile()).absolutePath());
		if(!load_grid_from_file((*iter).toLocalFile().toLatin1().constData()))
		{
			QMessageBox msg(this);
			QString str = tr("Load failed: ");
			str.append((*iter).toLocalFile());
			msg.setText(str);
			msg.exec();
		}
		++m_num_objects;
	}
}

void MainWindow::sceneInspectorClicked(QMouseEvent* event)
{
	if(getActiveObject() != m_activeObject){
		m_activeObject = getActiveObject();
		emit activeObjectChanged();
	}
}

void MainWindow::quit()
{
	this->close();
}

void MainWindow::
populateMenuBar ()
{
	QMenuBar* bar = menuBar();
	bar->clear();
	bar->addMenu(m_fileMenu);

	for(vector<QMenu*>::iterator i = m_moduleMenus.begin();
		i != m_moduleMenus.end(); ++i)
	{
		bar->addMenu(*i);
	}
}

void MainWindow::
activateModule (IModule* mod)
{
	typedef IModule::dock_list_t dock_list_t;

	if(m_activeModule == mod)
		return;

	if(m_activeModule){
	//	remove old modules dock widgets
		for(dock_list_t::iterator i = m_moduleDockWidgets.begin();
			i != m_moduleDockWidgets.end(); ++i)
		{
			removeDockWidget(i->second);
		}

		m_activeModule->deactivate();
	}
	
	m_activeModule = mod;

	if(m_activeModule){
		m_activeModule->activate (m_sceneInspector, m_scene);
		m_moduleDockWidgets = mod->getDockWidgets ();
		m_moduleMenus = mod->getMenus ();

		populateMenuBar ();

	//	add dock widgets
		for(dock_list_t::iterator i = m_moduleDockWidgets.begin();
			i != m_moduleDockWidgets.end(); ++i)
		{
			addDockWidget(i->first, i->second);
		}
	}
}


const char* MainWindow::log_text()
{
	return m_pLogText->toPlainText().toLocal8Bit().constData();
}
