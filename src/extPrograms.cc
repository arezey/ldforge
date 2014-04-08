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
 */

#include <QProcess>
#include <QTemporaryFile>
#include <QDialog>
#include <QDialogButtonBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include "main.h"
#include "configuration.h"
#include "miscallenous.h"
#include "mainWindow.h"
#include "ldDocument.h"
#include "radioGroup.h"
#include "editHistory.h"
#include "ui_ytruder.h"
#include "ui_intersector.h"
#include "ui_rectifier.h"
#include "ui_coverer.h"
#include "ui_isecalc.h"
#include "ui_edger2.h"
#include "dialogs.h"

enum extprog
{
	Isecalc,
	Intersector,
	Coverer,
	Ytruder,
	Rectifier,
	Edger2,
};

// =============================================================================
//
cfg (String, prog_isecalc, "");
cfg (String, prog_intersector, "");
cfg (String, prog_coverer, "");
cfg (String, prog_ytruder, "");
cfg (String, prog_rectifier, "");
cfg (String, prog_edger2, "");

QString* const g_extProgPaths[] =
{
	&prog_isecalc,
	&prog_intersector,
	&prog_coverer,
	&prog_ytruder,
	&prog_rectifier,
	&prog_edger2,
};

#ifndef _WIN32
cfg (Bool, prog_isecalc_wine, false);
cfg (Bool, prog_intersector_wine, false);
cfg (Bool, prog_coverer_wine, false);
cfg (Bool, prog_ytruder_wine, false);
cfg (Bool, prog_rectifier_wine, false);
cfg (Bool, prog_edger2_wine, false);

bool* const g_extProgWine[] =
{
	&prog_isecalc_wine,
	&prog_intersector_wine,
	&prog_coverer_wine,
	&prog_ytruder_wine,
	&prog_rectifier_wine,
	&prog_edger2_wine,
};
#endif // _WIN32

const char* g_extProgNames[] =
{
	"Isecalc",
	"Intersector",
	"Coverer",
	"Ytruder",
	"Rectifier",
	"Edger2"
};

// =============================================================================
//
static bool mkTempFile (QTemporaryFile& tmp, QString& fname)
{
	if (!tmp.open())
		return false;

	fname = tmp.fileName();
	tmp.close();
	return true;
}

// =============================================================================
//
static bool checkProgPath (const extprog prog)
{
	QString& path = *g_extProgPaths[prog];

	if (path.length() > 0)
		return true;

	ExtProgPathPrompt* dlg = new ExtProgPathPrompt (g_extProgNames[prog]);

	if (dlg->exec() && !dlg->getPath().isEmpty())
	{
		path = dlg->getPath();
		return true;
	}

	return false;
}

// =============================================================================
//
static QString processErrorString (extprog prog, QProcess& proc)
{
	switch (proc.error())
	{
		case QProcess::FailedToStart:
		{
			QString wineblurb;

#ifndef _WIN32
			if (*g_extProgWine[prog])
				wineblurb = "make sure Wine is installed and ";
#endif

			return format ("Program failed to start, %1check your permissions", wineblurb);
		} break;

		case QProcess::Crashed:
			return "Crashed.";

		case QProcess::WriteError:
		case QProcess::ReadError:
			return "I/O error.";

		case QProcess::UnknownError:
			return "Unknown error";

		case QProcess::Timedout:
			return format ("Timed out (30 seconds)");
	}

	return "";
}

// =============================================================================
//
static void writeObjects (const LDObjectList& objects, QFile& f)
{
	for (LDObject* obj : objects)
	{
		if (obj->type() == LDObject::ESubfile)
		{
			LDSubfile* ref = static_cast<LDSubfile*> (obj);
			LDObjectList objs = ref->inlineContents (true, false);

			writeObjects (objs, f);

			for (LDObject* obj : objs)
				obj->destroy();
		}
		else
			f.write ((obj->asText() + "\r\n").toUtf8());
	}
}

// =============================================================================
//
static void writeObjects (const LDObjectList& objects, QString fname)
{
	// Write the input file
	QFile f (fname);

	if (!f.open (QIODevice::WriteOnly | QIODevice::Text))
	{
		critical (format ("Couldn't open temporary file %1 for writing: %2\n", fname, f.errorString()));
		return;
	}

	writeObjects (objects, f);
	f.close();

#ifdef DEBUG
	QFile::copy (fname, "debug_lastInput");
#endif
}

// =============================================================================
//
void writeSelection (QString fname)
{
	writeObjects (selection(), fname);
}

// =============================================================================
//
void writeColorGroup (const int colnum, QString fname)
{
	LDObjectList objects;

	for (LDObject* obj : getCurrentDocument()->objects())
	{
		if (obj->isColored() == false || obj->color() != colnum)
			continue;

		objects << obj;
	}

	writeObjects (objects, fname);
}

// =============================================================================
//
bool runUtilityProcess (extprog prog, QString path, QString argvstr)
{
	QTemporaryFile input;
	QStringList argv = argvstr.split (" ", QString::SkipEmptyParts);

#ifndef _WIN32
	if (*g_extProgWine[prog])
	{
		argv.insert (0, path);
		path = "wine";
	}
#endif // _WIN32

	print ("Running command: %1 %2\n", path, argv.join (" "));

	if (!input.open())
		return false;

	QProcess proc;

	// Begin!
	proc.setStandardInputFile (input.fileName());
	proc.start (path, argv);

	if (!proc.waitForStarted())
	{
		critical (format ("Couldn't start %1: %2\n", g_extProgNames[prog], processErrorString (prog, proc)));
		return false;
	}

	// Write an enter, the utility tools all expect one
	input.write ("\n");

	// Wait while it runs
	proc.waitForFinished();

	QString err = "";

	if (proc.exitStatus() != QProcess::NormalExit)
		err = processErrorString (prog, proc);

	// Check the return code
	if (proc.exitCode() != 0)
		err = format ("Program exited abnormally (return code %1).",  proc.exitCode());

	if (!err.isEmpty())
	{
		critical (format ("%1 failed: %2\n", g_extProgNames[prog], err));
		return false;
	}

	return true;
}

// =============================================================================
//
static void insertOutput (QString fname, bool replace, QList<int> colorsToReplace)
{
#ifdef DEBUG
	QFile::copy (fname, "./debug_lastOutput");
#endif // RELEASE

	// Read the output file
	QFile f (fname);

	if (!f.open (QIODevice::ReadOnly))
	{
		critical (format ("Couldn't open temporary file %1 for reading.\n", fname));
		return;
	}

	LDObjectList objs = loadFileContents (&f, null);

	// If we replace the objects, delete the selection now.
	if (replace)
		g_win->deleteSelection();

	for (int colnum : colorsToReplace)
		g_win->deleteByColor (colnum);

	// Insert the new objects
	getCurrentDocument()->clearSelection();

	for (LDObject* obj : objs)
	{
		if (!obj->isScemantic())
		{
			obj->destroy();
			continue;
		}

		getCurrentDocument()->addObject (obj);
		obj->select();
	}

	g_win->doFullRefresh();
}

// =============================================================================
// Interface for Ytruder
// =============================================================================
DEFINE_ACTION (Ytruder, 0)
{
	setlocale (LC_ALL, "C");

	if (!checkProgPath (Ytruder))
		return;

	QDialog* dlg = new QDialog;
	Ui::YtruderUI ui;
	ui.setupUi (dlg);

	if (!dlg->exec())
		return;

	// Read the user's choices
	const enum { Distance, Symmetry, Projection, Radial } mode =
		ui.mode_distance->isChecked()   ? Distance :
		ui.mode_symmetry->isChecked()   ? Symmetry :
		ui.mode_projection->isChecked() ? Projection : Radial;

	const Axis axis =
		ui.axis_x->isChecked() ? X :
		ui.axis_y->isChecked() ? Y : Z;

	const double depth = ui.planeDepth->value(),
				 condAngle = ui.condAngle->value();

	QTemporaryFile indat, outdat;
	QString inDATName, outDATName;

	// Make temp files for the input and output files
	if (!mkTempFile (indat, inDATName) || !mkTempFile (outdat, outDATName))
		return;

	// Compose the command-line arguments
	QString argv = join (
	{
		(axis == X) ? "-x" : (axis == Y) ? "-y" : "-z",
		(mode == Distance) ? "-d" : (mode == Symmetry) ? "-s" : (mode == Projection) ? "-p" : "-r",
		depth,
		"-a",
		condAngle,
		inDATName,
		outDATName
	});

	writeSelection (inDATName);

	if (!runUtilityProcess (Ytruder, prog_ytruder, argv))
		return;

	insertOutput (outDATName, false, {});
}

// =============================================================================
// Rectifier interface
// =============================================================================
DEFINE_ACTION (Rectifier, 0)
{
	setlocale (LC_ALL, "C");

	if (!checkProgPath (Rectifier))
		return;

	QDialog* dlg = new QDialog;
	Ui::RectifierUI ui;
	ui.setupUi (dlg);

	if (!dlg->exec())
		return;

	QTemporaryFile indat, outdat;
	QString inDATName, outDATName;

	// Make temp files for the input and output files
	if (!mkTempFile (indat, inDATName) || !mkTempFile (outdat, outDATName))
		return;

	// Compose arguments
	QString argv = join (
	{
		(!ui.cb_condense->isChecked()) ? "-q" : "",
		(!ui.cb_subst->isChecked()) ? "-r" : "",
		(ui.cb_condlineCheck->isChecked()) ? "-a" : "",
		(ui.cb_colorize->isChecked()) ? "-c" : "",
		"-t",
		ui.dsb_coplthres->value(),
		inDATName,
		outDATName
	});

	writeSelection (inDATName);

	if (!runUtilityProcess (Rectifier, prog_rectifier, argv))
		return;

	insertOutput (outDATName, true, {});
}

// =============================================================================
// Intersector interface
// =============================================================================
DEFINE_ACTION (Intersector, 0)
{
	setlocale (LC_ALL, "C");

	if (!checkProgPath (Intersector))
		return;

	QDialog* dlg = new QDialog;
	Ui::IntersectorUI ui;
	ui.setupUi (dlg);

	makeColorComboBox (ui.cmb_incol);
	makeColorComboBox (ui.cmb_cutcol);
	ui.cb_repeat->setWhatsThis ("If this is set, " APPNAME " runs Intersector a second time with inverse files to cut the "
								" cutter group with the input group. Both groups are cut by the intersection.");
	ui.cb_edges->setWhatsThis ("Makes " APPNAME " try run Isecalc to create edgelines for the intersection.");

	int inCol, cutCol;
	const bool repeatInverse = ui.cb_repeat->isChecked();

	forever
	{
		if (!dlg->exec())
			return;

		inCol = ui.cmb_incol->itemData (ui.cmb_incol->currentIndex()).toInt();
		cutCol =  ui.cmb_cutcol->itemData (ui.cmb_cutcol->currentIndex()).toInt();

		if (inCol == cutCol)
		{
			critical ("Cannot use the same color group for both input and cutter!");
			continue;
		}

		break;
	}

	// Five temporary files!
	// indat = input group file
	// cutdat = cutter group file
	// outdat = primary output
	// outdat2 = inverse output
	// edgesdat = edges output (isecalc)
	QTemporaryFile indat, cutdat, outdat, outdat2, edgesdat;
	QString inDATName, cutDATName, outDATName, outDAT2Name, edgesDATName;

	if (!mkTempFile (indat, inDATName) || !mkTempFile (cutdat, cutDATName) ||
			!mkTempFile (outdat, outDATName) || !mkTempFile (outdat2, outDAT2Name) ||
			!mkTempFile (edgesdat, edgesDATName))
	{
		return;
	}

	QString parms = join (
	{
		(ui.cb_colorize->isChecked()) ? "-c" : "",
		(ui.cb_nocondense->isChecked()) ? "-t" : "",
		"-s",
		ui.dsb_prescale->value()
	});

	QString argv_normal = join (
	{
		parms,
		inDATName,
		cutDATName,
		outDATName
	});

	QString argv_inverse = join (
	{
		parms,
		cutDATName,
		inDATName,
		outDAT2Name
	});

	writeColorGroup (inCol, inDATName);
	writeColorGroup (cutCol, cutDATName);

	if (!runUtilityProcess (Intersector, prog_intersector, argv_normal))
		return;

	insertOutput (outDATName, false, {inCol});

	if (repeatInverse && runUtilityProcess (Intersector, prog_intersector, argv_inverse))
		insertOutput (outDAT2Name, false, {cutCol});

	if (
		ui.cb_edges->isChecked() &&
		checkProgPath (Isecalc) &&
		runUtilityProcess (Isecalc, prog_isecalc, join ( {inDATName, cutDATName, edgesDATName}))
	)
		insertOutput (edgesDATName, false, {});
}

// =============================================================================
//
DEFINE_ACTION (Coverer, 0)
{
	setlocale (LC_ALL, "C");

	if (!checkProgPath (Coverer))
		return;

	QDialog* dlg = new QDialog;
	Ui::CovererUI ui;
	ui.setupUi (dlg);
	makeColorComboBox (ui.cmb_col1);
	makeColorComboBox (ui.cmb_col2);

	int in1Col, in2Col;

	forever
	{
		if (!dlg->exec())
			return;

		in1Col = ui.cmb_col1->itemData (ui.cmb_col1->currentIndex()).toInt();
		in2Col = ui.cmb_col2->itemData (ui.cmb_col2->currentIndex()).toInt();

		if (in1Col == in2Col)
		{
			critical ("Cannot use the same color group for both input and cutter!");
			continue;
		}

		break;
	}

	QTemporaryFile in1dat, in2dat, outdat;
	QString in1DATName, in2DATName, outDATName;

	if (!mkTempFile (in1dat, in1DATName) || !mkTempFile (in2dat, in2DATName) || !mkTempFile (outdat, outDATName))
		return;

	QString argv = join (
	{
		(ui.cb_oldsweep->isChecked() ? "-s" : ""),
		(ui.cb_reverse->isChecked() ? "-r" : ""),
		(ui.dsb_segsplit->value() != 0 ? format ("-l %1", ui.dsb_segsplit->value()) : ""),
		(ui.sb_bias->value() != 0 ? format ("-s %1", ui.sb_bias->value()) : ""),
		in1DATName,
		in2DATName,
		outDATName
	});

	writeColorGroup (in1Col, in1DATName);
	writeColorGroup (in2Col, in2DATName);

	if (!runUtilityProcess (Coverer, prog_coverer, argv))
		return;

	insertOutput (outDATName, false, {});
}

// =============================================================================
//
DEFINE_ACTION (Isecalc, 0)
{
	setlocale (LC_ALL, "C");

	if (!checkProgPath (Isecalc))
		return;

	Ui::IsecalcUI ui;
	QDialog* dlg = new QDialog;
	ui.setupUi (dlg);

	makeColorComboBox (ui.cmb_col1);
	makeColorComboBox (ui.cmb_col2);

	int in1Col, in2Col;

	// Run the dialog and validate input
	forever
	{
		if (!dlg->exec())
			return;

		in1Col = ui.cmb_col1->itemData (ui.cmb_col1->currentIndex()).toInt(),
		in2Col = ui.cmb_col1->itemData (ui.cmb_col2->currentIndex()).toInt();

		if (in1Col == in2Col)
		{
			critical ("Cannot use the same color group for both input and cutter!");
			continue;
		}

		break;
	}

	QTemporaryFile in1dat, in2dat, outdat;
	QString in1DATName, in2DATName, outDATName;

	if (!mkTempFile (in1dat, in1DATName) || !mkTempFile (in2dat, in2DATName) || !mkTempFile (outdat, outDATName))
		return;

	QString argv = join (
	{
		in1DATName,
		in2DATName,
		outDATName
	});

	writeColorGroup (in1Col, in1DATName);
	writeColorGroup (in2Col, in2DATName);
	runUtilityProcess (Isecalc, prog_isecalc, argv);
	insertOutput (outDATName, false, {});
}

// =============================================================================
//
DEFINE_ACTION (Edger2, 0)
{
	setlocale (LC_ALL, "C");

	if (!checkProgPath (Edger2))
		return;

	QDialog* dlg = new QDialog;
	Ui::Edger2Dialog ui;
	ui.setupUi (dlg);

	if (!dlg->exec())
		return;

	QTemporaryFile in, out;
	QString inName, outName;

	if (!mkTempFile (in, inName) || !mkTempFile (out, outName))
		return;

	int unmatched = ui.unmatched->currentIndex();

	QString argv = join (
	{
		format ("-p %1", ui.precision->value()),
		format ("-af %1", ui.flatAngle->value()),
		format ("-ac %1", ui.condAngle->value()),
		format ("-ae %1", ui.edgeAngle->value()),
		ui.delLines->isChecked()     ? "-de" : "",
		ui.delCondLines->isChecked() ? "-dc" : "",
		ui.colored->isChecked()      ? "-c" : "",
		ui.bfc->isChecked()          ? "-b" : "",
		ui.convex->isChecked()       ? "-cx" : "",
		ui.concave->isChecked()      ? "-cv" : "",
		unmatched == 0 ? "-u+" : (unmatched == 2 ? "-u-" : ""),
		inName,
		outName,
	});

	writeSelection (inName);

	if (!runUtilityProcess (Edger2, prog_edger2, argv))
		return;

	insertOutput (outName, true, {});
}