/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 Santeri Piippo
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

#ifndef GLDRAW_H
#define GLDRAW_H

#include <QGLWidget>
#include <qtimer.h>
#include <qdialog.h>
#include "common.h"
#include "ldtypes.h"

class QDialogButtonBox;
class RadioBox;
class QDoubleSpinBox;
class QSpinBox;
class QLineEdit;

// =============================================================================
// GLRenderer
// 
// The main renderer object, draws the brick on the screen, manages the camera
// and selection picking. The instance of GLRenderer is accessible as
// g_win->R ()
// =============================================================================
class GLRenderer : public QGLWidget {
	Q_OBJECT
	
public:
	enum Camera {
		Top,
		Front,
		Left,
		Bottom,
		Back,
		Right,
		Free
	};
	
	enum ListType { NormalList, PickList, BFCFrontList, BFCBackList };
	
	GLRenderer (QWidget* parent = null);
	~GLRenderer ();
	
	void		beginPlaneDraw		();
	Camera		camera				() const { return m_camera; }
	Axis		cameraAxis			(bool y);
	void		clearOverlay		();
	void		compileObject		(LDObject* obj);
	void		compileAllObjects	();
	void		endPlaneDraw		(bool accept);
	QColor		getMainColor		();
	void		hardRefresh		();
	bool		picking				() const { return m_picking; }
	void		refresh			();
	void		resetAngles		();
	uchar*		screencap			(ushort& w, ushort& h);
	void		setBackground		();
	void		setCamera			(const GLRenderer::Camera cam);
	void		setupOverlay		();
	void		setZoom				(const double zoom) { m_zoom = zoom; }
	double		zoom				() const { return m_zoom; }
	
	static void	deleteLists			(LDObject* obj);

protected:
	void	contextMenuEvent	(QContextMenuEvent* ev);
	void	initializeGL		();
	void	keyPressEvent		(QKeyEvent* ev);
	void	keyReleaseEvent	(QKeyEvent* ev);
	void	leaveEvent			(QEvent* ev);
	void	mousePressEvent	(QMouseEvent* ev);
	void	mouseMoveEvent		(QMouseEvent* ev);
	void	mouseReleaseEvent	(QMouseEvent* ev);
	void	paintEvent			(QPaintEvent* ev);
	void	resizeGL			(int w, int h);
	void	wheelEvent			(QWheelEvent* ev);

private:
	QTimer* m_toolTipTimer;
	Qt::MouseButtons m_lastButtons;
	Qt::KeyboardModifiers m_keymods;
	ulong m_totalmove;
	vertex m_hoverpos;
	double m_virtWidth, m_virtHeight, m_rotX, m_rotY, m_rotZ, m_panX, m_panY, m_zoom;
	bool m_darkbg, m_picking, m_rangepick, m_addpick, m_drawToolTip, m_screencap, m_planeDraw;
	QPoint m_pos, m_rangeStart;
	QPen m_thinBorderPen, m_thickBorderPen;
	Camera m_camera, m_toolTipCamera;
	uint m_axeslist;
	ushort m_width, m_height;
	std::vector<vertex> m_planeDrawVerts;
	
	void	calcCameraIcons	();												// Compute geometry for camera icons
	void	clampAngle			(double& angle) const;							// Clamps an angle to [0, 360]
	void	compileList			(LDObject* obj, const ListType list);			// Compile one of the lists of an object
	void	compileSubObject	(LDObject* obj, const GLenum gltype);			// Sub-routine for object compiling
	void	compileVertex		(const vertex& vrt);							// Compile a single vertex to a list
	vertex	coordconv2_3		(const QPoint& pos2d, bool snap) const;		// Convert a 2D point to a 3D point
	QPoint	coordconv3_2		(const vertex& pos3d) const;					// Convert a 3D point to a 2D point
	void	drawGLScene		() const;										// Paint the GL scene
	void	pick				(uint mouseX, uint mouseY);					// Perform object selection
	void	setObjectColor		(LDObject* obj, const ListType list);			// Set the color to an object list
	
private slots:
	void	slot_toolTipTimer	();
};

// Alias for short namespaces
typedef GLRenderer GL;

static const GLRenderer::ListType g_glListTypes[] = {
	GL::NormalList,
	GL::PickList,
	GL::BFCFrontList,
	GL::BFCBackList,
};

class OverlayDialog : public QDialog {
	Q_OBJECT
	
public:
	explicit OverlayDialog (QWidget* parent = null, Qt::WindowFlags f = 0);
	
	str			fpath		() const;
	ushort		ofsx		() const;
	ushort		ofsy		() const;
	double		lwidth		() const;
	double		lheight		() const;
	GL::Camera	camera		() const;
	
private:
	RadioBox* rb_camera;
	QPushButton* btn_fpath;
	QLineEdit* le_fpath;
	QSpinBox* sb_ofsx, *sb_ofsy;
	QDoubleSpinBox* dsb_lwidth, *dsb_lheight;
	QDialogButtonBox* dbb_buttons;
	
private slots:
	void slot_fpath ();
	void slot_help ();
	void slot_dimensionsChanged ();
};

#endif // GLDRAW_H