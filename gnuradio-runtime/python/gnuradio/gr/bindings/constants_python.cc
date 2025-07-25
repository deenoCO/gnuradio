/*
 * Copyright 2020 Free Software Foundation, Inc.
 *
 * This file is part of GNU Radio
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

/***********************************************************************************/
/* This file is automatically generated using bindtool and can be manually edited  */
/* The following lines can be configured to regenerate this file during cmake      */
/* If manual edits are made, the following tags should be modified accordingly.    */
/* BINDTOOL_GEN_AUTOMATIC(0)                                                       */
/* BINDTOOL_USE_PYGCCXML(0)                                                        */
/* BINDTOOL_HEADER_FILE(constants.h)                                        */
/* BINDTOOL_HEADER_FILE_HASH(041484ee6adaa786bfff0c081c465c1e)                     */
/***********************************************************************************/

#include <pybind11/complex.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

#include <gnuradio/constants.h>
// pydoc.h is automatically generated in the build directory
#include <constants_pydoc.h>

void bind_constants(py::module& m)
{


    m.def("prefix", &::gr::prefix, D(prefix));


    m.def("sysconfdir", &::gr::sysconfdir, D(sysconfdir));


    m.def("prefsdir", &::gr::prefsdir, D(prefsdir));


    m.def("build_date", &::gr::build_date, D(build_date));


    m.def("version", &::gr::version, D(version));


    m.def("major_version", &::gr::major_version, D(major_version));


    m.def("api_version", &::gr::api_version, D(api_version));


    m.def("minor_version", &::gr::minor_version, D(minor_version));


    m.def("c_compiler", &::gr::c_compiler, D(c_compiler));


    m.def("cxx_compiler", &::gr::cxx_compiler, D(cxx_compiler));


    m.def("compiler_flags", &::gr::compiler_flags, D(compiler_flags));


    m.def("build_time_enabled_components",
          &::gr::build_time_enabled_components,
          D(build_time_enabled_components));
}
