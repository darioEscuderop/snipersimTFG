#ifndef BLOOM_FILTER_H
#define BLOOM_FILTER_H

#include <bitset>
#include <functional>

class BloomFilter {
public:
// A mayor numero de hash menor probabilidad de falsos positivos
    BloomFilter(int size, int numHashes) : bitArray(size), numHashes(numHashes) {}

    // Inserta un elemento en el filtro de Bloom
    void insert(const std::string& key) {
        for (int i = 0; i < numHashes; i++) {
            // Calcula el hash para el elemento y el índice actual
            size_t hash = hashFunction(key, i);
            // Establece el bit correspondiente en el arreglo de bits
            bitArray.set(hash % bitArray.size());
        }
    }

    // Verifica si un elemento está presente en el filtro de Bloom
    bool contains(const std::string& key) const {
        for (int i = 0; i < numHashes; i++) {
            // Calcula el hash para el elemento y el índice actual
            size_t hash = hashFunction(key, i);
            // Verifica si el bit correspondiente en el arreglo de bits está establecido
            if (!bitArray.test(hash % bitArray.size())) {
                return false;
            }
        }
        return true;
    }

private:
    std::bitset<512> bitArray; // Arreglo de bits para almacenar los elementos
    int numHashes; // Número de funciones hash a utilizar

    // Función de hash para generar un valor hash único para cada elemento
    size_t hashFunction(const std::string& key, int seed) const {
        std::hash<std::string> hasher;
        return hasher(key + std::to_string(seed));
    }
};

#endif // BLOOM_FILTER_H