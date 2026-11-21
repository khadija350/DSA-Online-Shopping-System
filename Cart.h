// ============================================================================
// Cart.h
// Shopping cart implemented as a SINGLY LINKED LIST.
// Each node holds one product the customer wants to buy.
// The cart only exists while the program is running (one session).
// ============================================================================
#ifndef CART_H
#define CART_H

#include <string>
#include "Inventory.h"

// One item (node) in the cart linked list
struct CartNode {
    int productId;
    std::string name;
    double price;       // price when added (refreshed at checkout if changed)
    int quantity;
    CartNode* next;     // pointer to the next item, nullptr if last
};

class Cart {
private:
    CartNode* head;     // first node of the list (nullptr = empty cart)

public:
    Cart();
    ~Cart();                             // frees every node

    // A cart owns its nodes, so copying a cart is not allowed
    // (two carts sharing the same nodes would delete them twice).
    Cart(const Cart&) = delete;
    Cart& operator=(const Cart&) = delete;

    // ----- basic linked-list operations -----
    bool isEmpty() const;
    int getItemCount() const;            // number of nodes
    CartNode* getHead() const;           // first node (for walking the list)
    CartNode* findItem(int productId) const;
    bool addItem(int productId, const std::string& name, double price, int quantity);
    bool removeItem(int productId);
    bool updateQuantity(int productId, int newQuantity);
    bool updatePrice(int productId, double newPrice);
    double calculateTotal() const;
    void display() const;
    void clear();                        // deletes all nodes

    // ----- operations that check against the inventory (return result codes) -----
    int addFromInventory(const Inventory& inventory, int productId, int quantity);
    int changeQuantity(const Inventory& inventory, int productId, int newQuantity);
    int checkItem(const CartNode* item, const Inventory& inventory) const;
    bool isReadyForCheckout(const Inventory& inventory) const;
};

#endif
