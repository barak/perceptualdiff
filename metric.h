/*
Metric
Copyright (C) 2006-2011 Yangli Hector Yee
Copyright (C) 2011-2016 Steven Myint, Jeff Terrace

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc., 59 Temple
Place, Suite 330, Boston, MA 02111-1307 USA
*/

#ifndef PERCEPTUALDIFF_METRIC_H
#define PERCEPTUALDIFF_METRIC_H

#include <ostream>
#include <string>


namespace pdiff
{
    class RGBAImage;


    struct PerceptualDiffParameters
    {
        PerceptualDiffParameters();

        // Only consider luminance; ignore chroma channels in the comparison.
        bool luminance_only;

        // Print a sum of the luminance and color differences of each pixel.
        bool sum_errors;

        // Field of view in degrees.
        float field_of_view;

        // The gamma to convert to linear color space
        float gamma;

        float luminance;

        // How many pixels different to ignore.
        unsigned int threshold_pixels;

        // How much color to use in the metric.
        // 0.0 is the same as luminance_only_ = true,
        // 1.0 means full strength.
        float color_factor;

        // How much to down sample image before comparing, in powers of 2.
        unsigned int down_sample;
    };


    // Image comparison metric using Yee's method.
    // References: A Perceptual Metric for Production Testing, Hector Yee, Journal
    // of Graphics Tools 2004
    //
    // Return true if the images are perceptually the same.
    bool yee_compare(const PerceptualDiffParameters &parameters,
                     const RGBAImage &image_a,
                     const RGBAImage &image_b,
                     std::string &output_error_string,
                     RGBAImage *output_image_difference=NULL,
                     std::ostream *output_verbose=NULL);
}

#endif
