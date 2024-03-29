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
#include "common/log.h"
#include "tool_dialog.h"
#include "tool_manager.h"
#include "app.h"
#include "../widgets/double_slider.h"
#include "../widgets/truncated_double_spin_box.h"

using namespace std;

ToolWidget::ToolWidget(const QString& name, QWidget* parent,
					ITool* tool, uint buttons) :
	QFrame(parent),
	m_currentFormLayout(NULL)
{

	m_tool = tool;
	//this->setWindowTitle(name);

	setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);

	QVBoxLayout* baseLayout = new QVBoxLayout(this);
	baseLayout->setSpacing(2);
	//baseLayout->setSpacing(10);

	QVBoxLayout* vBoxLayout = new QVBoxLayout();
	vBoxLayout->setSpacing(2);
	//vBoxLayout->setSpacing(10);
	m_mainLayout = vBoxLayout;
	baseLayout->addLayout(vBoxLayout);
	m_signalMapper = new QSignalMapper(this);
	connect(m_signalMapper, SIGNAL(mapped(int)),
			this, SLOT(buttonClicked(int)));

	if(buttons & IDB_APPLY){
		QPushButton* btn = new QPushButton(tr("Apply"), this);
		baseLayout->addWidget(btn, 0, Qt::AlignLeft);
		m_signalMapper->setMapping(btn, IDB_APPLY);
		connect(btn, SIGNAL(clicked()), m_signalMapper, SLOT(map()));
	}

	m_valueSignalMapper = new QSignalMapper(this);
	connect(m_valueSignalMapper, SIGNAL(mapped(int)),
			this, SIGNAL(valueChanged(int)));

	// QFrame* sep = new QFrame(this);
	// sep->setFrameShape(QFrame::HLine);
	// sep->setFrameShadow(QFrame::Sunken);
	// baseLayout->addWidget(sep);

/*
	if(buttons & IDB_PREVIEW){
		QPushButton* btn = new QPushButton(tr("Preview"), this);
		vBoxLayout->addWidget(btn, 0, Qt::AlignRight);
		m_signalMapper->setMapping(btn, IDB_PREVIEW);
		connect(btn, SIGNAL(clicked()), m_signalMapper, SLOT(map()));
	}

	if(buttons & IDB_OK){
		QPushButton* btn = new QPushButton(tr("Ok"), this);
		vBoxLayout->addWidget(btn, 0, Qt::AlignRight);
		m_signalMapper->setMapping(btn, IDB_OK);
		connect(btn, SIGNAL(clicked()), m_signalMapper, SLOT(map()));
	}

	if(buttons & IDB_CLOSE){
		QPushButton* btn = new QPushButton(tr("Close"), this);
		vBoxLayout->addWidget(btn, 0, Qt::AlignRight);
		m_signalMapper->setMapping(btn, IDB_CLOSE);
		connect(btn, SIGNAL(clicked()), m_signalMapper, SLOT(map()));
	}

	if(buttons & IDB_CANCEL){
		vBoxLayout->addSpacing(1);
		QPushButton* btn = new QPushButton(tr("Cancel"), this);
		vBoxLayout->addWidget(btn, 0, Qt::AlignRight);
		m_signalMapper->setMapping(btn, IDB_CANCEL);
		connect(btn, SIGNAL(clicked()), m_signalMapper, SLOT(map()));
	}
*/
}

QFormLayout* ToolWidget::current_form_layout()
{
	if(!m_currentFormLayout){
		m_currentFormLayout = new QFormLayout();
		//m_currentFormLayout->setSpacing(5);
		m_currentFormLayout->setSpacing(2);
		//m_currentFormLayout->setHorizontalSpacing(10);
		m_currentFormLayout->setHorizontalSpacing(2);
		m_currentFormLayout->setVerticalSpacing(8);
		m_mainLayout->addLayout(m_currentFormLayout);
		//m_mainLayout->addSpacing(15);
	}
	return m_currentFormLayout;
}

void ToolWidget::addWidget(const QString& caption, QWidget* widget)
{
	current_form_layout()->addRow(caption, widget);
	m_widgets.push_back(WidgetEntry(widget, WT_WIDGET));
}

void ToolWidget::addSlider(const QString& caption,
							double min, double max, double value)
{
	DoubleSlider* slider = new DoubleSlider(this);
	slider->setRange(min, max);
	slider->setValue(value);
	current_form_layout()->addRow(caption, slider);
	m_valueSignalMapper->setMapping(slider, (int)m_widgets.size());
	connect(slider, SIGNAL(valueChanged()), m_valueSignalMapper, SLOT(map()));
	m_widgets.push_back(WidgetEntry(slider, WT_SLIDER));
}

void ToolWidget::addSpinBox(const QString& caption,
							double min, double max, double value,
							double stepSize, int numDecimals)
{
	TruncatedDoubleSpinBox* spinner = new TruncatedDoubleSpinBox(this);
	spinner->setLocale(QLocale(tr("C")));
	spinner->setRange(min, max);
	spinner->setDecimals(numDecimals);
	spinner->setSingleStep(stepSize);
	spinner->setValue(value);
	current_form_layout()->addRow(caption, spinner);

	m_valueSignalMapper->setMapping(spinner, (int)m_widgets.size());
	
	connect(spinner,
			SIGNAL(valueChanged(double)),
			m_valueSignalMapper, SLOT(map()));
	

	m_widgets.push_back(WidgetEntry(spinner, WT_SPIN_BOX));
}

void ToolWidget::addComboBox(const QString& caption,
							const QStringList& entries,
							int activeEntry)
{
	QComboBox* combo = new QComboBox(this);
	combo->addItems(entries);
	combo->setCurrentIndex(activeEntry);
	current_form_layout()->addRow(caption, combo);
	m_valueSignalMapper->setMapping(combo, (int)m_widgets.size());
	connect(combo, SIGNAL(currentIndexChanged(int)), m_valueSignalMapper, SLOT(map()));
	m_widgets.push_back(WidgetEntry(combo, WT_COMBO_BOX));
}

void ToolWidget::addCheckBox(const QString& caption,
							bool bChecked)
{
	QCheckBox* check = new QCheckBox(caption, this);
	check->setChecked(bChecked);
	m_currentFormLayout = NULL;
	m_mainLayout->addWidget(check);
	m_valueSignalMapper->setMapping(check, (int)m_widgets.size());
	connect(check, SIGNAL(stateChanged(int)), m_valueSignalMapper, SLOT(map()));
	m_widgets.push_back(WidgetEntry(check, WT_CHECK_BOX));
}

void ToolWidget::addListBox(const QString& caption,
							QStringList& entries,
							bool multiSelection)
{
	QListWidget* list = new QListWidget(this);
	if(multiSelection)
		list->setSelectionMode(QAbstractItemView::MultiSelection);
	list->addItems(entries);
	current_form_layout()->addRow(caption, list);
	m_widgets.push_back(WidgetEntry(list, WT_LIST_BOX));
}

void ToolWidget::addTextBox(const QString& caption, const QString& text)
{
	QLineEdit* textBox = new QLineEdit(this);
	textBox->setText(text);
	current_form_layout()->addRow(caption, textBox);
	m_valueSignalMapper->setMapping(textBox, (int)m_widgets.size());
	connect(textBox, SIGNAL(textChanged(const QString&)), m_valueSignalMapper, SLOT(map()));
	m_widgets.push_back(WidgetEntry(textBox, WT_TEXT_BOX));
}

void ToolWidget::addFileBrowser(const QString& caption, FileWidgetType fwt,
								const QString& filter)
{
	FileWidget* fw = new FileWidget(fwt, filter, this);
	current_form_layout()->addRow(caption, fw);
	m_widgets.push_back(WidgetEntry(fw, WT_FILE_BROWSER));
}

void ToolWidget::buttonClicked(int buttonID)
{
	LGObject* obj = app::getActiveObject();
	if(!obj){
	//todo: create the appropriate object for the current module
		obj = app::createEmptyObject("new mesh", SOT_LG, 1, 0);
	}
	
	if(m_tool && (obj || m_tool->accepts_null_object_ptr())){
		switch(buttonID){
		case IDB_OK:
		case IDB_APPLY:
			try{
				m_tool->execute(obj, this);
			}
			catch(ug::UGError error){
				UG_LOG("Execution of tool " << m_tool->get_name() << " failed with the following message:\n");
				UG_LOG("  " << error.get_msg() << std::endl);
			}
			break;
		}
	}
/*
	switch(buttonID){
		case IDB_OK:
			accept();
			break;
		case IDB_CANCEL:
		case IDB_CLOSE:
			reject();
			break;
	}
*/
}

void ToolWidget::clearLayout(QLayout* layout)
{
    while(layout->count() > 0){
    	QLayoutItem *item = layout->takeAt(0);
        if (item->layout()) {
            clearLayout(item->layout());
            //delete item->layout();
        }
        if (item->widget()) {
            delete item->widget();
        }
        delete item;
    }
}

void ToolWidget::clear()
{
	clearLayout(m_mainLayout);
	m_currentFormLayout = NULL;
	m_widgets.clear();
}

template <class TNumber>
TNumber ToolWidget::to_number(int paramIndex, bool* bOKOut)
{
	if(bOKOut)
		*bOKOut = true;

	if(paramIndex < 0 || paramIndex >= (int)m_widgets.size()){
		UG_LOG("ERROR: bad parameter index in ToolDialog::to_number: " << paramIndex << std::endl);
		if(bOKOut)
			*bOKOut = false;
		return 0;
	}

	WidgetEntry& we = m_widgets[paramIndex];

	switch(we.m_widgetType){
	case WT_SLIDER:{
			DoubleSlider* slider = dynamic_cast<DoubleSlider*>(we.m_widget);
			return (TNumber)slider->value();
		}break;
	case WT_SPIN_BOX:{
			TruncatedDoubleSpinBox* spinBox = qobject_cast<TruncatedDoubleSpinBox*>(we.m_widget);
			return (TNumber)spinBox->value();
		}break;
	case WT_COMBO_BOX:{
			QComboBox* combo = qobject_cast<QComboBox*>(we.m_widget);
			return (TNumber)combo->currentIndex();
		}break;
	case WT_CHECK_BOX:{
			QCheckBox* check = qobject_cast<QCheckBox*>(we.m_widget);
			if(check->isChecked())
				return TNumber(1);
			return TNumber(0);
		}break;
	default:
		UG_LOG("ERROR in ToolDialog::to_number: Parameter " << paramIndex << " can't be converted to a number.\n");
		if(bOKOut)
			*bOKOut = false;
		return TNumber(0);
	}
}

bool ToolWidget::to_bool(int paramIndex, bool* bOKOut)
{
	return to_number<int>(paramIndex, bOKOut) != 0;
}

int ToolWidget::to_int(int paramIndex, bool* bOKOut)
{
	return to_number<int>(paramIndex, bOKOut);
}

double ToolWidget::to_double(int paramIndex, bool* bOKOut)
{
	return to_number<double>(paramIndex, bOKOut);
}

vector<int> ToolWidget::to_index_list(int paramIndex, bool* bOKOut)
{
	if(bOKOut)
		*bOKOut = true;

//	iterate over all entries in the list. if an entry is selected, push
//	then push the associated index into the index-array.
	vector<int> outVec;

	WidgetEntry& we = m_widgets[paramIndex];

	if(we.m_widgetType == WT_LIST_BOX){
		QListWidget* list = qobject_cast<QListWidget*>(we.m_widget);
		for(int i = 0; i < list->count(); ++i){
			QListWidgetItem* item = list->item(i);
			if(item->isSelected())
				outVec.push_back(i);
		}
	}
	else{
		UG_LOG("ERROR in ToolDialog::to_number: Parameter " << paramIndex << " can't be converted to a number.\n");
		if(bOKOut)
			*bOKOut = false;
	}
	return outVec;
}

QString ToolWidget::to_string(int paramIndex, bool* bOKOut)
{
	if(bOKOut)
		*bOKOut = true;

	WidgetEntry& we = m_widgets[paramIndex];

	if(we.m_widgetType == WT_TEXT_BOX){
		QLineEdit* textBox = qobject_cast<QLineEdit*>(we.m_widget);
		return textBox->text();
	}
	else if(we.m_widgetType == WT_FILE_BROWSER){
		FileWidget* fw = qobject_cast<FileWidget*>(we.m_widget);
		return fw->filename();
	}
	else{
		UG_LOG("ERROR in ToolDialog::to_string: Parameter " << paramIndex << " can't be converted to a string-list.\n");
		if(bOKOut)
			*bOKOut = false;
	}

	return QString();
}

QStringList ToolWidget::to_string_list(int paramIndex, bool* bOKOut)
{
	if(bOKOut)
		*bOKOut = true;

	WidgetEntry& we = m_widgets[paramIndex];

	if(we.m_widgetType == WT_FILE_BROWSER){
		FileWidget* fw = qobject_cast<FileWidget*>(we.m_widget);
		return fw->filenames();
	}
	else{
		UG_LOG("ERROR in ToolDialog::to_string_list: Parameter " << paramIndex << " can't be converted to a string-list.\n");
		if(bOKOut)
			*bOKOut = false;
	}

	return QStringList();
}

QWidget* ToolWidget::to_widget(int paramIndex, bool* bOkOut)
{
	if(paramIndex < 0 || paramIndex >= (int)m_widgets.size()){
		UG_LOG("ERROR: bad parameter index in ToolDialog::to_widget: " << paramIndex << std::endl);
		if(bOkOut)
			*bOkOut = false;
		return NULL;
	}

	WidgetEntry& we = m_widgets[paramIndex];
	if(we.m_widgetType == WT_WIDGET){
		if(bOkOut)
			*bOkOut = true;
		return we.m_widget;
	}
	if(bOkOut)
		*bOkOut = false;
	return NULL;
}

bool ToolWidget::setNumber(int paramIndex, double val)
{
	if(paramIndex < 0 || paramIndex >= (int)m_widgets.size()){
		UG_LOG("ERROR: bad parameter index in ToolDialog::setNumber: " << paramIndex << std::endl);
		return false;
	}

	WidgetEntry& we = m_widgets[paramIndex];

	switch(we.m_widgetType){
	case WT_SLIDER:{
			QSlider* slider = qobject_cast<QSlider*>(we.m_widget);
			slider->setValue(val);
		}break;
	case WT_SPIN_BOX:{
			TruncatedDoubleSpinBox* spinBox = qobject_cast<TruncatedDoubleSpinBox*>(we.m_widget);
			spinBox->setValue(val);
		}break;
	default:
		UG_LOG("ERROR in ToolDialog::setNumber: No matching widget found for parameter " << paramIndex << ".\n");
		return false;
	}

	return true;
}

bool ToolWidget::setString(int paramIndex, const QString& param)
{
	WidgetEntry& we = m_widgets[paramIndex];

	if(we.m_widgetType == WT_TEXT_BOX){
		QLineEdit* textBox = qobject_cast<QLineEdit*>(we.m_widget);
		textBox->setText(param);
	}
	else{
		UG_LOG("ERROR in ToolDialog::set_string: Parameter " << paramIndex << " can't be converted to a string-list.\n");
		return false;
	}

	return true;
}

bool ToolWidget::setStringList(int paramIndex, const QStringList& stringList)
{
	WidgetEntry& we = m_widgets[paramIndex];

	if(we.m_widgetType == WT_LIST_BOX){
		QListWidget* listBox = qobject_cast<QListWidget*>(we.m_widget);
		listBox->clear();
		listBox->addItems(stringList);
	}
	else{
		UG_LOG("ERROR in ToolDialog::set_string_list: Parameter " << paramIndex << " can't be converted to a list box.\n");
		return false;
	}

	return true;
}


void ToolWidget::refreshContents()
{
	if(m_tool)
		m_tool->refresh_dialog(this);
}
