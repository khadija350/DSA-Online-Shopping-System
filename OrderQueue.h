// ============================================================================
// OrderQueue.h
// Customer orders are stored in a QUEUE implemented as a linked list.
// FIFO: orders are added at the REAR and processed from the FRONT, so the
// first order placed is the first order processed.
//
//   front -> [Order 1001] -> [Order 1002] -> [Order 1003] <- rear
//
// Each Order also contains its own linked list of OrderItems.
// ============================================================================
#ifndef ORDERQUEUE_H
#define ORDERQUEUE_H

#include <string>
#include "Inventory.h"
#include "Cart.h"

// One purchased product inside an order (a copy of the details at purchase time)
struct OrderItem {
    int productId;
    std::string name;
    double price;       // price actually charged
    int quantity;
    OrderItem* next;
};

// One customer order (a node of the queue)
struct Order {
    int orderId;
    std::string customerName;
    OrderItem* itemsHead;   // linked list of purchased items
    int itemCount;          // number of different products in the order
    double totalAmount;     // total purchase amount
    Order* next;            // next order in the queue
};

// ----- helper functions for a single order -----
Order* createOrder(int orderId, const std::string& customerName); // new empty order
void addItemToOrder(Order* order, int productId, const std::string& name,
                    double price, int quantity);                   // append item, update total
void freeOrder(Order* order);                                      // delete order + its items
void displayOrder(const Order* order);

class OrderQueue {
private:
    Order* front;   // first order (next to be processed)
    Order* rear;    // last order (most recently placed)
    int size;

public:
    OrderQueue();
    ~OrderQueue();                       // frees every order

    // A queue owns its orders, so copying a queue is not allowed.
    OrderQueue(const OrderQueue&) = delete;
    OrderQueue& operator=(const OrderQueue&) = delete;

    bool isEmpty() const;
    int getSize() const;
    void enqueue(Order* order);          // add at rear
    Order* dequeue();                    // remove from front (caller must freeOrder it)
    Order* peek() const;                 // look at front without removing
    void displayAll() const;
    double calculateTotalAmount() const; // sum of all orders in the queue
    void clear();
};

// ----- checkout -----
// Turns a fully checked cart into an order: deducts stock, enqueues the order
// and clears the cart. Returns a result code.
int placeOrder(Cart& cart, Inventory& inventory, OrderQueue& queue,
               const std::string& customerName, int orderId);

// Interactive checkout: re-checks every cart item, asks the customer about
// problems, shows the final cart and asks for confirmation before placing
// the order. Returns SUCCESS only if an order was placed.
int runCheckout(Cart& cart, Inventory& inventory, OrderQueue& queue, int orderId);

#endif
