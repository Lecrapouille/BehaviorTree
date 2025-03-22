/**
 * @file TestSerialization.cpp
 * @brief Tests unitaires pour les fonctionnalités de sérialisation.
 * @author Claude
 */

#include "main.hpp"
#include "BehaviorTree/Serialization.hpp"
#include <iostream>
#include <cassert>
#include <array>

namespace bt {
namespace test {

/**
 * @brief Tests des fonctionnalités de sérialisation et désérialisation
 */
class TestSerialization : public TestCase
{
public:
    /**
     * @brief Test de sérialisation et désérialisation des types de base
     */
    void testBasicTypes()
    {
        // Sérialisation de types de base
        Serializer serializer;
        int32_t valueInt = 42;
        float valueFloat = 3.14159f;
        bool valueBool = true;
        
        serializer << valueInt << valueFloat << valueBool;
        
        // Désérialisation des données
        Deserializer deserializer(serializer.data());
        
        bool readBool;
        float readFloat;
        int32_t readInt;
        
        deserializer >> readBool >> readFloat >> readInt;
        
        // Vérifications
        ASSERT_EQUAL(readInt, valueInt);
        ASSERT_EQUAL(readFloat, valueFloat);
        ASSERT_EQUAL(readBool, valueBool);
        ASSERT_FALSE(deserializer.hasMoreData());
    }
    
    /**
     * @brief Test de sérialisation et désérialisation des chaînes
     */
    void testStrings()
    {
        Serializer serializer;
        std::string message1 = "Hello, World!";
        std::string message2 = "Sérialisation C++";
        std::string emptyStr = "";
        
        serializer << message1 << message2 << emptyStr;
        
        Deserializer deserializer(serializer.data());
        std::string readEmpty;
        std::string readMsg2;
        std::string readMsg1;
        
        deserializer >> readEmpty >> readMsg2 >> readMsg1;
        
        ASSERT_EQUAL(readMsg1, message1);
        ASSERT_EQUAL(readMsg2, message2);
        ASSERT_EQUAL(readEmpty, emptyStr);
        ASSERT_FALSE(deserializer.hasMoreData());
    }
    
    /**
     * @brief Test de sérialisation et désérialisation des structures
     */
    void testStructs()
    {
        struct Vector3
        {
            float x, y, z;
            bool operator==(const Vector3& other) const
            {
                return x == other.x && y == other.y && z == other.z;
            }
        };
        
        Vector3 position{1.0f, 2.0f, 3.0f};
        Vector3 velocity{0.1f, 0.2f, 0.3f};
        
        Serializer serializer;
        serializer << position << velocity;
        
        Deserializer deserializer(serializer.data());
        Vector3 readVelocity;
        Vector3 readPosition;
        
        deserializer >> readVelocity >> readPosition;
        
        ASSERT_TRUE(readPosition == position);
        ASSERT_TRUE(readVelocity == velocity);
        ASSERT_FALSE(deserializer.hasMoreData());
    }
    
    /**
     * @brief Test de sérialisation et désérialisation des tableaux
     */
    void testArrays()
    {
        std::array<int, 5> numbers = {10, 20, 30, 40, 50};
        
        Serializer serializer;
        uint32_t size = static_cast<uint32_t>(numbers.size());
        serializer << size;
        for (const auto& num : numbers)
        {
            serializer << num;
        }
        
        Deserializer deserializer(serializer.data());
        uint32_t readSize;
        deserializer >> readSize;
        
        ASSERT_EQUAL(readSize, numbers.size());
        
        std::vector<int> readNumbers;
        readNumbers.reserve(readSize);
        
        for (uint32_t i = 0; i < readSize; ++i)
        {
            int value;
            deserializer >> value;
            readNumbers.push_back(value);
            ASSERT_EQUAL(value, numbers[i]);
        }
        
        ASSERT_FALSE(deserializer.hasMoreData());
    }
    
    /**
     * @brief Exécute tous les tests de sérialisation
     */
    void run() override
    {
        RUN_TEST(testBasicTypes);
        RUN_TEST(testStrings);
        RUN_TEST(testStructs);
        RUN_TEST(testArrays);
    }
};

// Enregistrement du test
REGISTER_TEST(TestSerialization);

} // namespace test
} // namespace bt 