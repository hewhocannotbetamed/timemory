//  MIT License
//
//  Copyright (c) 2020, The Regents of the University of California,
//  through Lawrence Berkeley National Laboratory (subject to receipt of any
//  required approvals from the U.S. Dept. of Energy).  All rights reserved.
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to
//  deal in the Software without restriction, including without limitation the
//  rights to use, copy, modify, merge, publish, distribute, sublicense, and
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
//  IN THE SOFTWARE.

#pragma once

#if !defined(TIMEMORY_PYBIND11_SOURCE)
#    define TIMEMORY_PYBIND11_SOURCE
#endif

#include "timemory/utility/macros.hpp"
//
#include "timemory/enum.h"
#include "timemory/runtime/configure.hpp"
#include "timemory/runtime/enumerate.hpp"
#include "timemory/runtime/initialize.hpp"
#include "timemory/runtime/insert.hpp"
#include "timemory/runtime/invoker.hpp"
#include "timemory/runtime/properties.hpp"
#include "timemory/timemory.hpp"

#include "pybind11/cast.h"
#include "pybind11/eval.h"
#include "pybind11/functional.h"
#include "pybind11/iostream.h"
#include "pybind11/pybind11.h"
#include "pybind11/pytypes.h"
#include "pybind11/stl.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <future>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <ostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace py = pybind11;
using namespace std::placeholders;  // for _1, _2, _3...
using namespace py::literals;
using namespace tim::component;

using manager_t = tim::manager;

extern "C"
{
    extern PyObject*      PyEval_GetBuiltins();
    extern PyThreadState* PyThreadState_Get();
}

namespace pytim
{
using string_t = std::string;
//
//--------------------------------------------------------------------------------------//
//
inline auto
get_ostream_handle(py::object file_handle)
{
    using pystream_t  = py::detail::pythonbuf;
    using cxxstream_t = std::ostream;

    if(!(py::hasattr(file_handle, "write") && py::hasattr(file_handle, "flush")))
    {
        throw py::type_error(
            "get_ostream_handle(file_handle): incompatible function argument:  "
            "`file` must be a file-like object, but `" +
            (std::string)(py::repr(file_handle)) + "` provided");
    }
    auto buf    = std::make_shared<pystream_t>(file_handle);
    auto stream = std::make_shared<cxxstream_t>(buf.get());
    return std::make_pair(stream, buf);
}
//
//======================================================================================//
//
//                              MANAGER
//
//======================================================================================//
//
namespace manager
{
//
//--------------------------------------------------------------------------------------//
//
string_t
write_ctest_notes(py::object man, std::string directory, bool append)
{
    py::list filenames = man.attr("text_files").cast<py::list>();

    std::stringstream ss;
    ss << std::endl;
    ss << "IF(NOT DEFINED CTEST_NOTES_FILES)" << std::endl;
    ss << "    SET(CTEST_NOTES_FILES )" << std::endl;
    ss << "ENDIF(NOT DEFINED CTEST_NOTES_FILES)" << std::endl;
    ss << std::endl;

    // loop over ASCII report filenames
    for(auto itr : filenames)
    {
        std::string fname = itr.cast<std::string>();
#if defined(_WIN32) || defined(_WIN64)
        while(fname.find("\\") != std::string::npos)
            fname = fname.replace(fname.find("\\"), 1, "/");
#endif
        ss << "LIST(APPEND CTEST_NOTES_FILES \"" << fname << "\")" << std::endl;
    }

    ss << std::endl;
    ss << "IF(NOT \"${CTEST_NOTES_FILES}\" STREQUAL \"\")" << std::endl;
    ss << "    LIST(REMOVE_DUPLICATES CTEST_NOTES_FILES)" << std::endl;
    ss << "ENDIF(NOT \"${CTEST_NOTES_FILES}\" STREQUAL \"\")" << std::endl;
    ss << std::endl;

    tim::makedir(directory);
    std::string file_path =
        tim::utility::path{ TIMEMORY_JOIN("/", directory, "CTestNotes.cmake") };
    std::ofstream outf;
    if(append)
        outf.open(file_path.c_str(), std::ios::app);
    else
        outf.open(file_path.c_str());

    if(outf)
        outf << ss.str();
    outf.close();

    return file_path;
}
//
//--------------------------------------------------------------------------------------//
//
}  // namespace manager
//
//======================================================================================//
//
//                          OPTIONS
//
//======================================================================================//
//
namespace opt
{
//
//--------------------------------------------------------------------------------------//
//
void
safe_mkdir(string_t directory)
{
    tim::makedir(directory);
}
//
//--------------------------------------------------------------------------------------//
//
void
ensure_directory_exists(string_t file_path)
{
    auto npos = std::string::npos;
    if((npos = file_path.find_last_of("/\\")) != std::string::npos)
    {
        auto dir = file_path.substr(0, npos);
        tim::makedir(dir);
    }
}
//
//--------------------------------------------------------------------------------------//
//
py::object
parse_args(py::object parser)
{
    auto msettings                      = py::module::import("timemory.settings");
    auto moptions                       = py::module::import("timemory.options");
    auto args                           = parser.attr("parse_args")();
    msettings.attr("dart_output")       = args.attr("timemory_echo_dart");
    moptions.attr("matplotlib_backend") = args.attr("timemory_mpl_backend");
    return args;
}
//
//--------------------------------------------------------------------------------------//
//
py::object
parse_known_args(py::object parser)
{
    auto       msys                     = py::module::import("sys");
    auto       msettings                = py::module::import("timemory.settings");
    auto       moptions                 = py::module::import("timemory.options");
    py::list   kargs                    = parser.attr("parse_known_args")();
    py::object args                     = kargs[0];
    py::list   left                     = kargs[1].cast<py::list>();
    msys.attr("argv")                   = msys.attr("argv")[0].cast<py::list>() + left;
    msettings.attr("dart_output")       = args.attr("timemory_echo_dart");
    moptions.attr("matplotlib_backend") = args.attr("timemory_mpl_backend");
    return args;
}
//
//--------------------------------------------------------------------------------------//
//
}  // namespace opt
}  // namespace pytim
