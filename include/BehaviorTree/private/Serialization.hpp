//*****************************************************************************
// A C++ behavior tree lib https://github.com/Lecrapouille/BehaviorTree
//
// MIT License
//
// Copyright (c) 2024 Quentin Quadrat <lecrapouille@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//*****************************************************************************

#pragma once

#include <vector>
#include <cstring>
#include <type_traits>
#include <cstdint>

namespace bt {

namespace serialization
{
    //! \brief Base container for serialization
    using Container = std::vector<uint8_t>;
}

// ****************************************************************************
//! \brief Class facilitating the serialization of trivially copyable data into
//! a dynamic byte container.
//!
//! By 'trivially copyable', we mean basic types (int, float), simple structures,
//! or arrays of structures.
//!
//! This class provides a simple interface for serializing and deserializing data.
//! It uses a dynamic byte container to store the serialized data.
//!
//! The serialization is performed using the << and >> operators.
//!
//! The class is designed to be used as a friend class to allow access to the
//! internal container.
// ****************************************************************************
class Serializer
{
public:

    // ------------------------------------------------------------------------
    //! \brief Serialize data into the internal container.
    //!
    //! @tparam DataType Type of data to serialize.
    //! @param[in] serializer The serializer instance.
    //! @param[in] data The data to serialize.
    //! @return Reference to the serializer to chain operations.
    //!
    //! This method allows serializing data into the internal container.
    //! It uses a dynamic byte container to store the serialized data.
    //!
    //! The serialization is performed using the << and >> operators.
    // ------------------------------------------------------------------------
    template <typename DataType>
    friend Serializer& operator<<(Serializer& serializer, DataType const& data)
    {
        // Check that the data type is trivially copyable
        static_assert(
            std::is_standard_layout<DataType>::value,
            "The data type is too complex to be serialized directly");

        size_t size = serializer.m_container.size();
        serializer.m_container.resize(serializer.m_container.size() + sizeof(DataType));
        std::memcpy(serializer.m_container.data() + size, &data, sizeof(DataType));
        return serializer;
    }

    // ------------------------------------------------------------------------
    //! \brief Special overload for strings.
    //!
    //! @param[in] serializer The serializer instance.
    //! @param[in] str The string to serialize.
    //! @return Reference to the serializer to chain operations.
    // ------------------------------------------------------------------------
    friend Serializer& operator<<(Serializer& serializer, const std::string& str)
    {
        uint32_t size = static_cast<uint32_t>(str.size());
        serializer << size;

        if (size > 0)
        {
            size_t buffer_size = serializer.m_container.size();
            serializer.m_container.resize(buffer_size + size);
            std::memcpy(serializer.m_container.data() + buffer_size, str.data(), size);
        }
        return serializer;
    }

    // ------------------------------------------------------------------------
    //! \brief Clean the internal container.
    // ------------------------------------------------------------------------
    inline void clear()
    {
        m_container.clear();
    }

    // ------------------------------------------------------------------------
    //! \brief Get the serialized data.
    //!
    //! @return Constant reference to the data container.
    // ------------------------------------------------------------------------
    inline const serialization::Container& container() const
    {
        return m_container;
    }

    // ------------------------------------------------------------------------
    //! \brief Get a pointer to the serialized data.
    //!
    //! @return Pointer to the data.
    // ------------------------------------------------------------------------
    inline const uint8_t* data() const
    {
        return m_container.data();
    }

    // ------------------------------------------------------------------------
    //! \brief Get the size of the serialized data.
    //!
    //! @return Size of the data in bytes.
    // ------------------------------------------------------------------------
    inline size_t size() const
    {
        return m_container.size();
    }

private:

    //! \brief Container storing the serialized data.
    serialization::Container m_container;
};

// ****************************************************************************
//! \brief Class facilitating the deserialization of trivially copyable data from
//! a byte container.
//!
//! By 'trivially copyable', we mean basic types (int, float), simple structures,
//! or arrays of structures.
//!
//! This class provides a simple interface for deserializing data.
//! It uses a byte container to store the serialized data.
//!
//! The deserialization is performed using the >> operator.
// ****************************************************************************
class Deserializer
{
public:
    // ------------------------------------------------------------------------
    //! \brief Constructor.
    //!
    //! @param[in] container Container in which the class will store the bytes.
    //! @note The container must not be destroyed while this class is using it.
    // ------------------------------------------------------------------------
    Deserializer(const serialization::Container& container)
        : m_container(container)
    {}

    // ------------------------------------------------------------------------
    //! \brief Deserialize data from the container.
    //!
    //! @tparam DataType Type of data to deserialize.
    //! @param[in] deserializer The deserializer instance.
    //! @param[out] data The deserialized data.
    //! @return Reference to the deserializer to chain operations.
    // ------------------------------------------------------------------------
    template <typename DataType>
    friend Deserializer& operator>>(Deserializer& deserializer, DataType& data)
    {
        static_assert(
            std::is_standard_layout<DataType>::value,
            "The data type is too complex to be deserialized directly");

        // Check that there is enough data to read
        if (deserializer.m_position + sizeof(DataType) > deserializer.m_container.size())
        {
            throw std::runtime_error("Deserialization: end of data reached");
        }

        // Copy the data from the buffer
        std::memcpy(&data, deserializer.m_container.data() + deserializer.m_position, sizeof(DataType));
        deserializer.m_position += sizeof(DataType);
        return deserializer;
    }

    // ------------------------------------------------------------------------
    //! \brief Special overload for strings.
    //!
    //! @param[in] deserializer The deserializer instance.
    //! @param[out] str The deserialized string.
    //! @return Reference to the deserializer to chain operations.
    // ------------------------------------------------------------------------
    friend Deserializer& operator>>(Deserializer& deserializer, std::string& str)
    {
        // First, get the size of the string
        uint32_t size;
        deserializer >> size;

        // Then, get the content of the string
        if (size > 0)
        {
            if (deserializer.m_position + size > deserializer.m_container.size())
            {
                throw std::runtime_error("Deserialization: end of data reached for the string");
            }

            str.resize(size);
            std::memcpy(&str[0], deserializer.m_container.data() + deserializer.m_position, size);
            deserializer.m_position += size;
        }
        else
        {
            str.clear();
        }
        return deserializer;
    }

    // ------------------------------------------------------------------------
    //! \brief Reset the reading position.
    // ------------------------------------------------------------------------
    inline void reset()
    {
        m_position = 0;
    }

    // ------------------------------------------------------------------------
    //! \brief Check if there is more data to read.
    //!
    //! @return true if there is more data, false otherwise.
    // ------------------------------------------------------------------------
    inline bool hasMoreData() const
    {
        return m_position < m_container.size();
    }

private:
    //! \brief Reference to the data container.
    const serialization::Container& m_container;

    //! \brief Current reading position in the container.
    size_t m_position = 0;
};

} // namespace bt