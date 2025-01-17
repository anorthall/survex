/* kml.h
 * Export from Aven as KML.
 */
/* Copyright (C) 2005,2013,2014,2015,2016,2017,2018 Olly Betts
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include "exportfilter.h"

#include <proj.h>

#include "vector3.h"

#include <vector>

class KML : public ExportFilter {
    PJ* pj = NULL;
    bool in_linestring = false;
    bool in_wall = false;
    bool in_passage = false;
    bool clamp_to_ground;
    Vector3 v1, v2;
  public:
    KML(const char * input_datum, bool clamp_to_ground_);
    ~KML();
    const int * passes() const;
    void header(const char *, const char *, time_t,
		double, double, double,
		double, double, double);
    void start_pass(int pass);
    void line(const img_point *, const img_point *, unsigned, bool);
    void label(const img_point *, const char *, bool, int);
    void xsect(const img_point *, double, double, double);
    void wall(const img_point *, double, double);
    void passage(const img_point *, double, double, double);
    void tube_end();
    void footer();
};
