// ============================================================================
// Inventory.cpp
// Product array operations: add, update, delete, search and sort.
//
// Rule: products[0..count-1] is ALWAYS sorted by ID in ascending order.
// Every function that changes the array keeps this rule true, so binary
// search by ID always works.
// ============================================================================
#include "Inventory.h"
#include "Utils.h"
#include <iostream>
#include <iomanip>
using namespace std;

Inventory::Inventory() {
    count = 0;
}

int Inventory::getCount() const {
    return count;
}

bool Inventory::isEmpty() const {
    return count == 0;
}

bool Inventory::isFull() const {
    return count == MAX_PRODUCTS;
}

// Returns a copy of the product at the given index.
// An empty product (ID 0) is returned if the index is invalid.
Product Inventory::getProductAt(int index) const {
    if (index < 0 || index >= count) {
        Product empty = {0, "", "", 0.0, 0};
        return empty;
    }
    return products[index];
}

void Inventory::clear() {
    count = 0;
}

// ----------------------------------------------------------------------------
// Private helpers that keep the array sorted
// ----------------------------------------------------------------------------

// Finds the index where a product with this ID should be placed so that the
// array stays sorted. Moves forward while existing IDs are smaller.
int Inventory::findInsertPosition(int id) const {
    int position = 0;
    while (position < count && products[position].id < id) {
        position++;
    }
    return position;
}

// Inserts a product at 'position' by shifting later products one place right.
// Example: insert 102 into [101, 103, 104] -> [101, 102, 103, 104]
void Inventory::insertAt(int position, const Product& product) {
    for (int i = count; i > position; i--) {
        products[i] = products[i - 1];
    }
    products[position] = product;
    count++;
}

// Removes the product at 'position' by shifting later products one place left.
void Inventory::removeAt(int position) {
    for (int i = position; i < count - 1; i++) {
        products[i] = products[i + 1];
    }
    count--;
}

// ----------------------------------------------------------------------------
// Product management
// ----------------------------------------------------------------------------

// Checks that a product's values are acceptable:
// positive ID, non-empty name/category without '|', price > 0, stock >= 0.
bool Inventory::isValidProduct(const Product& product) const {
    if (product.id <= 0) return false;
    if (trim(product.name) == "" || trim(product.category) == "") return false;
    if (containsChar(product.name, '|') || containsChar(product.category, '|')) return false;
    if (product.price <= 0) return false;
    if (product.stock < 0) return false;
    return true;
}

// Registers a new product. Rejects invalid data, duplicate IDs and a full array.
int Inventory::addProduct(const Product& product) {
    if (!isValidProduct(product)) {
        return ERR_INVALID_DATA;
    }
    if (binarySearchById(product.id) != -1) {
        return ERR_DUPLICATE_ID;
    }
    if (isFull()) {
        return ERR_INVENTORY_FULL;
    }
    insertAt(findInsertPosition(product.id), product);
    return SUCCESS;
}

// Replaces the product that has 'oldId' with the 'updated' values.
// If the ID is changed, the record is removed and re-inserted at its new
// sorted position, so the array remains sorted by ID.
int Inventory::updateProduct(int oldId, const Product& updated) {
    int position = binarySearchById(oldId);
    if (position == -1) {
        return ERR_NOT_FOUND;
    }
    if (!isValidProduct(updated)) {
        return ERR_INVALID_DATA;
    }

    // Case 1: ID not changed -> overwrite in the same place
    if (updated.id == oldId) {
        products[position] = updated;
        return SUCCESS;
    }

    // Case 2: ID changed -> the new ID must not already exist
    if (binarySearchById(updated.id) != -1) {
        return ERR_DUPLICATE_ID;
    }
    removeAt(position);                                  // take out old record
    insertAt(findInsertPosition(updated.id), updated);   // put back in order
    return SUCCESS;
}

// Deletes the product with the given ID
int Inventory::deleteProduct(int id) {
    int position = binarySearchById(id);
    if (position == -1) {
        return ERR_NOT_FOUND;
    }
    removeAt(position);
    return SUCCESS;
}

// Decreases a product's stock (used only when an order is confirmed)
int Inventory::reduceStock(int id, int quantity) {
    int position = binarySearchById(id);
    if (position == -1) {
        return ERR_NOT_FOUND;
    }
    if (quantity <= 0) {
        return ERR_INVALID_QUANTITY;
    }
    if (quantity > products[position].stock) {
        return ERR_INSUFFICIENT_STOCK;
    }
    products[position].stock -= quantity;
    return SUCCESS;
}

// ----------------------------------------------------------------------------
// Searching
// ----------------------------------------------------------------------------

// BINARY SEARCH by ID - O(log n). Works because the array is sorted by ID.
// Looks at the middle element and discards the half that cannot contain the ID.
// Returns the index of the product, or -1 if not found.
int Inventory::binarySearchById(int id) const {
    int low = 0;
    int high = count - 1;

    while (low <= high) {
        int mid = low + (high - low) / 2;
        if (products[mid].id == id) {
            return mid;              // found
        } else if (products[mid].id < id) {
            low = mid + 1;           // search the right half
        } else {
            high = mid - 1;          // search the left half
        }
    }
    return -1;                       // not found
}

// LINEAR SEARCH by name - O(n). Checks every product; the name only needs to
// contain the key (e.g. "mouse" matches "Wireless Mouse"), capitals ignored.
int Inventory::searchByName(const string& key, Product results[]) const {
    int found = 0;
    for (int i = 0; i < count; i++) {
        if (containsIgnoreCase(products[i].name, key)) {
            results[found] = products[i];
            found++;
        }
    }
    return found;
}

// LINEAR SEARCH by category - O(n). Category must match exactly (capitals ignored).
int Inventory::searchByCategory(const string& category, Product results[]) const {
    int found = 0;
    string wanted = toLowerCase(trim(category));
    for (int i = 0; i < count; i++) {
        if (toLowerCase(trim(products[i].category)) == wanted) {
            results[found] = products[i];
            found++;
        }
    }
    return found;
}

// LINEAR SEARCH by price range - O(n). Finds products with
// minPrice <= price <= maxPrice. Returns -1 if the range itself is invalid.
int Inventory::searchByPriceRange(double minPrice, double maxPrice, Product results[]) const {
    if (minPrice < 0 || maxPrice < minPrice) {
        return -1;
    }
    int found = 0;
    for (int i = 0; i < count; i++) {
        if (products[i].price >= minPrice && products[i].price <= maxPrice) {
            results[found] = products[i];
            found++;
        }
    }
    return found;
}

// ----------------------------------------------------------------------------
// Sorting - always performed on a COPY so the main array stays sorted by ID
// ----------------------------------------------------------------------------

// Copies all products into another array and returns the count
int Inventory::copyProducts(Product copy[]) const {
    for (int i = 0; i < count; i++) {
        copy[i] = products[i];
    }
    return count;
}

// INSERTION SORT by price - O(n^2).
// Takes each product in turn ("key") and shifts larger (or smaller, for
// descending) products one place right until the key's correct place is found.
int Inventory::sortByPrice(Product sorted[], bool ascending) const {
    int n = copyProducts(sorted);
    for (int i = 1; i < n; i++) {
        Product key = sorted[i];
        int j = i - 1;
        while (j >= 0) {
            bool shouldShift;
            if (ascending) {
                shouldShift = sorted[j].price > key.price;
            } else {
                shouldShift = sorted[j].price < key.price;
            }
            if (!shouldShift) {
                break;
            }
            sorted[j + 1] = sorted[j];
            j--;
        }
        sorted[j + 1] = key;
    }
    return n;
}

// INSERTION SORT by name (A to Z, capitals ignored) - O(n^2)
int Inventory::sortByName(Product sorted[]) const {
    int n = copyProducts(sorted);
    for (int i = 1; i < n; i++) {
        Product key = sorted[i];
        string keyName = toLowerCase(key.name);
        int j = i - 1;
        while (j >= 0 && toLowerCase(sorted[j].name) > keyName) {
            sorted[j + 1] = sorted[j];
            j--;
        }
        sorted[j + 1] = key;
    }
    return n;
}

// INSERTION SORT by category - O(n^2). A to Z (ascending) or Z to A
// (descending), capitals ignored. The comparison is strict, so products in
// the same category keep their ID order (the sort is stable).
int Inventory::sortByCategory(Product sorted[], bool ascending) const {
    int n = copyProducts(sorted);
    for (int i = 1; i < n; i++) {
        Product key = sorted[i];
        string keyCategory = toLowerCase(key.category);
        int j = i - 1;
        while (j >= 0) {
            string current = toLowerCase(sorted[j].category);
            bool shouldShift;
            if (ascending) {
                shouldShift = current > keyCategory;
            } else {
                shouldShift = current < keyCategory;
            }
            if (!shouldShift) {
                break;
            }
            sorted[j + 1] = sorted[j];
            j--;
        }
        sorted[j + 1] = key;
    }
    return n;
}

// BUBBLE SORT by stock - O(n^2).
// Repeatedly compares neighbouring products and swaps them if they are in the
// wrong order. Stops early if a full pass makes no swaps (already sorted).
int Inventory::sortByStock(Product sorted[], bool ascending) const {
    int n = copyProducts(sorted);
    for (int pass = 0; pass < n - 1; pass++) {
        bool swapped = false;
        for (int i = 0; i < n - 1 - pass; i++) {
            bool wrongOrder;
            if (ascending) {
                wrongOrder = sorted[i].stock > sorted[i + 1].stock;
            } else {
                wrongOrder = sorted[i].stock < sorted[i + 1].stock;
            }
            if (wrongOrder) {
                Product temp = sorted[i];
                sorted[i] = sorted[i + 1];
                sorted[i + 1] = temp;
                swapped = true;
            }
        }
        if (!swapped) {
            break;
        }
    }
    return n;
}

// ----------------------------------------------------------------------------
// Display
// ----------------------------------------------------------------------------

void Inventory::displayAll() const {
    printProductTable(products, count);
}

// Prints a list of products as a neat table
void printProductTable(const Product list[], int size) {
    if (size == 0) {
        cout << "No products to display." << endl;
        return;
    }
    printLine();
    cout << left << setw(8) << "ID" << setw(26) << "Name" << setw(16) << "Category"
         << right << setw(12) << "Price (Rs)" << setw(10) << "Stock" << endl;
    printLine();
    for (int i = 0; i < size; i++) {
        cout << left << setw(8) << list[i].id << setw(26) << list[i].name
             << setw(16) << list[i].category
             << right << setw(12) << formatMoney(list[i].price)
             << setw(10) << list[i].stock << endl;
    }
    printLine();
}
