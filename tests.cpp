// ============================================================================
// tests.cpp
// Automated tests for the Online Shopping and Order Management System.
//
// Every test calls the REAL functions, then compares the expected result with
// the actual result and prints PASS or FAIL.
//
// The tests use ONLY the test_data/ folder. The application's data/ folder is
// never opened for writing; at the end we check that it is unchanged.
//
// For the interactive checkout, the customer's answers (Y/N, name) are
// supplied from a string instead of the keyboard, and the printed output is
// captured so it can be checked.
// ============================================================================
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <filesystem>
#include "Product.h"
#include "Inventory.h"
#include "Cart.h"
#include "OrderQueue.h"
#include "FileManager.h"
#include "Utils.h"
using namespace std;
namespace fs = std::filesystem;

// Test files (test_data/ only)
const string TEST_FOLDER    = "test_data";
const string TEST_PRODUCTS  = "test_data/products.txt";
const string TEST_PENDING   = "test_data/pending_orders.txt";
const string TEST_COMPLETED = "test_data/completed_orders.txt";

// Application files - only READ, to prove they are not modified
const string APP_FOLDER    = "data";
const string APP_PRODUCTS  = "data/products.txt";
const string APP_PENDING   = "data/pending_orders.txt";
const string APP_COMPLETED = "data/completed_orders.txt";

int totalTests = 0;
int passedTests = 0;
int failedTests = 0;

// ----------------------------------------------------------------------------
// Test helpers
// ----------------------------------------------------------------------------

// Compares expected and actual values and prints the result of one test
void check(const string& description, const string& expected, const string& actual) {
    totalTests++;
    bool pass = (expected == actual);
    if (pass) {
        passedTests++;
    } else {
        failedTests++;
    }
    cout << "Test " << totalTests << ": " << description << endl;
    cout << "    Expected: " << expected << endl;
    cout << "    Actual  : " << actual << endl;
    cout << "    Result  : " << (pass ? "PASS" : "FAIL") << endl;
}

void section(const string& title) {
    cout << "\n==================== " << title << " ====================" << endl;
}

string boolText(bool value) {
    return value ? "true" : "false";
}

// Builds a Product quickly
Product makeProduct(int id, const string& name, const string& category, double price, int stock) {
    Product p;
    p.id = id;
    p.name = name;
    p.category = category;
    p.price = price;
    p.stock = stock;
    return p;
}

// "101,102,103" from a product list
string idList(const Product list[], int size) {
    string result = "";
    for (int i = 0; i < size; i++) {
        if (i > 0) result += ",";
        result += to_string(list[i].id);
    }
    return result;
}

// IDs of the main inventory array in stored order
string inventoryIds(const Inventory& inventory) {
    Product copy[MAX_PRODUCTS];
    int n = inventory.copyProducts(copy);
    return idList(copy, n);
}

// Order IDs of a queue from front to rear
string queueIds(const OrderQueue& queue) {
    string result = "";
    Order* current = queue.peek();
    while (current != nullptr) {
        if (result != "") result += ",";
        result += to_string(current->orderId);
        current = current->next;
    }
    return result;
}

// Stock of a product (or -1 if not found)
int stockOf(const Inventory& inventory, int id) {
    int position = inventory.binarySearchById(id);
    if (position == -1) return -1;
    return inventory.getProductAt(position).stock;
}

// Five sample products (added in mixed ID order on purpose)
void loadSampleProducts(Inventory& inventory) {
    inventory.addProduct(makeProduct(104, "Wireless Mouse", "Electronics", 1500.00, 25));
    inventory.addProduct(makeProduct(101, "USB Keyboard", "Electronics", 2500.00, 10));
    inventory.addProduct(makeProduct(103, "Notebook", "Stationery", 150.00, 100));
    inventory.addProduct(makeProduct(102, "Gaming Mouse", "Electronics", 4500.00, 5));
    inventory.addProduct(makeProduct(105, "Ball Pen", "Stationery", 50.00, 0));
}

// Whole file content, or "<missing>" if it does not exist
string readWholeFile(const string& path) {
    ifstream in(path, ios::binary);
    if (!in) return "<missing>";
    stringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

// Counts how many times 'pattern' appears in 'text'
int countOccurrences(const string& text, const string& pattern) {
    int count = 0;
    size_t position = text.find(pattern);
    while (position != string::npos) {
        count++;
        position = text.find(pattern, position + pattern.length());
    }
    return count;
}

bool containsText(const string& text, const string& pattern) {
    return text.find(pattern) != string::npos;
}

// ---- Output capture: hides messages printed by the functions under test ----
stringstream capturedOutput;
streambuf* originalCout = nullptr;

void startCapture() {
    capturedOutput.str("");
    capturedOutput.clear();
    originalCout = cout.rdbuf(capturedOutput.rdbuf());
}

string stopCapture() {
    cout.rdbuf(originalCout);
    return capturedOutput.str();
}

// ---- Scripted input: supplies the customer's answers to runCheckout ----
stringstream scriptedInput;
streambuf* originalCin = nullptr;

// Runs the real interactive checkout with prepared answers.
// 'output' receives everything the checkout printed.
int runScriptedCheckout(Cart& cart, Inventory& inventory, OrderQueue& queue,
                        int orderId, const string& answers, string& output) {
    scriptedInput.str(answers);
    scriptedInput.clear();
    originalCin = cin.rdbuf(scriptedInput.rdbuf());
    startCapture();
    int result = runCheckout(cart, inventory, queue, orderId);
    output = stopCapture();
    cin.rdbuf(originalCin);
    return result;
}

// ----------------------------------------------------------------------------
// INVENTORY TESTS
// ----------------------------------------------------------------------------
void testInventory() {
    section("INVENTORY");
    Inventory inventory;

    check("Add valid product (ID 104)", resultMessage(SUCCESS),
          resultMessage(inventory.addProduct(makeProduct(104, "Wireless Mouse", "Electronics", 1500, 25))));
    check("Product count after first add", "1", to_string(inventory.getCount()));

    inventory.addProduct(makeProduct(101, "USB Keyboard", "Electronics", 2500, 10));
    inventory.addProduct(makeProduct(103, "Notebook", "Stationery", 150, 100));
    inventory.addProduct(makeProduct(102, "Gaming Mouse", "Electronics", 4500, 5));
    check("Products added in mixed order are stored sorted by ID", "101,102,103,104",
          inventoryIds(inventory));

    check("Reject duplicate product ID 102", resultMessage(ERR_DUPLICATE_ID),
          resultMessage(inventory.addProduct(makeProduct(102, "Copy", "Test", 10, 1))));
    check("Count unchanged after duplicate rejected", "4", to_string(inventory.getCount()));

    check("Reject negative price", resultMessage(ERR_INVALID_DATA),
          resultMessage(inventory.addProduct(makeProduct(200, "Bad", "Test", -5, 1))));
    check("Reject zero price", resultMessage(ERR_INVALID_DATA),
          resultMessage(inventory.addProduct(makeProduct(201, "Bad", "Test", 0, 1))));
    check("Reject negative stock", resultMessage(ERR_INVALID_DATA),
          resultMessage(inventory.addProduct(makeProduct(202, "Bad", "Test", 10, -1))));
    check("Reject empty product name", resultMessage(ERR_INVALID_DATA),
          resultMessage(inventory.addProduct(makeProduct(203, "  ", "Test", 10, 1))));
    check("Reject product ID 0", resultMessage(ERR_INVALID_DATA),
          resultMessage(inventory.addProduct(makeProduct(0, "Zero", "Test", 10, 1))));
    check("Reject name containing '|'", resultMessage(ERR_INVALID_DATA),
          resultMessage(inventory.addProduct(makeProduct(204, "A|B", "Test", 10, 1))));

    // Full array (boundary)
    Inventory full;
    for (int i = 1; i <= MAX_PRODUCTS; i++) {
        full.addProduct(makeProduct(i, "Item " + to_string(i), "General", 10, 1));
    }
    check("Fill inventory to maximum (100 products)", "100", to_string(full.getCount()));
    check("Reject 101st product when array is full", resultMessage(ERR_INVENTORY_FULL),
          resultMessage(full.addProduct(makeProduct(500, "Extra", "General", 10, 1))));
    check("Binary search finds last product in full array (ID 100)", "99",
          to_string(full.binarySearchById(100)));

    // Update (same ID)
    Product changed = makeProduct(104, "Wireless Mouse", "Electronics", 1600, 20);
    check("Update existing product 104 (price and stock)", resultMessage(SUCCESS),
          resultMessage(inventory.updateProduct(104, changed)));
    Product after = inventory.getProductAt(inventory.binarySearchById(104));
    check("Updated values stored", "1600.00/20", formatMoney(after.price) + "/" + to_string(after.stock));
    check("Update missing product 999", resultMessage(ERR_NOT_FOUND),
          resultMessage(inventory.updateProduct(999, makeProduct(999, "X", "Y", 10, 1))));
    check("Reject update with negative price", resultMessage(ERR_INVALID_DATA),
          resultMessage(inventory.updateProduct(104, makeProduct(104, "Wireless Mouse", "Electronics", -1, 20))));

    // Change ID
    Product moved = inventory.getProductAt(inventory.binarySearchById(101));
    moved.id = 110;
    check("Change product ID 101 -> 110", resultMessage(SUCCESS),
          resultMessage(inventory.updateProduct(101, moved)));
    check("Array still sorted by ID after ID change", "102,103,104,110", inventoryIds(inventory));
    check("Binary search finds new ID 110 at index 3", "3", to_string(inventory.binarySearchById(110)));
    check("Binary search no longer finds old ID 101", "-1", to_string(inventory.binarySearchById(101)));

    Product clash = inventory.getProductAt(inventory.binarySearchById(102));
    clash.id = 103;
    check("Reject ID change 102 -> 103 (duplicate)", resultMessage(ERR_DUPLICATE_ID),
          resultMessage(inventory.updateProduct(102, clash)));
    check("Array unchanged after rejected ID change", "102,103,104,110", inventoryIds(inventory));

    // Delete
    check("Delete existing product 103", resultMessage(SUCCESS),
          resultMessage(inventory.deleteProduct(103)));
    check("Array after delete", "102,104,110", inventoryIds(inventory));
    check("Delete missing product 103 again", resultMessage(ERR_NOT_FOUND),
          resultMessage(inventory.deleteProduct(103)));
    Inventory empty;
    check("Delete from empty inventory", resultMessage(ERR_NOT_FOUND),
          resultMessage(empty.deleteProduct(1)));

    // Searching
    Inventory sample;
    loadSampleProducts(sample);
    Product results[MAX_PRODUCTS];
    check("Binary search ID 101 (first)", "0", to_string(sample.binarySearchById(101)));
    check("Binary search ID 103 (middle)", "2", to_string(sample.binarySearchById(103)));
    check("Binary search ID 105 (last)", "4", to_string(sample.binarySearchById(105)));
    check("Binary search missing ID 999", "-1", to_string(sample.binarySearchById(999)));
    check("Binary search in empty inventory", "-1", to_string(empty.binarySearchById(101)));

    int n = sample.searchByName("mouse", results);
    check("Name search 'mouse' (partial match)", "102,104", idList(results, n));
    n = sample.searchByName("MOUSE", results);
    check("Name search 'MOUSE' (capitals ignored)", "102,104", idList(results, n));
    check("Name search with no match", "0", to_string(sample.searchByName("laptop", results)));

    n = sample.searchByCategory("electronics", results);
    check("Category search 'electronics'", "101,102,104", idList(results, n));
    check("Category search with no match", "0", to_string(sample.searchByCategory("Toys", results)));

    n = sample.searchByPriceRange(150, 2500, results);
    check("Price range 150-2500 (boundaries included)", "101,103,104", idList(results, n));
    check("Price range with no products (5000-6000)", "0",
          to_string(sample.searchByPriceRange(5000, 6000, results)));
    check("Invalid price range (min > max)", "-1",
          to_string(sample.searchByPriceRange(3000, 1000, results)));
}

// ----------------------------------------------------------------------------
// SORTING TESTS
// ----------------------------------------------------------------------------
void testSorting() {
    section("SORTING");
    Inventory sample;
    loadSampleProducts(sample);
    Product sorted[MAX_PRODUCTS];
    int n;

    n = sample.sortByPrice(sorted, true);
    check("Insertion sort by price ascending", "105,103,104,101,102", idList(sorted, n));
    n = sample.sortByPrice(sorted, false);
    check("Insertion sort by price descending", "102,101,104,103,105", idList(sorted, n));
    n = sample.sortByName(sorted);
    check("Insertion sort by name A-Z", "105,102,103,101,104", idList(sorted, n));
    n = sample.sortByStock(sorted, true);
    check("Bubble sort by stock ascending", "105,102,101,104,103", idList(sorted, n));
    n = sample.sortByStock(sorted, false);
    check("Bubble sort by stock descending", "103,104,101,102,105", idList(sorted, n));
    check("Main array still sorted by ID after all sorts", "101,102,103,104,105", inventoryIds(sample));

    Inventory empty;
    check("Sort empty inventory returns 0 products", "0", to_string(empty.sortByPrice(sorted, true)));
    Inventory single;
    single.addProduct(makeProduct(7, "Only Item", "General", 99, 3));
    n = single.sortByName(sorted);
    check("Sort single-item inventory", "7", idList(sorted, n));

    // ----- Category sorting (insertion sort on a copy) -----
    // Sample categories: 101 Electronics, 102 Electronics, 103 Stationery,
    //                    104 Electronics, 105 Stationery
    n = sample.sortByCategory(sorted, true);
    check("Insertion sort by category A-Z", "101,102,104,103,105", idList(sorted, n));
    n = sample.sortByCategory(sorted, false);
    check("Insertion sort by category Z-A", "103,105,101,102,104", idList(sorted, n));

    // Duplicate categories written with different capitals
    Inventory mixed;
    mixed.addProduct(makeProduct(1, "Robot", "Toys", 900, 2));
    mixed.addProduct(makeProduct(2, "Novel", "books", 700, 4));
    mixed.addProduct(makeProduct(3, "Charger", "Electronics", 1200, 6));
    mixed.addProduct(makeProduct(4, "Atlas", "Books", 1500, 1));
    mixed.addProduct(makeProduct(5, "Puzzle", "toys", 400, 9));
    n = mixed.sortByCategory(sorted, true);
    check("Category A-Z: duplicates keep ID order, capitals ignored", "2,4,3,1,5", idList(sorted, n));
    n = mixed.sortByCategory(sorted, false);
    check("Category Z-A: duplicates keep ID order, capitals ignored", "1,5,3,2,4", idList(sorted, n));

    check("Main array still sorted by ID after category sorts", "101,102,103,104,105", inventoryIds(sample));
    check("Category sort of empty inventory (A-Z / Z-A)", "0/0",
          to_string(empty.sortByCategory(sorted, true)) + "/" + to_string(empty.sortByCategory(sorted, false)));
    string singleResult = idList(sorted, single.sortByCategory(sorted, true));
    singleResult += "/" + idList(sorted, single.sortByCategory(sorted, false));
    check("Category sort of single-item inventory (A-Z / Z-A)", "7/7", singleResult);
}

// ----------------------------------------------------------------------------
// CART TESTS
// ----------------------------------------------------------------------------
void testCart() {
    section("SHOPPING CART");
    Inventory inventory;
    loadSampleProducts(inventory);
    Cart cart;

    check("New cart is empty", "true", boolText(cart.isEmpty()));
    check("Empty cart total", "0.00", formatMoney(cart.calculateTotal()));

    check("Add 2 x product 104", resultMessage(SUCCESS),
          resultMessage(cart.addFromInventory(inventory, 104, 2)));
    check("Cart has 1 item", "1", to_string(cart.getItemCount()));
    check("Add same product 104 again (1 more)", resultMessage(SUCCESS),
          resultMessage(cart.addFromInventory(inventory, 104, 1)));
    check("No duplicate node: still 1 item, quantity 3", "1/3",
          to_string(cart.getItemCount()) + "/" + to_string(cart.findItem(104)->quantity));

    cart.addFromInventory(inventory, 101, 1);
    check("Cart total = 3 x 1500 + 1 x 2500", "7000.00", formatMoney(cart.calculateTotal()));

    check("Reject quantity 0", resultMessage(ERR_INVALID_QUANTITY),
          resultMessage(cart.addFromInventory(inventory, 103, 0)));
    check("Reject negative quantity", resultMessage(ERR_INVALID_QUANTITY),
          resultMessage(cart.addFromInventory(inventory, 103, -2)));
    check("Reject product that does not exist", resultMessage(ERR_NOT_FOUND),
          resultMessage(cart.addFromInventory(inventory, 999, 1)));
    check("Reject unavailable product (stock 0)", resultMessage(ERR_OUT_OF_STOCK),
          resultMessage(cart.addFromInventory(inventory, 105, 1)));
    check("Reject quantity above stock (6 > 5)", resultMessage(ERR_INSUFFICIENT_STOCK),
          resultMessage(cart.addFromInventory(inventory, 102, 6)));
    check("Reject when cart qty + new qty exceeds stock (3 + 23 > 25)", resultMessage(ERR_INSUFFICIENT_STOCK),
          resultMessage(cart.addFromInventory(inventory, 104, 23)));
    check("Stock not reduced by adding to cart", "25", to_string(stockOf(inventory, 104)));

    check("Update cart quantity of 104 to 5", resultMessage(SUCCESS),
          resultMessage(cart.changeQuantity(inventory, 104, 5)));
    check("Reject update above stock (30)", resultMessage(ERR_INSUFFICIENT_STOCK),
          resultMessage(cart.changeQuantity(inventory, 104, 30)));
    check("Reject update to quantity 0", resultMessage(ERR_INVALID_QUANTITY),
          resultMessage(cart.changeQuantity(inventory, 104, 0)));
    check("Quantity still 5 after rejected updates", "5", to_string(cart.findItem(104)->quantity));
    check("Update quantity of item not in cart", resultMessage(ERR_NOT_FOUND),
          resultMessage(cart.changeQuantity(inventory, 103, 2)));

    check("Remove item 101", "true", boolText(cart.removeItem(101)));
    check("Cart has 1 item after removal", "1", to_string(cart.getItemCount()));
    check("Remove item not in cart (999)", "false", boolText(cart.removeItem(999)));

    cart.clear();
    check("Cart empty after clear", "true", boolText(cart.isEmpty()));
    check("Remove from empty cart", "false", boolText(cart.removeItem(104)));
    check("Total of cleared cart", "0.00", formatMoney(cart.calculateTotal()));
}

// ----------------------------------------------------------------------------
// ORDER QUEUE TESTS
// ----------------------------------------------------------------------------
void testOrders() {
    section("ORDER QUEUE");
    OrderQueue queue;
    check("New queue is empty", "true", boolText(queue.isEmpty()));

    Order* first = createOrder(1001, "Ali Khan");
    addItemToOrder(first, 104, "Wireless Mouse", 1500, 2);
    addItemToOrder(first, 300, "Bag", 500, 1);
    check("Order total = 2 x 1500 + 1 x 500", "3500.00", formatMoney(first->totalAmount));
    check("Order item count", "2", to_string(first->itemCount));

    Order* second = createOrder(1002, "Sara Ahmed");
    addItemToOrder(second, 101, "USB Keyboard", 2500, 1);
    Order* third = createOrder(1003, "Usman Ali");
    addItemToOrder(third, 103, "Notebook", 150, 3);

    queue.enqueue(first);
    queue.enqueue(second);
    queue.enqueue(third);
    check("Enqueue 3 orders: size", "3", to_string(queue.getSize()));
    check("Peek shows first order placed", "1001", to_string(queue.peek()->orderId));
    check("Queue order front to rear", "1001,1002,1003", queueIds(queue));
    check("Total pending amount = 3500 + 2500 + 450", "6450.00",
          formatMoney(queue.calculateTotalAmount()));

    string processedOrder = "";
    for (int i = 0; i < 3; i++) {
        Order* removed = queue.dequeue();
        if (i > 0) processedOrder += ",";
        processedOrder += to_string(removed->orderId);
        freeOrder(removed);
    }
    check("Dequeue follows FIFO order", "1001,1002,1003", processedOrder);

    check("Queue empty after all dequeued", "true", boolText(queue.isEmpty()));
    check("Dequeue from empty queue returns nothing", "null",
          queue.dequeue() == nullptr ? "null" : "order");
    check("Peek on empty queue returns nothing", "null",
          queue.peek() == nullptr ? "null" : "order");
    check("Total of empty queue", "0.00", formatMoney(queue.calculateTotalAmount()));

    queue.enqueue(createOrder(1004, "New Customer"));
    check("Enqueue works again after queue became empty", "1/1004",
          to_string(queue.getSize()) + "/" + to_string(queue.peek()->orderId));
    queue.clear();
    check("Clear empties the queue", "true/0", boolText(queue.isEmpty()) + "/" + to_string(queue.getSize()));
}

// ----------------------------------------------------------------------------
// CHECKOUT TESTS
// ----------------------------------------------------------------------------
void testCheckout() {
    section("CHECKOUT");
    string output;

    // 1. Empty cart
    {
        Inventory inventory;
        loadSampleProducts(inventory);
        Cart cart;
        OrderQueue queue;
        check("Checkout with empty cart is refused", resultMessage(ERR_EMPTY_CART),
              resultMessage(runScriptedCheckout(cart, inventory, queue, 1001, "", output)));
        check("placeOrder with empty cart is refused", resultMessage(ERR_EMPTY_CART),
              resultMessage(placeOrder(cart, inventory, queue, "Ali", 1001)));
        check("No order created for empty cart", "0", to_string(queue.getSize()));
    }

    // 2. Deleted product - customer refuses to remove it
    {
        Inventory inventory;
        loadSampleProducts(inventory);
        Cart cart;
        OrderQueue queue;
        cart.addFromInventory(inventory, 104, 2);
        inventory.deleteProduct(104);       // admin deletes the product
        int result = runScriptedCheckout(cart, inventory, queue, 1001, "N\n", output);
        check("Deleted product: checkout blocked if not removed", resultMessage(ERR_CART_NEEDS_REVIEW),
              resultMessage(result));
        check("Deleted product: customer clearly informed", "true",
              boolText(containsText(output, "no longer available")));
        check("Deleted product: item kept in cart (not removed silently)", "true",
              boolText(cart.findItem(104) != nullptr));
        check("Deleted product: no order created", "0", to_string(queue.getSize()));
        check("placeOrder cannot bypass review (deleted product)", resultMessage(ERR_CART_NEEDS_REVIEW),
              resultMessage(placeOrder(cart, inventory, queue, "Ali", 1001)));
    }

    // 3. Deleted product - customer removes it, buys the rest
    {
        Inventory inventory;
        loadSampleProducts(inventory);
        Cart cart;
        OrderQueue queue;
        cart.addFromInventory(inventory, 101, 1);
        cart.addFromInventory(inventory, 104, 2);
        inventory.deleteProduct(104);
        int result = runScriptedCheckout(cart, inventory, queue, 1001, "Y\nAli Khan\nY\n", output);
        check("Deleted product removed by customer, checkout succeeds", resultMessage(SUCCESS),
              resultMessage(result));
        check("Order contains only the remaining item", "1/2500.00",
              to_string(queue.peek()->itemCount) + "/" + formatMoney(queue.peek()->totalAmount));
    }

    // 4. Unavailable product (stock set to 0 after adding to cart)
    {
        Inventory inventory;
        loadSampleProducts(inventory);
        Cart cart;
        OrderQueue queue;
        cart.addFromInventory(inventory, 102, 2);
        inventory.updateProduct(102, makeProduct(102, "Gaming Mouse", "Electronics", 4500, 0));
        int result = runScriptedCheckout(cart, inventory, queue, 1001, "N\n", output);
        check("Unavailable product: checkout blocked", resultMessage(ERR_CART_NEEDS_REVIEW),
              resultMessage(result));
        check("Unavailable product: customer clearly informed", "true",
              boolText(containsText(output, "unavailable")));
        check("Unavailable product: no order created", "0", to_string(queue.getSize()));
    }

    // 5 & 6. Insufficient stock - declined, then accepted
    {
        Inventory inventory;
        loadSampleProducts(inventory);
        Cart cart;
        OrderQueue queue;
        cart.addFromInventory(inventory, 104, 5);
        inventory.updateProduct(104, makeProduct(104, "Wireless Mouse", "Electronics", 1500, 3));

        int result = runScriptedCheckout(cart, inventory, queue, 1001, "N\nN\n", output);
        check("Insufficient stock: customer asked to reduce quantity", "true",
              boolText(containsText(output, "Reduce the quantity to 3?")));
        check("Insufficient stock declined: checkout blocked", resultMessage(ERR_CART_NEEDS_REVIEW),
              resultMessage(result));
        check("Insufficient stock declined: quantity NOT changed silently", "5",
              to_string(cart.findItem(104)->quantity));
        check("Insufficient stock declined: stock unchanged", "3", to_string(stockOf(inventory, 104)));

        result = runScriptedCheckout(cart, inventory, queue, 1001, "Y\nAli Khan\nY\n", output);
        check("Insufficient stock accepted (reduce to 3): checkout succeeds", resultMessage(SUCCESS),
              resultMessage(result));
        check("Order quantity reduced to available stock", "3",
              to_string(queue.peek()->itemsHead->quantity));
        check("Stock deducted after confirmation (3 - 3)", "0", to_string(stockOf(inventory, 104)));
    }

    // 7. Invalid quantities
    {
        Inventory inventory;
        loadSampleProducts(inventory);
        Cart cart;
        OrderQueue queue;
        cart.addFromInventory(inventory, 103, 2);
        check("Invalid quantity 0 cannot be set in cart", resultMessage(ERR_INVALID_QUANTITY),
              resultMessage(cart.changeQuantity(inventory, 103, 0)));
        check("Invalid negative quantity cannot be set", "false", boolText(cart.updateQuantity(103, -1)));
        check("placeOrder rejects empty customer name", resultMessage(ERR_INVALID_DATA),
              resultMessage(placeOrder(cart, inventory, queue, "   ", 1001)));
        check("Stock unchanged after rejected orders", "100", to_string(stockOf(inventory, 103)));
    }

    // 8 & 9. Changed price - cancel first, then confirm
    {
        Inventory inventory;
        loadSampleProducts(inventory);
        Cart cart;
        OrderQueue queue;
        cart.addFromInventory(inventory, 101, 2);    // 2 x 2500
        inventory.updateProduct(101, makeProduct(101, "USB Keyboard", "Electronics", 2750, 10));

        check("Outdated price can never be charged (placeOrder refuses)", resultMessage(ERR_CART_NEEDS_REVIEW),
              resultMessage(placeOrder(cart, inventory, queue, "Ali", 1001)));

        int result = runScriptedCheckout(cart, inventory, queue, 1001, "Ali Khan\nN\n", output);
        check("Changed price: old and new prices shown", "true",
              boolText(containsText(output, "old price Rs. 2500.00") && containsText(output, "new price Rs. 2750.00")));
        check("Changed price: updated total shown for review", "true",
              boolText(containsText(output, "Rs. 5500.00")));
        check("Cancelled checkout returns cancelled", resultMessage(ERR_CANCELLED), resultMessage(result));
        check("Cancelled checkout: stock NOT deducted", "10", to_string(stockOf(inventory, 101)));
        check("Cancelled checkout: no order enqueued", "0", to_string(queue.getSize()));
        check("Cancelled checkout: cart kept for editing", "1/2",
              to_string(cart.getItemCount()) + "/" + to_string(cart.findItem(101)->quantity));

        result = runScriptedCheckout(cart, inventory, queue, 1001, "Ali Khan\nY\n", output);
        check("Confirmed checkout succeeds", resultMessage(SUCCESS), resultMessage(result));
        check("Order charged at the latest price (2 x 2750)", "5500.00",
              formatMoney(queue.peek()->totalAmount));
        check("Order stores customer name and order ID", "Ali Khan/1001",
              queue.peek()->customerName + "/" + to_string(queue.peek()->orderId));
        check("Confirmed checkout: stock deducted (10 - 2)", "8", to_string(stockOf(inventory, 101)));
        check("Confirmed checkout: cart cleared", "true", boolText(cart.isEmpty()));
    }
}

// ----------------------------------------------------------------------------
// FILE HANDLING TESTS (test_data/ only)
// ----------------------------------------------------------------------------
void testFileHandling() {
    section("FILE HANDLING");
    FileManager files;
    error_code ec;

    // Missing folder and files are created automatically
    fs::remove_all(TEST_FOLDER, ec);
    check("test_data folder removed before test", "false", boolText(fs::exists(TEST_FOLDER)));
    check("setupStorage creates folder and files", "true",
          boolText(files.setupStorage(TEST_FOLDER, TEST_PRODUCTS, TEST_PENDING, TEST_COMPLETED)));
    check("All three test files now exist", "true",
          boolText(fs::exists(TEST_PRODUCTS) && fs::exists(TEST_PENDING) && fs::exists(TEST_COMPLETED)));

    // Missing files are handled safely
    Inventory noProducts;
    check("Loading a missing products file is safe", "true",
          boolText(files.loadProducts("test_data/missing_file.txt", noProducts)));
    check("Missing file gives empty inventory", "0", to_string(noProducts.getCount()));
    OrderQueue noOrders;
    check("Loading a missing orders file is safe", "true/0",
          boolText(files.loadPendingOrders("test_data/missing_orders.txt", TEST_COMPLETED, noOrders))
          + "/" + to_string(noOrders.getSize()));
    check("First order ID with no saved orders", "1001",
          to_string(files.getNextOrderId(TEST_PENDING, TEST_COMPLETED)));

    // Save and load products
    Inventory original;
    loadSampleProducts(original);
    original.addProduct(makeProduct(106, "Office Chair Deluxe", "Furniture", 12999.50, 4));
    check("Save products to file", "true", boolText(files.saveProducts(TEST_PRODUCTS, original)));
    Inventory loaded;
    files.loadProducts(TEST_PRODUCTS, loaded);
    check("Loaded product IDs match saved (sorted)", "101,102,103,104,105,106", inventoryIds(loaded));
    Product chair = loaded.getProductAt(loaded.binarySearchById(106));
    check("Loaded product details match (name with spaces, price, stock)",
          "Office Chair Deluxe|Furniture|12999.50|4",
          chair.name + "|" + chair.category + "|" + formatMoney(chair.price) + "|" + to_string(chair.stock));

    // Invalid lines are skipped
    {
        ofstream out(TEST_PRODUCTS);
        out << "abc|broken line\n";
        out << "107|Lamp|Home|-5.00|3\n";
        out << "108|Desk Lamp|Home|899.99|7\n";
        out << "108|Duplicate Lamp|Home|10.00|1\n";
    }
    Inventory partial;
    startCapture();
    files.loadProducts(TEST_PRODUCTS, partial);
    stopCapture();
    check("Invalid/duplicate lines in file are skipped", "108", inventoryIds(partial));
    files.saveProducts(TEST_PRODUCTS, original);    // restore good data

    // Save and load pending orders
    OrderQueue queue;
    Order* a = createOrder(1001, "Ali Khan");
    addItemToOrder(a, 104, "Wireless Mouse", 1500, 1);
    addItemToOrder(a, 101, "USB Keyboard", 2500, 2);
    Order* b = createOrder(1002, "Sara Ahmed");
    addItemToOrder(b, 103, "Notebook", 150, 4);
    Order* c = createOrder(1003, "Usman Ali");
    addItemToOrder(c, 102, "Gaming Mouse", 4500, 1);
    queue.enqueue(a);
    queue.enqueue(b);
    queue.enqueue(c);
    check("Save pending orders", "true", boolText(files.savePendingOrders(TEST_PENDING, queue)));

    OrderQueue reloaded;
    files.loadPendingOrders(TEST_PENDING, TEST_COMPLETED, reloaded);
    check("Reloaded pending orders keep FIFO order", "1001,1002,1003", queueIds(reloaded));
    check("Reloaded pending total matches", formatMoney(queue.calculateTotalAmount()),
          formatMoney(reloaded.calculateTotalAmount()));
    OrderItem* firstItem = reloaded.peek()->itemsHead;
    check("Reloaded order item details", "104|Wireless Mouse|1500.00|1",
          to_string(firstItem->productId) + "|" + firstItem->name + "|" +
          formatMoney(firstItem->price) + "|" + to_string(firstItem->quantity));
    check("Next order ID restored from pending file", "1004",
          to_string(files.getNextOrderId(TEST_PENDING, TEST_COMPLETED)));

    // Process orders
    startCapture();
    bool processed = files.processNextOrder(reloaded, TEST_PENDING, TEST_COMPLETED);
    stopCapture();
    check("Process next order", "true", boolText(processed));
    check("Processed order 1001 appended to completed file", "true",
          boolText(files.orderExistsInFile(TEST_COMPLETED, 1001)));
    check("Processed order 1001 removed from pending file", "false",
          boolText(files.orderExistsInFile(TEST_PENDING, 1001)));

    OrderQueue afterRestart;
    files.loadPendingOrders(TEST_PENDING, TEST_COMPLETED, afterRestart);
    check("After restart, remaining orders still in FIFO order", "1002,1003", queueIds(afterRestart));

    startCapture();
    files.processNextOrder(afterRestart, TEST_PENDING, TEST_COMPLETED);
    files.processNextOrder(afterRestart, TEST_PENDING, TEST_COMPLETED);
    stopCapture();
    string history = readWholeFile(TEST_COMPLETED);
    size_t pos1 = history.find("ORDER|1001|");
    size_t pos2 = history.find("ORDER|1002|");
    size_t pos3 = history.find("ORDER|1003|");
    check("Completed history keeps processing order 1001, 1002, 1003", "true",
          boolText(pos1 != string::npos && pos2 != string::npos && pos3 != string::npos
                   && pos1 < pos2 && pos2 < pos3));
    check("Completed history has 3 orders", "3", to_string(countOccurrences(history, "ORDER|")));
    check("Pending file empty after processing all", "0",
          to_string(countOccurrences(readWholeFile(TEST_PENDING), "ORDER|")));
    check("Next order ID restored from completed file", "1004",
          to_string(files.getNextOrderId(TEST_PENDING, TEST_COMPLETED)));

    startCapture();
    bool emptyResult = files.processNextOrder(afterRestart, TEST_PENDING, TEST_COMPLETED);
    string emptyOutput = stopCapture();
    check("Processing empty order queue is refused safely", "false", boolText(emptyResult));
    check("Empty queue message shown", "true", boolText(containsText(emptyOutput, "No pending orders")));

    // Duplicate prevention: order is in BOTH files (program closed mid-process)
    {
        Order* d = createOrder(1005, "Crash Test");
        addItemToOrder(d, 103, "Notebook", 150, 1);
        files.appendCompletedOrder(TEST_COMPLETED, d);
        OrderQueue temp;
        temp.enqueue(d);
        files.savePendingOrders(TEST_PENDING, temp);
    }
    OrderQueue recovered;
    startCapture();
    files.loadPendingOrders(TEST_PENDING, TEST_COMPLETED, recovered);
    stopCapture();
    check("Already-completed order is not loaded as pending again", "0", to_string(recovered.getSize()));
    check("Already-completed order removed from pending file", "false",
          boolText(files.orderExistsInFile(TEST_PENDING, 1005)));
    check("Order 1005 recorded once in completed file", "1",
          to_string(countOccurrences(readWholeFile(TEST_COMPLETED), "ORDER|1005|")));

    {
        Order* e = createOrder(1006, "Twice Test");
        addItemToOrder(e, 103, "Notebook", 150, 1);
        files.appendCompletedOrder(TEST_COMPLETED, e);
        recovered.enqueue(e);
    }
    startCapture();
    files.processNextOrder(recovered, TEST_PENDING, TEST_COMPLETED);
    stopCapture();
    check("processNextOrder does not write a duplicate completed entry", "1",
          to_string(countOccurrences(readWholeFile(TEST_COMPLETED), "ORDER|1006|")));
    check("Next order ID after 1006", "1007",
          to_string(files.getNextOrderId(TEST_PENDING, TEST_COMPLETED)));
}

// ----------------------------------------------------------------------------
// INTEGRATION TEST: full workflow with restarts (test_data/ only)
// ----------------------------------------------------------------------------
void testIntegration() {
    section("INTEGRATION (full workflow with restart)");
    FileManager files;
    error_code ec;
    fs::remove_all(TEST_FOLDER, ec);
    files.setupStorage(TEST_FOLDER, TEST_PRODUCTS, TEST_PENDING, TEST_COMPLETED);

    // Session 1: admin adds products, customer buys
    {
        Inventory inventory;
        Cart cart;
        OrderQueue queue;
        loadSampleProducts(inventory);
        files.saveProducts(TEST_PRODUCTS, inventory);

        cart.addFromInventory(inventory, 104, 2);
        cart.addFromInventory(inventory, 103, 5);
        int orderId = files.getNextOrderId(TEST_PENDING, TEST_COMPLETED);
        string output;
        int result = runScriptedCheckout(cart, inventory, queue, orderId, "Ali Khan\nY\n", output);
        files.saveProducts(TEST_PRODUCTS, inventory);
        files.savePendingOrders(TEST_PENDING, queue);
        check("Session 1: order placed", resultMessage(SUCCESS), resultMessage(result));

        cart.addFromInventory(inventory, 101, 1);
        orderId++;
        result = runScriptedCheckout(cart, inventory, queue, orderId, "Sara Ahmed\nY\n", output);
        files.saveProducts(TEST_PRODUCTS, inventory);
        files.savePendingOrders(TEST_PENDING, queue);
        check("Session 1: second order placed", resultMessage(SUCCESS), resultMessage(result));
    }

    // Session 2: restart, check saved data, process one order
    {
        Inventory inventory;
        OrderQueue queue;
        files.loadProducts(TEST_PRODUCTS, inventory);
        files.loadPendingOrders(TEST_PENDING, TEST_COMPLETED, queue);
        check("Session 2: reduced stock persisted (104: 25-2, 103: 100-5, 101: 10-1)", "23/95/9",
              to_string(stockOf(inventory, 104)) + "/" + to_string(stockOf(inventory, 103)) + "/" +
              to_string(stockOf(inventory, 101)));
        check("Session 2: pending orders restored in FIFO order", "1001,1002", queueIds(queue));
        check("Session 2: pending total (3000+750) + 2500", "6250.00",
              formatMoney(queue.calculateTotalAmount()));
        check("Session 2: next order ID", "1003",
              to_string(files.getNextOrderId(TEST_PENDING, TEST_COMPLETED)));
        startCapture();
        files.processNextOrder(queue, TEST_PENDING, TEST_COMPLETED);
        stopCapture();
    }

    // Session 3: restart again
    {
        OrderQueue queue;
        files.loadPendingOrders(TEST_PENDING, TEST_COMPLETED, queue);
        check("Session 3: only order 1002 still pending", "1002", queueIds(queue));
        check("Session 3: order 1001 in completed history", "true",
              boolText(files.orderExistsInFile(TEST_COMPLETED, 1001)));
        check("Session 3: next order ID still unique", "1003",
              to_string(files.getNextOrderId(TEST_PENDING, TEST_COMPLETED)));
    }
}

// ----------------------------------------------------------------------------
// MAIN
// ----------------------------------------------------------------------------
int main() {
    cout << "=========================================================" << endl;
    cout << " AUTOMATED TESTS - Online Shopping and Order Management" << endl;
    cout << " Test files: test_data/   (application data/ is not modified)" << endl;
    cout << "=========================================================" << endl;

    // Snapshot of the application's data files BEFORE testing (read only)
    bool appFolderBefore = fs::exists(APP_FOLDER);
    string productsBefore = readWholeFile(APP_PRODUCTS);
    string pendingBefore = readWholeFile(APP_PENDING);
    string completedBefore = readWholeFile(APP_COMPLETED);

    testInventory();
    testSorting();
    testCart();
    testOrders();
    testCheckout();
    testFileHandling();
    testIntegration();

    section("DATA ISOLATION");
    check("Application data/ folder existence unchanged", boolText(appFolderBefore),
          boolText(fs::exists(APP_FOLDER)));
    check("data/products.txt unchanged by tests", "true",
          boolText(readWholeFile(APP_PRODUCTS) == productsBefore));
    check("data/pending_orders.txt unchanged by tests", "true",
          boolText(readWholeFile(APP_PENDING) == pendingBefore));
    check("data/completed_orders.txt unchanged by tests", "true",
          boolText(readWholeFile(APP_COMPLETED) == completedBefore));

    cout << "\n=========================================================" << endl;
    cout << " TEST SUMMARY" << endl;
    cout << " Total: " << totalTests << "   Passed: " << passedTests
         << "   Failed: " << failedTests << endl;
    cout << "=========================================================" << endl;
    return failedTests == 0 ? 0 : 1;
}
