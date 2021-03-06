/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013, 2014 Santeri Piippo
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once
#include <QString>
#include <QObject>
#include <QStringList>
#include <QMetaType>
#include <QVector3D>
#include <QSharedPointer>
#include "macros.h"

class LDObject;
class QFile;
class QTextStream;
class Matrix;
class LDDocument;

using int8 = qint8;
using int16 = qint16;
using int32 = qint32;
using int64 = qint64;
using uint8 = quint8;
using uint16 = quint16;
using uint32 = quint32;
using uint64 = quint64;
using LDObjectPtr = QSharedPointer<LDObject>;
using LDObjectList = QList<LDObjectPtr>;
using LDObjectWeakPtr = QWeakPointer<LDObject>;
using LDObjectWeakList = QList<LDObjectWeakPtr>;
using LDDocumentPtr = QSharedPointer<LDDocument>;
using LDDocumentWeakPtr = QWeakPointer<LDDocument>;

template<typename T, typename R>
using Pair = std::pair<T, R>;

enum Axis
{
	X,
	Y,
	Z
};

//
// Derivative of QVector3D: this class is used for the vertices.
//
class Vertex : public QVector3D
{
public:
	using ApplyFunction = std::function<void (Axis, double&)>;
	using ApplyConstFunction = std::function<void (Axis, double)>;

	Vertex();
	Vertex (const QVector3D& a);
	Vertex (qreal xpos, qreal ypos, qreal zpos);

	void	apply (ApplyFunction func);
	void	apply (ApplyConstFunction func) const;
	QString	toString (bool mangled = false) const;
	void	transform (const Matrix& matr, const Vertex& pos);
	void	setCoordinate (Axis ax, qreal value);

	bool	operator< (const Vertex& other) const;
	double	operator[] (Axis ax) const;
};

Q_DECLARE_METATYPE (Vertex)

//
// A mathematical 3 x 3 matrix
//
class Matrix
{
public:
	Matrix() {}
	Matrix (const std::initializer_list<double>& vals);

	// Constructs a matrix all 9 elements initialized to the same value.
	Matrix (double fillval);

	// Constructs a matrix with a C-array.
	// note: @vals is expected to have exactly 9 elements.
	Matrix (double vals[]);

	// Calculates the matrix's determinant.
	double			getDeterminant() const;

	// Multiplies this matrix with @other
	// note: a.mult(b) is not equivalent to b.mult(a)!
	Matrix			mult (const Matrix& other) const;

	// Prints the matrix to stdout.
	void			dump() const;

	// Yields a string representation of the matrix.
	QString			toString() const;

	// Zeroes the matrix out.
	void			zero();

	// Assigns the matrix values to the values of @other.
	Matrix&			operator= (const Matrix& other);

	// Returns a mutable reference to a value by @idx
	inline double& value (int idx)
	{
		return m_vals[idx];
	}

	// An overload of value() for const matrices.
	inline const double& value (int idx) const
	{
		return m_vals[idx];
	}

	// An operator overload for mult().
	inline Matrix operator* (const Matrix& other) const
	{
		return mult (other);
	}

	// An operator overload for value().
	inline double& operator[] (int idx)
	{
		return value (idx);
	}

	// An operator overload for value() const.
	inline const double& operator[] (int idx) const
	{
		return value (idx);
	}

	// Checks whether the two matrices have the same values.
	bool operator== (const Matrix& other) const;

private:
	double m_vals[9];
};

//
// Defines a bounding box that encompasses a given set of objects.
// vertex0 is the minimum vertex, vertex1 is the maximum vertex.
//
class LDBoundingBox
{
	PROPERTY (private,	bool,		isEmpty,	setEmpty,		STOCK_WRITE)
	PROPERTY (private,	Vertex,		vertex0,	setVertex0,		STOCK_WRITE)
	PROPERTY (private,	Vertex,		vertex1,	setVertex1,		STOCK_WRITE)

public:
	// Constructs an empty bounding box.
	LDBoundingBox();

	// Clears the bounding box
	void reset();

	// Calculates the bounding box's values from the objects in the current
	// document.
	void calculateFromCurrentDocument();

	// Returns the length of the bounding box on the longest measure.
	double longestMeasurement() const;

	// Calculates the given \c obj to the bounding box, adjusting
	// extremas if necessary.
	void calcObject (LDObjectPtr obj);

	// Calculates the given \c vertex to the bounding box, adjusting
	// extremas if necessary.
	void calcVertex (const Vertex& vertex);

	// Yields the center of the bounding box.
	Vertex center() const;

	// An operator overload for calcObject()
	LDBoundingBox& operator<< (LDObjectPtr obj);

	// An operator overload for calcVertex()
	LDBoundingBox& operator<< (const Vertex& v);
};

extern const Vertex g_origin; // Vertex at (0, 0, 0)
extern const Matrix g_identity; // Identity matrix

static const double pi = 3.14159265358979323846;
