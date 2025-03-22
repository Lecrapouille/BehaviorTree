//*****************************************************************************
// A C++ behavior tree lib https://github.com/Lecrapouille/BehaviorTree
//
// MIT License
//
// Copyright (c) 2025 Quentin Quadrat <lecrapouille@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//*****************************************************************************

#include "Path.hpp"
#include <sstream>
#include <sys/stat.h>

namespace bt {

//------------------------------------------------------------------------------
Path::Path(std::string const& path, char const delimiter)
    : m_delimiter(delimiter)
{
    add(path);
}

//------------------------------------------------------------------------------
void Path::add(std::string const& path)
{
    if (!path.empty())
    {
        split(path);
    }
}

//------------------------------------------------------------------------------
void Path::reset(std::string const& path)
{
    m_search_paths.clear();
    split(path);
}

//------------------------------------------------------------------------------
void Path::clear()
{
    m_search_paths.clear();
}

//------------------------------------------------------------------------------
void Path::remove(std::string const& path)
{
    m_search_paths.remove(path);
}

//------------------------------------------------------------------------------
bool Path::exist(std::string const& path) const
{
    struct stat buffer;
    return stat(path.c_str(), &buffer) == 0;
}

//------------------------------------------------------------------------------
std::pair<std::string, bool> Path::find(std::string const& filename) const
{
    if (Path::exist(filename))
        return std::make_pair(filename, true);

    for (auto const& it : m_search_paths)
    {
        std::string file(it + filename);
        if (Path::exist(file))
            return std::make_pair(file, true);
    }

    // Not found
    return std::make_pair(std::string(), false);
}

//------------------------------------------------------------------------------
std::string Path::expand(std::string const& filename) const
{
    for (auto const& it : m_search_paths)
    {
        std::string file(it + filename);
        if (Path::exist(file))
            return file;
    }

    return filename;
}

//------------------------------------------------------------------------------
bool Path::open(std::string& filename,
                std::ifstream& ifs,
                std::ios_base::openmode mode) const
{
    ifs.open(filename.c_str(), mode);
    if (ifs)
        return true;

    for (auto const& it : m_search_paths)
    {
        std::string file(it + filename);
        ifs.open(file.c_str(), mode);
        if (ifs)
        {
            filename = file;
            return true;
        }
    }

    // Not found
    return false;
}

//------------------------------------------------------------------------------
bool Path::open(std::string& filename,
                std::ofstream& ofs,
                std::ios_base::openmode mode) const
{
    ofs.open(filename.c_str(), mode);
    if (ofs)
        return true;

    for (auto const& it : m_search_paths)
    {
        std::string file(it + filename);
        ofs.open(file.c_str(), mode);
        if (ofs)
        {
            filename = file;
            return true;
        }
    }

    // Not found
    return false;
}

//------------------------------------------------------------------------------
bool Path::open(std::string& filename,
                std::fstream& fs,
                std::ios_base::openmode mode) const
{
    fs.open(filename.c_str(), mode);
    if (fs)
        return true;

    for (auto const& it : m_search_paths)
    {
        std::string file(it + filename);
        fs.open(filename.c_str(), mode);
        if (fs)
        {
            filename = file;
            return true;
        }
    }

    // Not found
    return false;
}

//------------------------------------------------------------------------------
std::vector<std::string> Path::pathes() const
{
    std::vector<std::string> res;
    res.reserve(m_search_paths.size());
    for (auto const& it : m_search_paths)
    {
        res.push_back(it);
    }

    return res;
}

//------------------------------------------------------------------------------
std::string Path::toString() const
{
    std::string string_path;

    string_path += ".";
    string_path += m_delimiter;

    for (auto const& it : m_search_paths)
    {
        string_path += it;
        string_path.pop_back(); // Remove the '/' char
        string_path += m_delimiter;
    }

    return string_path;
}

//------------------------------------------------------------------------------
void Path::split(std::string const& path)
{
    std::stringstream ss(path);
    std::string directory;

    while (std::getline(ss, directory, m_delimiter))
    {
        if (directory.empty())
            continue;

        if ((*directory.rbegin() == '\\') || (*directory.rbegin() == '/'))
            m_search_paths.push_back(directory);
        else
            m_search_paths.push_back(directory + "/");
    }
}

} // namespace bt
