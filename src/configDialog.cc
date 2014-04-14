/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013, 2014 Santeri Piippo
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *  =====================================================================
 *
 *  configDialog.cxx: Settings dialog and everything related to it.
 *  Actual configuration core is in config.cxx.
 */

#include <QGridLayout>
#include <QFileDialog>
#include <QColorDialog>
#include <QBoxLayout>
#include <QKeyEvent>
#include <QGroupBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QCheckBox>
#include "main.h"
#include "configDialog.h"
#include "ldDocument.h"
#include "configuration.h"
#include "miscallenous.h"
#include "colors.h"
#include "colorSelector.h"
#include "glRenderer.h"
#include "ui_config.h"

extern_cfg (String, gl_bgcolor);
extern_cfg (String, gl_maincolor);
extern_cfg (Bool, lv_colorize);
extern_cfg (Bool, gl_colorbfc);
extern_cfg (Float, gl_maincolor_alpha);
extern_cfg (Int, gl_linethickness);
extern_cfg (String, gui_colortoolbar);
extern_cfg (Bool, edit_schemanticinline);
extern_cfg (Bool, gl_blackedges);
extern_cfg (Bool, gl_aa);
extern_cfg (Bool, gui_implicitfiles);
extern_cfg (String, net_downloadpath);
extern_cfg (Bool, net_guesspaths);
extern_cfg (Bool, net_autoclose);
extern_cfg (Bool, gl_logostuds);
extern_cfg (Bool,	gl_linelengths);
extern_cfg (String, ld_defaultname);
extern_cfg (String, ld_defaultuser);
extern_cfg (Int, ld_defaultlicense);
extern_cfg (String, gl_selectcolor);
extern_cfg (String, prog_ytruder);
extern_cfg (String, prog_rectifier);
extern_cfg (String, prog_intersector);
extern_cfg (String, prog_coverer);
extern_cfg (String, prog_isecalc);
extern_cfg (String, prog_edger2);
extern_cfg (Bool, prog_ytruder_wine);
extern_cfg (Bool, prog_rectifier_wine);
extern_cfg (Bool, prog_intersector_wine);
extern_cfg (Bool, prog_coverer_wine);
extern_cfg (Bool, prog_isecalc_wine);
extern_cfg (Bool, prog_edger2_wine);

const char* g_extProgPathFilter =
#ifdef _WIN32
	"Applications (*.exe)(*.exe);;All files (*.*)(*.*)";
#else
	"";
#endif

// =============================================================================
// =============================================================================
ConfigDialog::ConfigDialog (ConfigDialog::Tab deftab, QWidget* parent, Qt::WindowFlags f) :
	QDialog (parent, f)
{
	assert (g_win != null);
	ui = new Ui_ConfigUI;
	ui->setupUi (this);

	// Interface tab
	setButtonBackground (ui->backgroundColorButton, gl_bgcolor);
	connect (ui->backgroundColorButton, SIGNAL (clicked()),
			 this, SLOT (slot_setGLBackground()));

	setButtonBackground (ui->mainColorButton, gl_maincolor);
	connect (ui->mainColorButton, SIGNAL (clicked()),
			 this, SLOT (slot_setGLForeground()));

	setButtonBackground (ui->selColorButton, gl_selectcolor);
	connect (ui->selColorButton, SIGNAL (clicked()),
			 this, SLOT (slot_setGLSelectColor()));

	ui->mainColorAlpha->setValue (gl_maincolor_alpha * 10.0f);
	ui->lineThickness->setValue (gl_linethickness);
	ui->colorizeObjects->setChecked (lv_colorize);
	ui->colorBFC->setChecked (gl_colorbfc);
	ui->blackEdges->setChecked (gl_blackedges);
	ui->m_aa->setChecked (gl_aa);
	ui->implicitFiles->setChecked (gui_implicitfiles);
	ui->m_logostuds->setChecked (gl_logostuds);
	ui->linelengths->setChecked (gl_linelengths);

	int i = 0;

	for (QAction* act : g_win->findChildren<QAction*>())
	{
		KeySequenceConfig* cfg = g_win->shortcutForAction (act);

		if (cfg)
			addShortcut (*cfg, act, i);
	}

	ui->shortcutsList->setSortingEnabled (true);
	ui->shortcutsList->sortItems();

	connect (ui->shortcut_set, SIGNAL (clicked()), this, SLOT (slot_setShortcut()));
	connect (ui->shortcut_reset, SIGNAL (clicked()), this, SLOT (slot_resetShortcut()));
	connect (ui->shortcut_clear, SIGNAL (clicked()), this, SLOT (slot_clearShortcut()));

	quickColors = quickColorsFromConfig();
	updateQuickColorList();

	connect (ui->quickColor_add, SIGNAL (clicked()), this, SLOT (slot_setColor()));
	connect (ui->quickColor_remove, SIGNAL (clicked()), this, SLOT (slot_delColor()));
	connect (ui->quickColor_edit, SIGNAL (clicked()), this, SLOT (slot_setColor()));
	connect (ui->quickColor_addSep, SIGNAL (clicked()), this, SLOT (slot_addColorSeparator()));
	connect (ui->quickColor_moveUp, SIGNAL (clicked()), this, SLOT (slot_moveColor()));
	connect (ui->quickColor_moveDown, SIGNAL (clicked()), this, SLOT (slot_moveColor()));
	connect (ui->quickColor_clear, SIGNAL (clicked()), this, SLOT (slot_clearColors()));

	ui->downloadPath->setText (net_downloadpath);
	ui->guessNetPaths->setChecked (net_guesspaths);
	ui->autoCloseNetPrompt->setChecked (net_autoclose);
	connect (ui->findDownloadPath, SIGNAL (clicked (bool)), this, SLOT (slot_findDownloadFolder()));

	ui->m_profileName->setText (ld_defaultname);
	ui->m_profileUsername->setText (ld_defaultuser);
	ui->m_profileLicense->setCurrentIndex (ld_defaultlicense);

	initGrids();
	initExtProgs();
	selectPage (deftab);

	connect (ui->buttonBox, SIGNAL (clicked (QAbstractButton*)),
		this, SLOT (buttonClicked (QAbstractButton*)));

	connect (ui->m_pages, SIGNAL (currentChanged (int)),
		this, SLOT (selectPage (int)));

	connect (ui->m_pagelist, SIGNAL (currentRowChanged (int)),
		this, SLOT (selectPage (int)));
}

// =============================================================================
// =============================================================================
ConfigDialog::~ConfigDialog()
{
	delete ui;
}

// =============================================================================
// =============================================================================
void ConfigDialog::selectPage (int row)
{
	ui->m_pagelist->setCurrentRow (row);
	ui->m_pages->setCurrentIndex (row);
}

// =============================================================================
// Adds a shortcut entry to the list of shortcuts.
// =============================================================================
void ConfigDialog::addShortcut (KeySequenceConfig& cfg, QAction* act, int& i)
{
	ShortcutListItem* item = new ShortcutListItem;
	item->setIcon (act->icon());
	item->setKeyConfig (&cfg);
	item->setAction (act);
	setShortcutText (item);

	// If the action doesn't have a valid icon, use an empty one
	// so that the list is kept aligned.
	if (act->icon().isNull())
		item->setIcon (getIcon ("empty"));

	ui->shortcutsList->insertItem (i++, item);
}

// =============================================================================
// Initializes the table of grid stuff
// =============================================================================
void ConfigDialog::initGrids()
{
	QGridLayout* gridlayout = new QGridLayout;
	QLabel* xlabel = new QLabel ("X"),
	*ylabel = new QLabel ("Y"),
	*zlabel = new QLabel ("Z"),
	*anglabel = new QLabel ("Angle");
	int i = 1;

	for (QLabel* label : QList<QLabel*> ({xlabel, ylabel, zlabel, anglabel}))
	{
		label->setAlignment (Qt::AlignCenter);
		gridlayout->addWidget (label, 0, i++);
	}

	for (int i = 0; i < g_NumGrids; ++i)
	{
		// Icon
		lb_gridIcons[i] = new QLabel;
		lb_gridIcons[i]->setPixmap (getIcon (format ("grid-%1", QString (g_GridInfo[i].name).toLower())));

		// Text label
		lb_gridLabels[i] = new QLabel (format ("%1:", g_GridInfo[i].name));

		QHBoxLayout* labellayout = new QHBoxLayout;
		labellayout->addWidget (lb_gridIcons[i]);
		labellayout->addWidget (lb_gridLabels[i]);
		gridlayout->addLayout (labellayout, i + 1, 0);

		// Add the widgets
		for (int j = 0; j < 4; ++j)
		{
			dsb_gridData[i][j] = new QDoubleSpinBox;

			// Set the maximum angle
			if (j == 3)
				dsb_gridData[i][j]->setMaximum (360);

			dsb_gridData[i][j]->setValue (*g_GridInfo[i].confs[j]);
			gridlayout->addWidget (dsb_gridData[i][j], i + 1, j + 1);
		}
	}

	ui->grids->setLayout (gridlayout);
}

// =============================================================================
// =============================================================================
static struct LDExtProgInfo
{
	const QString		name,
						iconname;
	QString* const		path;
	QLineEdit*		input;
	QPushButton*	setPathButton;
#ifndef _WIN32
	bool* const		wine;
	QCheckBox*		wineBox;
#endif // _WIN32
} g_LDExtProgInfo[] =
{
#ifndef _WIN32
# define EXTPROG(NAME, LOWNAME) { #NAME, #LOWNAME, &prog_##LOWNAME, null, null, &prog_##LOWNAME##_wine, null },
#else
# define EXTPROG(NAME, LOWNAME) { #NAME, #LOWNAME, &prog_##LOWNAME, null, null },
#endif
	EXTPROG (Ytruder, ytruder)
	EXTPROG (Rectifier, rectifier)
	EXTPROG (Intersector, intersector)
	EXTPROG (Isecalc, isecalc)
	EXTPROG (Coverer, coverer)
	EXTPROG (Edger2, edger2)
#undef EXTPROG
};

// =============================================================================
// Initializes the stuff in the ext programs tab
// =============================================================================
void ConfigDialog::initExtProgs()
{
	QGridLayout* pathsLayout = new QGridLayout;
	int row = 0;

	for (LDExtProgInfo& info : g_LDExtProgInfo)
	{
		QLabel* icon = new QLabel,
		*progLabel = new QLabel (info.name);
		QLineEdit* input = new QLineEdit;
		QPushButton* setPathButton = new QPushButton;

		icon->setPixmap (getIcon (info.iconname));
		input->setText (*info.path);
		setPathButton->setIcon (getIcon ("folder"));
		info.input = input;
		info.setPathButton = setPathButton;

		connect (setPathButton, SIGNAL (clicked()), this, SLOT (slot_setExtProgPath()));

		pathsLayout->addWidget (icon, row, 0);
		pathsLayout->addWidget (progLabel, row, 1);
		pathsLayout->addWidget (input, row, 2);
		pathsLayout->addWidget (setPathButton, row, 3);

#ifndef _WIN32
		QCheckBox* wineBox = new QCheckBox ("Wine");
		wineBox->setChecked (*info.wine);
		info.wineBox = wineBox;
		pathsLayout->addWidget (wineBox, row, 4);
#endif

		++row;
	}

	ui->extProgs->setLayout (pathsLayout);
}

// =============================================================================
// Set the settings based on widget data.
// =============================================================================
void ConfigDialog::applySettings()
{
	// Apply configuration
	lv_colorize = ui->colorizeObjects->isChecked();
	gl_colorbfc = ui->colorBFC->isChecked();
	gl_blackedges = ui->blackEdges->isChecked();
	gl_maincolor_alpha = ( (double) ui->mainColorAlpha->value()) / 10.0f;
	gl_linethickness = ui->lineThickness->value();
	gui_implicitfiles = ui->implicitFiles->isChecked();
	net_downloadpath = ui->downloadPath->text();
	net_guesspaths = ui->guessNetPaths->isChecked();
	net_autoclose = ui->autoCloseNetPrompt->isChecked();
	gl_logostuds = ui->m_logostuds->isChecked();
	gl_linelengths = ui->linelengths->isChecked();
	ld_defaultuser = ui->m_profileUsername->text();
	ld_defaultname = ui->m_profileName->text();
	ld_defaultlicense = ui->m_profileLicense->currentIndex();
	gl_aa = ui->m_aa->isChecked();

	// Rebuild the quick color toolbar
	g_win->setQuickColors (quickColors);
	gui_colortoolbar = quickColorString();

	// Set the grid settings
	for (int i = 0; i < g_NumGrids; ++i)
		for (int j = 0; j < 4; ++j)
			*g_GridInfo[i].confs[j] = dsb_gridData[i][j]->value();

	// Apply key shortcuts
	g_win->updateActionShortcuts();

	// Ext program settings
	for (const LDExtProgInfo& info : g_LDExtProgInfo)
	{
		*info.path = info.input->text();

#ifndef _WIN32
		*info.wine = info.wineBox->isChecked();
#endif // _WIN32
	}

	Config::save();
	reloadAllSubfiles();
	loadLogoedStuds();
	g_win->R()->setBackground();
	g_win->doFullRefresh();
	g_win->updateDocumentList();
}

// =============================================================================
// A dialog button was clicked
// =============================================================================
void ConfigDialog::buttonClicked (QAbstractButton* button)
{
	typedef QDialogButtonBox QDDB;
	QDialogButtonBox* dbb = ui->buttonBox;

	if (button == dbb->button (QDDB::Ok))
	{
		applySettings();
		accept();
	} elif (button == dbb->button (QDDB::Apply))
	{
		applySettings();
	} elif (button == dbb->button (QDDB::Cancel))
	{
		reject();
	}
}

// =============================================================================
// Update the list of color toolbar items in the quick color tab.
// =============================================================================
void ConfigDialog::updateQuickColorList (LDQuickColor* sel)
{
	for (QListWidgetItem * item : quickColorItems)
		delete item;

	quickColorItems.clear();

	// Init table items
	for (LDQuickColor& entry : quickColors)
	{
		QListWidgetItem* item = new QListWidgetItem;

		if (entry.isSeparator())
		{
			item->setText ("--------");
			item->setIcon (getIcon ("empty"));
		}
		else
		{
			LDColor* col = entry.color();

			if (col == null)
			{
				item->setText ("[[unknown color]]");
				item->setIcon (getIcon ("error"));
			}
			else
			{
				item->setText (col->name);
				item->setIcon (makeColorIcon (col, 16));
			}
		}

		ui->quickColorList->addItem (item);
		quickColorItems << item;

		if (sel && &entry == sel)
		{
			ui->quickColorList->setCurrentItem (item);
			ui->quickColorList->scrollToItem (item);
		}
	}
}

// =============================================================================
// Quick colors: add or edit button was clicked.
// =============================================================================
void ConfigDialog::slot_setColor()
{
	LDQuickColor* entry = null;
	QListWidgetItem* item = null;
	const bool isNew = static_cast<QPushButton*> (sender()) == ui->quickColor_add;

	if (not isNew)
	{
		item = getSelectedQuickColor();

		if (not item)
			return;

		int i = getItemRow (item, quickColorItems);
		entry = &quickColors[i];

		if (entry->isSeparator() == true)
			return; // don't color separators
	}

	int defval = entry ? entry->color()->index : -1;
	int val;

	if (not ColorSelector::selectColor (val, defval, this))
		return;

	if (entry)
		entry->setColor (getColor (val));
	else
	{
		LDQuickColor entry (getColor (val), null);

		item = getSelectedQuickColor();
		int idx = (item) ? getItemRow (item, quickColorItems) + 1 : quickColorItems.size();

		quickColors.insert (idx, entry);
		entry = quickColors[idx];
	}

	updateQuickColorList (entry);
}

// =============================================================================
// Remove a quick color
// =============================================================================
void ConfigDialog::slot_delColor()
{
	if (ui->quickColorList->selectedItems().isEmpty())
		return;

	QListWidgetItem* item = ui->quickColorList->selectedItems() [0];
	quickColors.removeAt (getItemRow (item, quickColorItems));
	updateQuickColorList();
}

// =============================================================================
// Move a quick color up/down
// =============================================================================
void ConfigDialog::slot_moveColor()
{
	const bool up = (static_cast<QPushButton*> (sender()) == ui->quickColor_moveUp);

	if (ui->quickColorList->selectedItems().isEmpty())
		return;

	QListWidgetItem* item = ui->quickColorList->selectedItems() [0];
	int idx = getItemRow (item, quickColorItems);
	int dest = up ? (idx - 1) : (idx + 1);

	if (dest < 0 || dest >= quickColorItems.size())
		return; // destination out of bounds

	LDQuickColor tmp = quickColors[dest];
	quickColors[dest] = quickColors[idx];
	quickColors[idx] = tmp;

	updateQuickColorList (&quickColors[dest]);
}

// =============================================================================
//
// Add a separator to quick colors
//
void ConfigDialog::slot_addColorSeparator()
{
	quickColors << LDQuickColor::getSeparator();
	updateQuickColorList (&quickColors[quickColors.size() - 1]);
}

// =============================================================================
//
// Clear all quick colors
//
void ConfigDialog::slot_clearColors()
{
	quickColors.clear();
	updateQuickColorList();
}

// =============================================================================
//
// Pick a color and set the appropriate configuration option.
//
void ConfigDialog::pickColor (QString& conf, QPushButton* button)
{
	QColor col = QColorDialog::getColor (QColor (conf));

	if (col.isValid())
	{
		int r = col.red(),
			g = col.green(),
			b = col.blue();

		QString colname;
		colname.sprintf ("#%.2X%.2X%.2X", r, g, b);
		conf = colname;
		setButtonBackground (button, colname);
	}
}

// =============================================================================
// =============================================================================
void ConfigDialog::slot_setGLBackground()
{
	pickColor (gl_bgcolor, ui->backgroundColorButton);
}

// =============================================================================
// =============================================================================
void ConfigDialog::slot_setGLForeground()
{
	pickColor (gl_maincolor, ui->mainColorButton);
}

// =============================================================================
// =============================================================================
void ConfigDialog::slot_setGLSelectColor()
{
	pickColor (gl_selectcolor, ui->selColorButton);
}

// =============================================================================
// Sets background color of a given button.
// =============================================================================
void ConfigDialog::setButtonBackground (QPushButton* button, QString value)
{
	button->setIcon (getIcon ("colorselect"));
	button->setAutoFillBackground (true);
	button->setStyleSheet (format ("background-color: %1", value));
}

// =============================================================================
// Finds the given list widget item in the list of widget items given.
// =============================================================================
int ConfigDialog::getItemRow (QListWidgetItem* item, QList<QListWidgetItem*>& haystack)
{
	int i = 0;

	for (QListWidgetItem* it : haystack)
	{
		if (it == item)
			return i;

		++i;
	}

	return -1;
}

// =============================================================================
// Which quick color is currently selected?
// =============================================================================
QListWidgetItem* ConfigDialog::getSelectedQuickColor()
{
	if (ui->quickColorList->selectedItems().isEmpty())
		return null;

	return ui->quickColorList->selectedItems() [0];
}

// =============================================================================
// Get the list of shortcuts selected
// =============================================================================
QList<ShortcutListItem*> ConfigDialog::getShortcutSelection()
{
	QList<ShortcutListItem*> out;

	for (QListWidgetItem* entry : ui->shortcutsList->selectedItems())
		out << static_cast<ShortcutListItem*> (entry);

	return out;
}

// =============================================================================
// Edit the shortcut of a given action.
// =============================================================================
void ConfigDialog::slot_setShortcut()
{
	QList<ShortcutListItem*> sel = getShortcutSelection();

	if (sel.size() < 1)
		return;

	ShortcutListItem* item = sel[0];

	if (KeySequenceDialog::staticDialog (item->keyConfig(), this))
		setShortcutText (item);
}

// =============================================================================
// Reset a shortcut to defaults
// =============================================================================
void ConfigDialog::slot_resetShortcut()
{
	QList<ShortcutListItem*> sel = getShortcutSelection();

	for (ShortcutListItem* item : sel)
	{
		item->keyConfig()->reset();
		setShortcutText (item);
	}
}

// =============================================================================
// Remove the shortcut of an action.
// =============================================================================
void ConfigDialog::slot_clearShortcut()
{
	QList<ShortcutListItem*> sel = getShortcutSelection();

	for (ShortcutListItem* item : sel)
	{
		item->keyConfig()->setValue (QKeySequence());
		setShortcutText (item);
	}
}

// =============================================================================
// Set the path of an external program
// =============================================================================
void ConfigDialog::slot_setExtProgPath()
{
	const LDExtProgInfo* info = null;

	for (const LDExtProgInfo& it : g_LDExtProgInfo)
	{
		if (it.setPathButton == sender())
		{
			info = &it;
			break;
		}
	}

	assert (info != null);
	QString fpath = QFileDialog::getOpenFileName (this, format ("Path to %1", info->name), *info->path, g_extProgPathFilter);

	if (fpath.isEmpty())
		return;

	info->input->setText (fpath);
}

// =============================================================================
//
// '...' button pressed for the download path
//
void ConfigDialog::slot_findDownloadFolder()
{
	QString dpath = QFileDialog::getExistingDirectory();
	ui->downloadPath->setText (dpath);
}

// =============================================================================
//
// Updates the text string for a given shortcut list item
//
void ConfigDialog::setShortcutText (ShortcutListItem* item)
{
	QAction* act = item->action();
	QString label = act->iconText();
	QString keybind = item->keyConfig()->getValue().toString();
	item->setText (format ("%1 (%2)", label, keybind));
}

// =============================================================================
// Gets the configuration string of the quick color toolbar
// =============================================================================
QString ConfigDialog::quickColorString()
{
	QString val;

	for (const LDQuickColor& entry : quickColors)
	{
		if (val.length() > 0)
			val += ':';

		if (entry.isSeparator())
			val += '|';
		else
			val += format ("%1", entry.color()->index);
	}

	return val;
}

// ===============================================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// ===============================================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// ===============================================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// ===============================================================================================
KeySequenceDialog::KeySequenceDialog (QKeySequence seq, QWidget* parent, Qt::WindowFlags f) :
	QDialog (parent, f), seq (seq)
{
	lb_output = new QLabel;
	IMPLEMENT_DIALOG_BUTTONS

	setWhatsThis (tr ("Into this dialog you can input a key sequence for use as a "
		"shortcut in LDForge. Use OK to confirm the new shortcut and Cancel to "
		"dismiss."));

	QVBoxLayout* layout = new QVBoxLayout;
	layout->addWidget (lb_output);
	layout->addWidget (bbx_buttons);
	setLayout (layout);

	updateOutput();
}

// =============================================================================
// =============================================================================
bool KeySequenceDialog::staticDialog (KeySequenceConfig* cfg, QWidget* parent)
{
	KeySequenceDialog dlg (cfg->getValue(), parent);

	if (dlg.exec() == QDialog::Rejected)
		return false;

	cfg->setValue (dlg.seq);
	return true;
}

// =============================================================================
// =============================================================================
void KeySequenceDialog::updateOutput()
{
	QString shortcut = seq.toString();

	if (seq == QKeySequence())
		shortcut = "&lt;empty&gt;";

	QString text = format ("<center><b>%1</b></center>", shortcut);
	lb_output->setText (text);
}

// =============================================================================
// =============================================================================
void KeySequenceDialog::keyPressEvent (QKeyEvent* ev)
{
	seq = ev->key() + ev->modifiers();
	updateOutput();
}
