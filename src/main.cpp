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

#include <QApplication>
#include <QSurfaceFormat>
#include <iostream>
#include <clocale>
#include <cstring>
#include <QFileOpenEvent>
#include "app.h"
#include "arg_tool.h"
#include "tools/standard_tools.h"
#include "bridge/bridge.h"
#include "common/util/path_provider.h"
#include "common/util/plugin_util.h"
#include "util/file_util.h"
//TESTING
#include <QDialog>
#include <QVBoxLayout>

using namespace std;

class MyApplication : public QApplication
{
	public:
		MyApplication(int & argc, char ** argv) :
			QApplication(argc, argv), m_pMainWindow(NULL)	{}

		virtual bool	event ( QEvent * e )
		{
			if(e->type() == QEvent::FileOpen){
				QFileOpenEvent* foe = dynamic_cast<QFileOpenEvent*>(e);
				if(foe){
					QString str = foe->file();
					if(m_pMainWindow){
						if(m_pMainWindow->load_grid_from_file(str.toLocal8Bit().constData()))
						{
							m_pMainWindow->settings().setValue("file-path",
												QFileInfo(str).absolutePath());
						}
					}
					return true;
				}
			}
			return QApplication::event(e);
		}

		void setMainWindow(MainWindow* win){
			m_pMainWindow = win;
		}

	private:
		MainWindow* m_pMainWindow;
};


static void WriteToFileInUserDataDir(const char* filename, const QString& content)
{
	SetFileContent (app::UserDataDir().absoluteFilePath(filename),
	                content);
}


int main(int argc, char *argv[])
{
	MyApplication myApp(argc, argv);
	myApp.setQuitOnLastWindowClosed(true);
	myApp.setAttribute (Qt::AA_UseDesktopOpenGL);
	
	QCoreApplication::setOrganizationName("EmVis");
    QCoreApplication::setApplicationName("EmVis 0.1");

    {
    //	write the absolute path of the application to .promesh/promesh_home
    	WriteToFileInUserDataDir ("promesh_home", app::AppDir().path());
    }

    {
    //	no-gui script processing
	    ArgTool args(argc, (const char**) argv);
	    
		string inFile = args.get_string ("-in", "",
									"(filename): Loads a mesh for script processing.\n"
									"Only relevant if '-script ...' is specified.");

		string outFile = args.get_string ("-out", "",
									"(filename): The resulting mesh from script processing\n"
									"will be saved to this file.\n"
									"Only relevant if '-script ...' is specified.");

		if(args.has_param ("-help", "Prints help on command line usage")){
			cout << "Command line options for ProMesh.\n\n";
			cout << args.get_help() << endl;
			return 0;
		}
	}

    cout.sync_with_stdio(true);


	QString qss = GetFileContent(":/styles/emvis_style.css");
	QString varsStr = GetFileContent(":/styles/dark_theme_variables.txt");
	QStringList varsList = varsStr.split(QRegularExpression("[\r\n]"),QString::SkipEmptyParts);
	QRegularExpression regVar("\\s*(@\\w+)\\s*(.+)");
	QMap<QString, QString> varMap;
	for(QStringList::Iterator iter = varsList.begin(); iter != varsList.end(); ++iter){
		QRegularExpressionMatch match = regVar.match(*iter);
		if(match.hasMatch()){
			varMap[match.captured(1)] = match.captured(2);
		}
	}
	
	QMapIterator<QString, QString> mapIter(varMap);
	mapIter.toBack();
	while(mapIter.hasPrevious()){
		mapIter.previous();
		qss.replace(mapIter.key(), mapIter.value());
	}

	myApp.setStyleSheet(qss);

/*
	if(!QGLFormat::hasOpenGL())
	{
		cerr << "System has no openGL support!" << endl;
		return 1;
	}
*/
	//UG_SET_DEBUG_LEVEL(LIB_GRID, 2);
//	ug::script::Initialize();
//	ug::script::InitLibGridScript(ug::script::GetLuaState());

	MainWindow* pMainWindow = app::getMainWindow();

	pMainWindow->init();

	myApp.setMainWindow(pMainWindow);
	pMainWindow->setWindowTitle(QString("EmVis"));

	pMainWindow->show();

	setlocale(LC_NUMERIC, "C");

	for(int i = 1; i < argc; ++i){
		if(argv[i][0] == '-')
			break;

		if(pMainWindow->load_grid_from_file(argv[i])){
			pMainWindow->settings().setValue("file-path",
											 QFileInfo(argv[i]).absolutePath());
		}
	}

	// dlg->show();

	#ifdef UG_DEBUG
		UG_LOG("DEBUG MODE\n");
	#endif
	#ifdef UG_ENABLE_DEBUG_LOGS
		UG_LOG("DEBUG LOGS ACTIVE\n");
		UG_SET_DEBUG_LEVEL(ug::LIB_GRID, 1);
	#endif

	return myApp.exec();
}
