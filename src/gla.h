//
//  gla.h
//
//  Header file for the GLA abstraction layer.
//
//  Copyright (C) 2002 Mark R. Shinwell.
//  Copyright (C) 2003 Olly Betts
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//

// Use texture mapped fonts (lots faster, at least with hardware 3d)
#define USE_FNT

#include "wx.h"
#include "aventypes.h"
#include "quaternion.h"

#ifdef USE_FNT
#include "fnt.h"
#endif

class GfxCore;

#define GLA_DEBUG 1

typedef Double glaCoord;
typedef GLuint glaList;

class GLAPoint {
    glaCoord xc, yc, zc;

public:
    GLAPoint(glaCoord x, glaCoord y, glaCoord z) : xc(x), yc(y), zc(z) {}
    ~GLAPoint() {}

    glaCoord GetX() { return xc; }
    glaCoord GetY() { return yc; }
    glaCoord GetZ() { return zc; }
};

class GLAPen {
    double m_Red;
    double m_Green;
    double m_Blue;
    double m_Alpha;
    
public:
    GLAPen();
    ~GLAPen();

    void SetColour(double red, double green, double blue); // arguments in range 0 to 1.0
    void SetAlpha(double alpha);
    void Interpolate(const GLAPen&, double how_far);

    double GetRed() const;
    double GetGreen() const;
    double GetBlue() const;
    double GetAlpha() const;
};

#ifdef AVENGL
class GLACanvas : public wxGLCanvas {
#else
class GLACanvas : public wxWindow {
#endif

#ifdef GLA_DEBUG
    int m_Vertices;
#endif

#ifdef AVENGL
    GLdouble modelview_matrix[16];
    GLdouble projection_matrix[16];
    GLint viewport[4];
#endif

    // Viewing volume diameter:
    glaCoord m_VolumeDiameter;

    // Parameters for plotting data:
    Quaternion m_Rotation;
    Double m_Scale;
    struct {
        Double x;
        Double y;
        Double z;
    } m_Translation;

#ifdef USE_FNT
    fntTexFont m_Font;
#else
    static void * const m_Font;
    static const int m_FontSize;
#endif

    GLUquadric* m_Quadric;
    
    bool m_Perspective;

    Double SetViewportAndProjection();
public:
    GLACanvas(wxWindow* parent, int id, const wxPoint& posn, wxSize size);
    ~GLACanvas();
#ifdef FLYFREE
    Vector3 m_ViewPoint;
#endif
    void FirstShow();

    void Clear();
    void StartDrawing();
    void FinishDrawing();

    void SetVolumeDiameter(glaCoord diameter);
    void SetDataTransform();
    void SetIndicatorTransform();
    void SetQuaternion(Quaternion& q);
    
    glaList CreateList(GfxCore*, void (GfxCore::*generator)());
    void DeleteList(glaList l);
    void DrawList(glaList l);
    
    void SetBackgroundColour(float red, float green, float blue);
    void SetColour(const GLAPen& pen, bool set_transparency = false, double rgb_scale = 1.0);
#if 0 // Unused and won't build on MS Windows
    void SetBlendColour(const GLAPen& pen, bool set_transparency = false, double rgb_scale = 1.0);
#endif
    void SetPolygonColour(GLAPen& pen, bool front, bool set_transparency = false);
   
    void DrawText(glaCoord x, glaCoord y, glaCoord z, const wxString& str);
    void DrawIndicatorText(int x, int y, const wxString& str);
    void GetTextExtent(const wxString& str, int * x_ext, int * y_ext);
    
    void BeginQuadrilaterals();
    void EndQuadrilaterals();
    void BeginLines();
    void EndLines();
    void BeginTriangleStrip();
    void EndTriangleStrip();
    void BeginTriangles();
    void EndTriangles();
    void BeginPolyline();
    void EndPolyline();
    void BeginPolygon();
    void EndPolygon();
    
    void DrawRectangle(GLAPen& edge, GLAPen& fill, GLAPen& fill_top,
                       glaCoord x0, glaCoord y0, glaCoord w, glaCoord h, bool draw_lines = true);
    void DrawCircle(GLAPen& edge, GLAPen& fill, glaCoord cx, glaCoord cy, glaCoord radius);
    void DrawSemicircle(GLAPen& edge, GLAPen& fill, glaCoord cx, glaCoord cy, glaCoord radius, glaCoord start);
    void DrawTriangle(GLAPen& edge, GLAPen& fill, GLAPoint* vertices);
    
    void DrawBlob(GLAPen& pen, glaCoord x, glaCoord y, glaCoord z, glaCoord radius);
    void DrawRing(GLAPen& pen, glaCoord x, glaCoord y, glaCoord radius);
 
    void PlaceVertex(glaCoord x, glaCoord y, glaCoord z);
    void PlaceIndicatorVertex(glaCoord x, glaCoord y);

    void PlaceNormal(glaCoord x, glaCoord y, glaCoord z);
    
    void EnableDashedLines();
    void DisableDashedLines();

    void EnableSmoothPolygons();
    void DisableSmoothPolygons();

    void SetRotation(Quaternion&);
    void SetScale(Double);
    void SetTranslation(Double, Double, Double);
    void AddTranslation(Double, Double, Double);
    void AddTranslationScreenCoordinates(int dx, int dy);

    void Transform(Double x, Double y, Double z, Double* x_out, Double* y_out, Double* z_out);
    void ReverseTransform(Double x, Double y, Double* x_out, Double* y_out, Double* z_out);

    int GetFontSize() const { return m_Font.getFontSize(); }

    Double SurveyUnitsAcrossViewport();

    void TogglePerspective() { m_Perspective = !m_Perspective; }
    bool GetPerspective() const { return m_Perspective; }
};
