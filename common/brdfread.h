// Copyright 2005 Mitsubishi Electric Research Laboratories All Rights Reserved.

// Permission to use, copy and modify this software and its documentation without
// fee for educational, research and non-profit purposes, is hereby granted, provided
// that the above copyright notice and the following three paragraphs appear in all copies.

// To request permission to incorporate this software into commercial products contact:
// Vice President of Marketing and Business Development;
// Mitsubishi Electric Research Laboratories (MERL), 201 Broadway, Cambridge, MA 02139 or
// <license@merl.com>.

// IN NO EVENT SHALL MERL BE LIABLE TO ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL,
// OR CONSEQUENTIAL DAMAGES, INCLUDING LOST PROFITS, ARISING OUT OF THE USE OF THIS SOFTWARE AND
// ITS DOCUMENTATION, EVEN IF MERL HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.

// MERL SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED
// HEREUNDER IS ON AN "AS IS" BASIS, AND MERL HAS NO OBLIGATIONS TO PROVIDE MAINTENANCE, SUPPORT,
// UPDATES, ENHANCEMENTS OR MODIFICATIONS.

// The Implementation has been moved to "brdfread.cpp" for compatibility
#ifndef BRDFREAD_H
#define BRDFREAD_H

#include <cmath>
#include <common/color.h>
#include <cstdlib>
#include <iostream>

#define BRDF_SAMPLING_RES_THETA_H 90
#define BRDF_SAMPLING_RES_THETA_D 90
#define BRDF_SAMPLING_RES_PHI_D 360

#define RED_SCALE (1.0 / 1500.0)
#define GREEN_SCALE (1.15 / 1500.0)
#define BLUE_SCALE (1.66 / 1500.0)
#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif

class BRDFRead {
public:
  BRDFRead(const char *filename);

  /**
   * Given a pair of incoming/outgoing angles, look up the BRDF.
   */
  Color lookupBrdfValues(double theta_in, double phi_in, double theta_out, double phi_out);

private:
  /**
   * Read BRDF data
   */
  bool readBrdf(const char *filename);

  /**
   * cross product of two vectors
   */
  void cross_product(double *v1, double *v2, double *out);

  /**
   * normalize vector
   */
  void normalize(double *v);

  /**
   * rotate vector along one axis
   */
  void rotate_vector(double *vector, double *axis, double angle, double *out);

  /**
   * convert standard coordinates to half vector/difference vector coordinates
   */
  void std_coords_to_half_diff_coords(double theta_in, double phi_in, double theta_out, double phi_out,
                                      double &theta_half, double &phi_half, double &theta_diff, double &phi_diff);

  /**
   * Lookup theta_half index
   * This is a non-linear mapping!
   * In:  [0 .. pi/2]
   * Out: [0 .. 89]
   */
  inline int theta_half_index(double theta_half);

  /**
   * Lookup theta_diff index
   * In:  [0 .. pi/2]
   * Out: [0 .. 89]
   */
  inline int theta_diff_index(double theta_diff);

  /**
   * Lookup phi_diff index
   */
  inline int phi_diff_index(double phi_diff);

  double *brdfData;
};

#endif
