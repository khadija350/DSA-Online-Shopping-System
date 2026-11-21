// ============================================================================
// main.cpp
// Online Shopping and Order Management System - console menus.
//
// Data structures used:
//   Inventory  - array of Product structs, kept sorted by ID
//   Cart       - singly linked list
//   OrderQueue - linked-list queue (FIFO)
// All data is saved to the data/ folder immediately after every change.
// ============================================================================
#include <iostream>
#include <string>
#include "Product.h"
#include "Inventory.h"
#include "Cart.h"
#include "OrderQueue.h"
#include "FileManager.h"
#include "Utils.h"
using namespace std;

// File locations used by the application
const string DATA_FOLDER    = "data";
const string PRODUCTS_FILE  = "data/products.txt";
const string PENDING_FILE   = "data/pending_orders.txt";
const string COMPLETED_FILE = "data/completed_orders.txt";

// ----------------------------------------------------------------------------
// Small input helpers for product fields
// ----------------------------------------------------------------------------

// Asks until a positive product ID is entered
int readProductId(const string& prompt) {
    while (true) {
        int id = readInt(prompt);
        if (id > 0) {
            return id;
        }
        cout << "Product ID must be a positive number." << endl;
    }
}

// Asks until a price greater than 0 is entered (rounded to 2 decimals)
double readPrice(const string& prompt) {
    while (true) {
        double price = roundToCents(readDouble(prompt));
        if (price > 0) {
            return price;
        }
        cout << "Price must be greater than 0." << endl;
    }
}

// Asks until a stock value of 0 or more is entered
int readStock(const string& prompt) {
    while (true) {
        int stock = readInt(prompt);
        if (stock >= 0) {
            return stock;
        }
        cout << "Stock cannot be negative." << endl;
    }
}

// Saves the inventory and tells the user whether it worked
void saveInventory(FileManager& files, const Inventory& inventory) {
    if (files.saveProducts(PRODUCTS_FILE, inventory)) {
        cout << "Changes saved." << endl;
    } else {
        cout << "WARNING: Products could not be saved to " << PRODUCTS_FILE << "." << endl;
    }
}

// Shows a single product as a one-row table
void showProduct(const Product& product) {
    Product list[1] = {product};
    printProductTable(list, 1);
}

// ----------------------------------------------------------------------------
// Shared menus (used by Admin and Customer)
// ----------------------------------------------------------------------------

// Search products by ID, name, category or price range
void searchMenu(const Inventory& inventory) {
    cout << "\n--- Search Products ---" << endl;
    cout << "1. Search by ID (binary search)" << endl;
    cout << "2. Search by name (linear search)" << endl;
    cout << "3. Search by category (linear search)" << endl;
    cout << "4. Search by price range (linear search)" << endl;
    cout << "5. Back" << endl;
    int choice = readIntInRange("Enter choice: ", 1, 5);

    Product results[MAX_PRODUCTS];
    int found = 0;

    if (choice == 1) {
        int id = readInt("Enter product ID: ");
        int position = inventory.binarySearchById(id);
        if (position == -1) {
            cout << "No product found with ID " << id << "." << endl;
        } else {
            showProduct(inventory.getProductAt(position));
        }
        return;
    } else if (choice == 2) {
        string key = readText("Enter name (or part of name): ");
        found = inventory.searchByName(key, results);
    } else if (choice == 3) {
        string category = readText("Enter category: ");
        found = inventory.searchByCategory(category, results);
    } else if (choice == 4) {
        double minPrice = readDouble("Enter minimum price: ");
        double maxPrice = readDouble("Enter maximum price: ");
        found = inventory.searchByPriceRange(minPrice, maxPrice, results);
        if (found == -1) {
            cout << "Invalid range. Minimum must be 0 or more and not greater than maximum." << endl;
            return;
        }
    } else {
        return;
    }

    if (found == 0) {
        cout << "No matching products found." << endl;
    } else {
        cout << found << " product(s) found:" << endl;
        printProductTable(results, found);
    }
}

// ----------------------------------------------------------------------------
// Admin menu functions
// ----------------------------------------------------------------------------

void addProductMenu(Inventory& inventory, FileManager& files) {
    cout << "\n--- Add Product ---" << endl;
    if (inventory.isFull()) {
        cout << resultMessage(ERR_INVENTORY_FULL) << endl;
        return;
    }
    Product product;
    product.id = readProductId("Enter product ID: ");
    if (inventory.binarySearchById(product.id) != -1) {
        cout << resultMessage(ERR_DUPLICATE_ID) << endl;
        return;
    }
    product.name = readText("Enter product name: ");
    product.category = readText("Enter category: ");
    product.price = readPrice("Enter price (Rs): ");
    product.stock = readStock("Enter stock quantity: ");

    int result = inventory.addProduct(product);
    if (result == SUCCESS) {
        cout << "Product added successfully." << endl;
        saveInventory(files, inventory);
    } else {
        cout << resultMessage(result) << endl;
    }
}

void updateProductMenu(Inventory& inventory, FileManager& files) {
    cout << "\n--- Update Product ---" << endl;
    if (inventory.isEmpty()) {
        cout << "There are no products to update." << endl;
        return;
    }
    int oldId = readInt("Enter ID of the product to update: ");
    int position = inventory.binarySearchById(oldId);
    if (position == -1) {
        cout << resultMessage(ERR_NOT_FOUND) << endl;
        return;
    }

    Product updated = inventory.getProductAt(position);
    showProduct(updated);
    cout << "What do you want to change?" << endl;
    cout << "1. ID   2. Name   3. Category   4. Price   5. Stock   6. Cancel" << endl;
    int choice = readIntInRange("Enter choice: ", 1, 6);

    if (choice == 1) {
        updated.id = readProductId("Enter new product ID: ");
    } else if (choice == 2) {
        updated.name = readText("Enter new name: ");
    } else if (choice == 3) {
        updated.category = readText("Enter new category: ");
    } else if (choice == 4) {
        updated.price = readPrice("Enter new price (Rs): ");
    } else if (choice == 5) {
        updated.stock = readStock("Enter new stock quantity: ");
    } else {
        cout << "Update cancelled." << endl;
        return;
    }

    int result = inventory.updateProduct(oldId, updated);
    if (result == SUCCESS) {
        cout << "Product updated successfully." << endl;
        if (updated.id != oldId) {
            cout << "Product ID changed from " << oldId << " to " << updated.id
                 << ". The product list is still sorted by ID." << endl;
        }
        saveInventory(files, inventory);
    } else {
        cout << resultMessage(result) << endl;
    }
}

void deleteProductMenu(Inventory& inventory, FileManager& files) {
    cout << "\n--- Delete Product ---" << endl;
    if (inventory.isEmpty()) {
        cout << "There are no products to delete." << endl;
        return;
    }
    int id = readInt("Enter ID of the product to delete: ");
    int position = inventory.binarySearchById(id);
    if (position == -1) {
        cout << resultMessage(ERR_NOT_FOUND) << endl;
        return;
    }
    showProduct(inventory.getProductAt(position));
    if (!readYesNo("Are you sure you want to delete this product? (Y/N): ")) {
        cout << "Delete cancelled." << endl;
        return;
    }
    int result = inventory.deleteProduct(id);
    if (result == SUCCESS) {
        cout << "Product deleted." << endl;
        saveInventory(files, inventory);
    } else {
        cout << resultMessage(result) << endl;
    }
}

// Shows products sorted in different ways (sorting is done on a copy)
void sortMenu(const Inventory& inventory) {
    cout << "\n--- Sort and Display Products ---" << endl;
    cout << "1. Price: low to high (insertion sort)" << endl;
    cout << "2. Price: high to low (insertion sort)" << endl;
    cout << "3. Name: A to Z (insertion sort)" << endl;
    cout << "4. Stock: low to high (bubble sort)" << endl;
    cout << "5. Stock: high to low (bubble sort)" << endl;
    cout << "6. Back" << endl;
    int choice = readIntInRange("Enter choice: ", 1, 6);

    Product sorted[MAX_PRODUCTS];
    int n = 0;
    if (choice == 1) n = inventory.sortByPrice(sorted, true);
    else if (choice == 2) n = inventory.sortByPrice(sorted, false);
    else if (choice == 3) n = inventory.sortByName(sorted);
    else if (choice == 4) n = inventory.sortByStock(sorted, true);
    else if (choice == 5) n = inventory.sortByStock(sorted, false);
    else return;

    printProductTable(sorted, n);
    cout << "(Display only - the stored product list remains sorted by ID.)" << endl;
}

void viewPendingOrders(const OrderQueue& queue) {
    cout << "\n--- Pending Orders (FIFO: first order is processed next) ---" << endl;
    if (queue.isEmpty()) {
        cout << "No pending orders." << endl;
        return;
    }
    queue.displayAll();
}

void viewPendingTotal(const OrderQueue& queue) {
    cout << "\n--- Total Pending Order Amount ---" << endl;
    cout << "Pending orders: " << queue.getSize() << endl;
    cout << "Total amount:   Rs. " << formatMoney(queue.calculateTotalAmount()) << endl;
}

void adminMenu(Inventory& inventory, OrderQueue& queue, FileManager& files) {
    while (true) {
        cout << "\n========== ADMIN MENU ==========" << endl;
        cout << " 1. Add product" << endl;
        cout << " 2. Update product" << endl;
        cout << " 3. Delete product" << endl;
        cout << " 4. View all products" << endl;
        cout << " 5. Search products" << endl;
        cout << " 6. Sort and display products" << endl;
        cout << " 7. View pending orders" << endl;
        cout << " 8. View total pending order amount" << endl;
        cout << " 9. Process next order" << endl;
        cout << "10. View completed orders" << endl;
        cout << "11. Return to main menu" << endl;
        int choice = readIntInRange("Enter choice: ", 1, 11);

        switch (choice) {
            case 1:  addProductMenu(inventory, files); break;
            case 2:  updateProductMenu(inventory, files); break;
            case 3:  deleteProductMenu(inventory, files); break;
            case 4:
                cout << "\n--- All Products (sorted by ID) ---" << endl;
                inventory.displayAll();
                break;
            case 5:  searchMenu(inventory); break;
            case 6:  sortMenu(inventory); break;
            case 7:  viewPendingOrders(queue); break;
            case 8:  viewPendingTotal(queue); break;
            case 9:
                cout << "\n--- Process Next Order ---" << endl;
                files.processNextOrder(queue, PENDING_FILE, COMPLETED_FILE);
                break;
            case 10:
                cout << "\n--- Completed Orders ---" << endl;
                files.displayCompletedOrders(COMPLETED_FILE);
                break;
            case 11: return;
        }
    }
}

// ----------------------------------------------------------------------------
// Customer menu functions
// ----------------------------------------------------------------------------

void addToCartMenu(const Inventory& inventory, Cart& cart) {
    cout << "\n--- Add Product to Cart ---" << endl;
    if (inventory.isEmpty()) {
        cout << "No products are available in the shop." << endl;
        return;
    }
    int id = readInt("Enter product ID: ");
    int position = inventory.binarySearchById(id);
    if (position == -1) {
        cout << resultMessage(ERR_NOT_FOUND) << endl;
        return;
    }
    Product product = inventory.getProductAt(position);
    showProduct(product);
    if (product.stock == 0) {
        cout << resultMessage(ERR_OUT_OF_STOCK) << endl;
        return;
    }

    int quantity = readInt("Enter quantity: ");
    int result = cart.addFromInventory(inventory, id, quantity);
    if (result == SUCCESS) {
        cout << "Added " << quantity << " x " << product.name << " to your cart." << endl;
    } else {
        cout << resultMessage(result) << endl;
        if (result == ERR_INSUFFICIENT_STOCK) {
            int inCart = 0;
            CartNode* existing = cart.findItem(id);
            if (existing != nullptr) {
                inCart = existing->quantity;
            }
            cout << "Available: " << product.stock << "    Already in your cart: " << inCart << endl;
        }
    }
}

void updateCartMenu(const Inventory& inventory, Cart& cart) {
    cout << "\n--- Update Cart Quantity ---" << endl;
    if (cart.isEmpty()) {
        cout << "Your cart is empty." << endl;
        return;
    }
    cart.display();
    int id = readInt("Enter product ID to update: ");
    if (cart.findItem(id) == nullptr) {
        cout << "This product is not in your cart." << endl;
        return;
    }
    int quantity = readInt("Enter new quantity: ");
    int result = cart.changeQuantity(inventory, id, quantity);
    if (result == SUCCESS) {
        cout << "Quantity updated." << endl;
    } else if (result == ERR_NOT_FOUND) {
        cout << "This product is no longer sold in the shop. Please remove it from your cart." << endl;
    } else {
        cout << resultMessage(result) << endl;
        if (result == ERR_INSUFFICIENT_STOCK) {
            Product product = inventory.getProductAt(inventory.binarySearchById(id));
            cout << "Only " << product.stock << " available." << endl;
        }
    }
}

void removeCartMenu(Cart& cart) {
    cout << "\n--- Remove Cart Item ---" << endl;
    if (cart.isEmpty()) {
        cout << "Your cart is empty." << endl;
        return;
    }
    cart.display();
    int id = readInt("Enter product ID to remove: ");
    if (cart.removeItem(id)) {
        cout << "Item removed from cart." << endl;
    } else {
        cout << "This product is not in your cart." << endl;
    }
}

void checkoutMenu(Cart& cart, Inventory& inventory, OrderQueue& queue,
                  FileManager& files, int& nextOrderId) {
    int result = runCheckout(cart, inventory, queue, nextOrderId);
    if (result == SUCCESS) {
        nextOrderId++;
        // Save the reduced stock and the new pending order immediately
        bool productsSaved = files.saveProducts(PRODUCTS_FILE, inventory);
        bool ordersSaved = files.savePendingOrders(PENDING_FILE, queue);
        if (productsSaved && ordersSaved) {
            cout << "Order and updated inventory saved." << endl;
        } else {
            cout << "WARNING: The order was placed but could not be fully saved to file." << endl;
        }
    }
}

void customerMenu(Inventory& inventory, Cart& cart, OrderQueue& queue,
                  FileManager& files, int& nextOrderId) {
    while (true) {
        cout << "\n========== CUSTOMER MENU ==========" << endl;
        cout << "1. View products" << endl;
        cout << "2. Search products" << endl;
        cout << "3. Add product to cart" << endl;
        cout << "4. View cart" << endl;
        cout << "5. Update cart quantity" << endl;
        cout << "6. Remove cart item" << endl;
        cout << "7. Checkout" << endl;
        cout << "8. Return to main menu" << endl;
        int choice = readIntInRange("Enter choice: ", 1, 8);

        switch (choice) {
            case 1:
                cout << "\n--- Products ---" << endl;
                inventory.displayAll();
                break;
            case 2: searchMenu(inventory); break;
            case 3: addToCartMenu(inventory, cart); break;
            case 4:
                cout << "\n--- Your Cart ---" << endl;
                cart.display();
                break;
            case 5: updateCartMenu(inventory, cart); break;
            case 6: removeCartMenu(cart); break;
            case 7: checkoutMenu(cart, inventory, queue, files, nextOrderId); break;
            case 8: return;
        }
    }
}

// ----------------------------------------------------------------------------
// Program start
// ----------------------------------------------------------------------------
int main() {
    Inventory inventory;
    Cart cart;
    OrderQueue orderQueue;
    FileManager files;

    cout << "=================================================" << endl;
    cout << "   ONLINE SHOPPING AND ORDER MANAGEMENT SYSTEM" << endl;
    cout << "=================================================" << endl;

    // Create the data folder/files if needed, then load saved data
    if (!files.setupStorage(DATA_FOLDER, PRODUCTS_FILE, PENDING_FILE, COMPLETED_FILE)) {
        cout << "WARNING: Data files could not be created. Changes may not be saved." << endl;
    }
    files.loadProducts(PRODUCTS_FILE, inventory);
    files.loadPendingOrders(PENDING_FILE, COMPLETED_FILE, orderQueue);
    int nextOrderId = files.getNextOrderId(PENDING_FILE, COMPLETED_FILE);

    cout << "Loaded " << inventory.getCount() << " product(s) and "
         << orderQueue.getSize() << " pending order(s)." << endl;

    while (true) {
        cout << "\n========== MAIN MENU ==========" << endl;
        cout << "1. Admin" << endl;
        cout << "2. Customer" << endl;
        cout << "3. Exit" << endl;
        int choice = readIntInRange("Enter choice: ", 1, 3);

        if (choice == 1) {
            adminMenu(inventory, orderQueue, files);
        } else if (choice == 2) {
            customerMenu(inventory, cart, orderQueue, files, nextOrderId);
        } else {
            cout << "All data has been saved. Goodbye!" << endl;
            break;
        }
    }
    return 0;   // destructors of cart and orderQueue free all remaining nodes
}
