// ============================================================================
// Inventory.h
// Stores all products in a fixed-size array that is ALWAYS kept sorted by
// product ID (ascending). Keeping it sorted allows binary search by ID.
// ============================================================================
#ifndef INVENTORY_H
#define INVENTORY_H

#include "Product.h"

class Inventory {
private:
    Product products[MAX_PRODUCTS]; // the product array (sorted by ID)
    int count;                      // how many products are stored

    // Helpers that keep the array sorted
    int findInsertPosition(int id) const;              // where a new ID belongs
    void insertAt(int position, const Product& product); // shift right, insert
    void removeAt(int position);                       // shift left, remove

public:
    Inventory();

    // ----- basic information -----
    int getCount() const;
    bool isEmpty() const;
    bool isFull() const;
    Product getProductAt(int index) const;  // product at a given array index
    void clear();                           // removes all products

    // ----- product management (return a result code from Product.h) -----
    bool isValidProduct(const Product& product) const;
    int addProduct(const Product& product);
    int updateProduct(int oldId, const Product& updated);
    int deleteProduct(int id);
    int reduceStock(int id, int quantity);

    // ----- searching -----
    int binarySearchById(int id) const;  // returns array index, or -1
    // The following fill 'results' and return how many products matched
    int searchByName(const std::string& key, Product results[]) const;
    int searchByCategory(const std::string& category, Product results[]) const;
    int searchByPriceRange(double minPrice, double maxPrice, Product results[]) const; // -1 = invalid range

    // ----- sorting (on a temporary copy; main array stays sorted by ID) -----
    int copyProducts(Product copy[]) const;
    int sortByPrice(Product sorted[], bool ascending) const; // insertion sort
    int sortByName(Product sorted[]) const;                  // insertion sort
    int sortByStock(Product sorted[], bool ascending) const; // bubble sort

    // ----- display -----
    void displayAll() const;
};

// Prints any list of products as a table
void printProductTable(const Product list[], int size);

#endif
