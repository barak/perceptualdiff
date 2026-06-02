/*
RGBAImage.cpp
Copyright (C) 2006-2011 Yangli Hector Yee
Copyright (C) 2011-2016 Steven Myint, Jeff Terrace

(This entire file was rewritten by Jim Tilander)

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

#include "rgba_image.h"

#include <gdk-pixbuf/gdk-pixbuf.h>

#include <algorithm>
#include <cassert>
#include <ciso646>
#include <cctype>
#include <cstring>
#include <string>


namespace pdiff
{
    struct PixbufDeleter
    {
        inline void operator()(GdkPixbuf *image)
        {
            if (image)
            {
                g_object_unref(image);
            }
        }
    };


    static std::shared_ptr<GdkPixbuf> to_pixbuf(const RGBAImage &image)
    {
        const auto *data = image.get_data();

        std::shared_ptr<GdkPixbuf> pixbuf(
            gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8,
                           static_cast<int>(image.get_width()),
                           static_cast<int>(image.get_height())),
            PixbufDeleter());
        assert(pixbuf.get());

        const auto rowstride = gdk_pixbuf_get_rowstride(pixbuf.get());
        auto *pixels = gdk_pixbuf_get_pixels(pixbuf.get());

        for (auto y = 0u; y < image.get_height();
             y++, data += image.get_width())
        {
            auto *row = pixels + static_cast<size_t>(y) * rowstride;
            for (auto x = 0u; x < image.get_width(); ++x)
            {
                const auto pixel = data[x];
                row[x * 4] = pixel & 0xff;
                row[x * 4 + 1] = (pixel >> 8) & 0xff;
                row[x * 4 + 2] = (pixel >> 16) & 0xff;
                row[x * 4 + 3] = (pixel >> 24) & 0xff;
            }
        }

        return pixbuf;
    }


    static std::shared_ptr<RGBAImage> to_rgba_image(GdkPixbuf *image,
                                                    const std::string &filename="")
    {
        const auto w = static_cast<unsigned int>(gdk_pixbuf_get_width(image));
        const auto h = static_cast<unsigned int>(gdk_pixbuf_get_height(image));
        const auto channels = gdk_pixbuf_get_n_channels(image);
        const auto rowstride = gdk_pixbuf_get_rowstride(image);
        const auto *pixels = gdk_pixbuf_read_pixels(image);
        const auto has_alpha = gdk_pixbuf_get_has_alpha(image);

        auto result = std::make_shared<RGBAImage>(w, h, filename);
        for (unsigned int y = 0; y < h; y++)
        {
            const auto *row = pixels + static_cast<size_t>(y) * rowstride;
            for (unsigned int x = 0; x < w; ++x)
            {
                const auto pixel = row + x * channels;
                const auto alpha = has_alpha ? pixel[3] : 255;
                result->set(pixel[0], pixel[1], pixel[2], alpha, x + y * w);
            }
        }

        return result;

    }


    static std::string to_lower_copy(std::string text)
    {
        std::transform(text.begin(), text.end(), text.begin(),
                       [](unsigned char value)
                       {
                           return static_cast<char>(std::tolower(value));
                       });
        return text;
    }


    static std::string pixbuf_type_from_filename(const std::string &filename)
    {
        const auto dot = filename.find_last_of('.');
        if (dot == std::string::npos)
        {
            return "";
        }

        const auto extension = to_lower_copy(filename.substr(dot + 1));
        if (extension == "jpg" or extension == "jpeg")
        {
            return "jpeg";
        }
        if (extension == "tif" or extension == "tiff")
        {
            return "tiff";
        }
        if (extension == "png" or extension == "bmp" or extension == "gif" or
            extension == "ico" or extension == "pnm" or extension == "xpm")
        {
            return extension;
        }

        return "";
    }

    std::shared_ptr<RGBAImage> RGBAImage::down_sample(unsigned int w,
                                                      unsigned int h) const
    {
        if (w == 0)
        {
            w = width_ / 2;
        }

        if (h == 0)
        {
            h = height_ / 2;
        }

        if (width_ <= 1 or height_ <= 1)
        {
            return nullptr;
        }
        if (width_ == w and height_ == h)
        {
            return nullptr;
        }
        assert(w <= width_);
        assert(h <= height_);

        auto pixbuf = to_pixbuf(*this);
        std::unique_ptr<GdkPixbuf, PixbufDeleter> converted(
            gdk_pixbuf_scale_simple(pixbuf.get(), static_cast<int>(w),
                                    static_cast<int>(h), GDK_INTERP_HYPER));
        if (not converted)
        {
            throw RGBImageException("Failed to scale image " + name_);
        }

        auto img = to_rgba_image(converted.get(), name_);

        return img;
    }

    void RGBAImage::write_to_file(const std::string &filename) const
    {
        const auto file_type = pixbuf_type_from_filename(filename);
        if (file_type.empty())
        {
            throw RGBImageException("Can't save to unknown filetype '" +
                                    filename + "'");
        }

        auto pixbuf = to_pixbuf(*this);
        GError *error = nullptr;

        const bool result = !!gdk_pixbuf_save(pixbuf.get(), filename.c_str(),
                                              file_type.c_str(), &error, nullptr);
        if (not result)
        {
            auto reason = std::string();
            if (error and error->message)
            {
                reason = ": " + std::string(error->message);
            }
            if (error)
            {
                g_error_free(error);
            }
            throw RGBImageException("Failed to save to '" + filename + "'" + reason);
        }
    }

    std::shared_ptr<RGBAImage> read_from_file(const std::string &filename)
    {
        if (not gdk_pixbuf_get_file_info(filename.c_str(), nullptr, nullptr))
        {
            throw RGBImageException("Unknown filetype '" + filename + "'");
        }

        GError *error = nullptr;
        std::unique_ptr<GdkPixbuf, PixbufDeleter> pixbuf(
            gdk_pixbuf_new_from_file(filename.c_str(), &error));
        if (not pixbuf)
        {
            auto reason = std::string();
            if (error and error->message)
            {
                reason = ": " + std::string(error->message);
            }
            if (error)
            {
                g_error_free(error);
            }
            throw RGBImageException("Failed to load the image " + filename + reason);
        }

        return to_rgba_image(pixbuf.get(), filename);
    }
}
