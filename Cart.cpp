// ============================================================================
// Cart.cpp
// Singly linked list operations for the shopping cart.
//
//   head -> [101, Mouse, x2] -> [205, Keyboard, x1] -> nullptr
// ============================================================================
#include "Cart.h"
#include "Utils.h"
#include <iostream>
#include <iomanip>
using namespace std;

Cart::Cart() {
    head = nullptr;
}

// Destructor: frees all nodes when the cart object is destroyed
Cart::~Cart() {
    clear();
}

bool Cart::isEmpty() const {
    return head == nullptr;
}

// Counts the nodes by walking from head to the end
int Cart::getItemCount() const {
    int count = 0;
    CartNode* current = head;
    while (current != nullptr) {
        count++;
        current = current->next;
    }
    return count;
}

CartNode* Cart::getHead() const {
    return head;
}

// Linear search through the list for a product ID. Returns the node or nullptr.
CartNode* Cart::findItem(int productId) const {
    CartNode* current = head;
    while (current != nullptr) {
        if (current->productId == productId) {
            return current;
        }
        current = current->next;
    }
    return nullptr;
}

// Adds an item at the END of the list.
// If the product is already in the cart, only its quantity is increased
// (no duplicate nodes).
bool Cart::addItem(int productId, const string& name, double price, int quantity) {
    if (quantity <= 0) {
        return false;
    }

    CartNode* existing = findItem(productId);
    if (existing != nullptr) {
        existing->quantity += quantity;
        existing->name = name;
        existing->price = price;    // keep the latest price
        return true;
    }

    // Create a new node
    CartNode* newNode = new CartNode;
    newNode->productId = productId;
    newNode->name = name;
    newNode->price = price;
    newNode->quantity = quantity;
    newNode->next = nullptr;

    if (head == nullptr) {
        head = newNode;              // first item in the cart
    } else {
        CartNode* current = head;    // walk to the last node
        while (current->next != nullptr) {
            current = current->next;
        }
        current->next = newNode;     // link the new node at the end
    }
    return true;
}

// Removes the node with this product ID. Returns false if it is not in the cart.
bool Cart::removeItem(int productId) {
    if (head == nullptr) {
        return false;                // empty cart
    }

    // Special case: removing the first node
    if (head->productId == productId) {
        CartNode* toDelete = head;
        head = head->next;
        delete toDelete;
        return true;
    }

    // General case: keep track of the previous node so we can re-link
    CartNode* previous = head;
    CartNode* current = head->next;
    while (current != nullptr) {
        if (current->productId == productId) {
            previous->next = current->next;  // skip over the node
            delete current;
            return true;
        }
        previous = current;
        current = current->next;
    }
    return false;                    // not found
}

// Sets a new quantity for an item (no stock check - see changeQuantity)
bool Cart::updateQuantity(int productId, int newQuantity) {
    if (newQuantity <= 0) {
        return false;
    }
    CartNode* item = findItem(productId);
    if (item == nullptr) {
        return false;
    }
    item->quantity = newQuantity;
    return true;
}

// Sets a new price for an item (used at checkout when the shop price changed)
bool Cart::updatePrice(int productId, double newPrice) {
    CartNode* item = findItem(productId);
    if (item == nullptr || newPrice <= 0) {
        return false;
    }
    item->price = newPrice;
    return true;
}

// Total = sum of (price x quantity) for every node
double Cart::calculateTotal() const {
    double total = 0.0;
    CartNode* current = head;
    while (current != nullptr) {
        total += current->price * current->quantity;
        current = current->next;
    }
    return total;
}

// Prints the cart as a table with subtotals and the total
void Cart::display() const {
    if (head == nullptr) {
        cout << "Your cart is empty." << endl;
        return;
    }
    printLine();
    cout << left << setw(5) << "No." << setw(8) << "ID" << setw(26) << "Product"
         << right << setw(12) << "Price" << setw(6) << "Qty" << setw(14) << "Subtotal" << endl;
    printLine();

    int number = 1;
    CartNode* current = head;
    while (current != nullptr) {
        cout << left << setw(5) << number << setw(8) << current->productId
             << setw(26) << current->name
             << right << setw(12) << formatMoney(current->price)
             << setw(6) << current->quantity
             << setw(14) << formatMoney(current->price * current->quantity) << endl;
        number++;
        current = current->next;
    }
    printLine();
    cout << "Cart total: Rs. " << formatMoney(calculateTotal()) << endl;
}

// Deletes every node, one at a time, starting from the head
void Cart::clear() {
    while (head != nullptr) {
        CartNode* toDelete = head;
        head = head->next;
        delete toDelete;
    }
}

// ----------------------------------------------------------------------------
// Operations that check the inventory
// ----------------------------------------------------------------------------

// Adds a product from the inventory to the cart after checking:
// valid quantity, product exists, product in stock, and that
// (quantity already in cart + new quantity) does not exceed the stock.
// Stock is NOT reduced here - only when the order is confirmed.
int Cart::addFromInventory(const Inventory& inventory, int productId, int quantity) {
    if (quantity <= 0) {
        return ERR_INVALID_QUANTITY;
    }
    int position = inventory.binarySearchById(productId);
    if (position == -1) {
        return ERR_NOT_FOUND;
    }
    Product product = inventory.getProductAt(position);
    if (product.stock == 0) {
        return ERR_OUT_OF_STOCK;
    }

    int alreadyInCart = 0;
    CartNode* existing = findItem(productId);
    if (existing != nullptr) {
        alreadyInCart = existing->quantity;
    }
    if (alreadyInCart + quantity > product.stock) {
        return ERR_INSUFFICIENT_STOCK;
    }

    addItem(productId, product.name, product.price, quantity);
    return SUCCESS;
}

// Changes the quantity of an item already in the cart, checking the stock.
// Call findItem() first to tell "not in cart" apart from "removed from shop".
int Cart::changeQuantity(const Inventory& inventory, int productId, int newQuantity) {
    if (findItem(productId) == nullptr) {
        return ERR_NOT_FOUND;
    }
    if (newQuantity <= 0) {
        return ERR_INVALID_QUANTITY;
    }
    int position = inventory.binarySearchById(productId);
    if (position == -1) {
        return ERR_NOT_FOUND;            // product deleted from the shop
    }
    Product product = inventory.getProductAt(position);
    if (product.stock == 0) {
        return ERR_OUT_OF_STOCK;
    }
    if (newQuantity > product.stock) {
        return ERR_INSUFFICIENT_STOCK;
    }
    updateQuantity(productId, newQuantity);
    return SUCCESS;
}

// Compares one cart item with the CURRENT inventory. Returns:
//   ERR_NOT_FOUND          - product was deleted from the shop
//   ERR_OUT_OF_STOCK       - product exists but stock is 0
//   ERR_INSUFFICIENT_STOCK - cart quantity is more than current stock
//   SUCCESS                - item can be bought
// (Price changes are checked separately with pricesEqual().)
int Cart::checkItem(const CartNode* item, const Inventory& inventory) const {
    int position = inventory.binarySearchById(item->productId);
    if (position == -1) {
        return ERR_NOT_FOUND;
    }
    Product product = inventory.getProductAt(position);
    if (product.stock == 0) {
        return ERR_OUT_OF_STOCK;
    }
    if (item->quantity > product.stock) {
        return ERR_INSUFFICIENT_STOCK;
    }
    return SUCCESS;
}

// True only if the cart is not empty, every item is available in the needed
// quantity, and every cart price equals the current shop price.
// placeOrder() uses this as a final safety check so an outdated price
// can never be charged.
bool Cart::isReadyForCheckout(const Inventory& inventory) const {
    if (head == nullptr) {
        return false;
    }
    CartNode* current = head;
    while (current != nullptr) {
        if (checkItem(current, inventory) != SUCCESS) {
            return false;
        }
        Product product = inventory.getProductAt(inventory.binarySearchById(current->productId));
        if (!pricesEqual(current->price, product.price)) {
            return false;
        }
        current = current->next;
    }
    return true;
}
