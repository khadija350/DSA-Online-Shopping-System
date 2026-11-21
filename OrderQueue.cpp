// ============================================================================
// OrderQueue.cpp
// Linked-list queue of orders, plus the checkout process that turns the
// shopping cart into an order.
// ============================================================================
#include "OrderQueue.h"
#include "Utils.h"
#include <iostream>
#include <iomanip>
using namespace std;

// ----------------------------------------------------------------------------
// Single-order helpers
// ----------------------------------------------------------------------------

// Creates a new order with no items yet
Order* createOrder(int orderId, const string& customerName) {
    Order* order = new Order;
    order->orderId = orderId;
    order->customerName = customerName;
    order->itemsHead = nullptr;
    order->itemCount = 0;
    order->totalAmount = 0.0;
    order->next = nullptr;
    return order;
}

// Appends an item to the end of the order's item list and updates the
// item count and total purchase amount
void addItemToOrder(Order* order, int productId, const string& name,
                    double price, int quantity) {
    OrderItem* item = new OrderItem;
    item->productId = productId;
    item->name = name;
    item->price = price;
    item->quantity = quantity;
    item->next = nullptr;

    if (order->itemsHead == nullptr) {
        order->itemsHead = item;
    } else {
        OrderItem* current = order->itemsHead;
        while (current->next != nullptr) {
            current = current->next;
        }
        current->next = item;
    }
    order->itemCount++;
    order->totalAmount += price * quantity;
}

// Deletes all items of an order and then the order itself
void freeOrder(Order* order) {
    if (order == nullptr) {
        return;
    }
    OrderItem* current = order->itemsHead;
    while (current != nullptr) {
        OrderItem* toDelete = current;
        current = current->next;
        delete toDelete;
    }
    delete order;
}

// Prints one order like a receipt
void displayOrder(const Order* order) {
    if (order == nullptr) {
        return;
    }
    cout << "Order ID: " << order->orderId << "    Customer: " << order->customerName << endl;
    printLine();
    cout << left << setw(8) << "ID" << setw(26) << "Product"
         << right << setw(12) << "Price" << setw(6) << "Qty" << setw(14) << "Subtotal" << endl;
    printLine();
    OrderItem* item = order->itemsHead;
    while (item != nullptr) {
        cout << left << setw(8) << item->productId << setw(26) << item->name
             << right << setw(12) << formatMoney(item->price)
             << setw(6) << item->quantity
             << setw(14) << formatMoney(item->price * item->quantity) << endl;
        item = item->next;
    }
    printLine();
    cout << "Items: " << order->itemCount
         << "    Total purchase amount: Rs. " << formatMoney(order->totalAmount) << endl;
}

// ----------------------------------------------------------------------------
// OrderQueue (linked-list queue)
// ----------------------------------------------------------------------------

OrderQueue::OrderQueue() {
    front = nullptr;
    rear = nullptr;
    size = 0;
}

// Destructor: frees every order still in the queue
OrderQueue::~OrderQueue() {
    clear();
}

bool OrderQueue::isEmpty() const {
    return front == nullptr;
}

int OrderQueue::getSize() const {
    return size;
}

// ENQUEUE - O(1): adds the order at the rear of the queue
void OrderQueue::enqueue(Order* order) {
    order->next = nullptr;
    if (rear == nullptr) {
        front = order;          // queue was empty: order is both front and rear
        rear = order;
    } else {
        rear->next = order;     // link after the current last order
        rear = order;
    }
    size++;
}

// DEQUEUE - O(1): removes the order at the front and returns it.
// Returns nullptr if the queue is empty.
// The caller becomes responsible for the order and must call freeOrder().
Order* OrderQueue::dequeue() {
    if (front == nullptr) {
        return nullptr;
    }
    Order* removed = front;
    front = front->next;
    if (front == nullptr) {
        rear = nullptr;         // queue is now empty
    }
    removed->next = nullptr;
    size--;
    return removed;
}

// PEEK: returns the front order without removing it (nullptr if empty)
Order* OrderQueue::peek() const {
    return front;
}

// Prints all orders from front to rear (the order they will be processed)
void OrderQueue::displayAll() const {
    if (front == nullptr) {
        cout << "No orders to display." << endl;
        return;
    }
    int position = 1;
    Order* current = front;
    while (current != nullptr) {
        cout << "\n[" << position << "] ";
        displayOrder(current);
        position++;
        current = current->next;
    }
}

// Adds up the total purchase amount of every order in the queue
double OrderQueue::calculateTotalAmount() const {
    double total = 0.0;
    Order* current = front;
    while (current != nullptr) {
        total += current->totalAmount;
        current = current->next;
    }
    return total;
}

// Removes and frees every order
void OrderQueue::clear() {
    while (front != nullptr) {
        Order* toDelete = front;
        front = front->next;
        freeOrder(toDelete);
    }
    rear = nullptr;
    size = 0;
}

// ----------------------------------------------------------------------------
// Checkout
// ----------------------------------------------------------------------------

// Places the order. This is the ONLY place where stock is deducted.
// Steps: final safety check -> copy cart items into a new order ->
// reduce stock -> enqueue order -> clear cart.
int placeOrder(Cart& cart, Inventory& inventory, OrderQueue& queue,
               const string& customerName, int orderId) {
    if (cart.isEmpty()) {
        return ERR_EMPTY_CART;
    }
    if (trim(customerName) == "" || containsChar(customerName, '|')) {
        return ERR_INVALID_DATA;
    }
    // Every item must be available and priced at the CURRENT shop price
    if (!cart.isReadyForCheckout(inventory)) {
        return ERR_CART_NEEDS_REVIEW;
    }

    Order* order = createOrder(orderId, trim(customerName));
    CartNode* current = cart.getHead();
    while (current != nullptr) {
        addItemToOrder(order, current->productId, current->name,
                       current->price, current->quantity);
        inventory.reduceStock(current->productId, current->quantity);
        current = current->next;
    }

    queue.enqueue(order);
    cart.clear();
    return SUCCESS;
}

// Interactive checkout following the approved rules:
//  1. Empty cart -> not allowed.
//  2. Every item is re-checked against the current inventory.
//  3. Deleted / out-of-stock items must be removed before checkout continues.
//  4. If quantity > stock, the customer is ASKED whether to reduce it.
//  5. If the customer declines, the item is left for them to edit or remove.
//  6. Changed prices are shown (old and new) and the cart uses the new price.
//  7-9. Final cart and new total are shown, then confirmation is required.
// Stock is changed only if the customer confirms (inside placeOrder).
int runCheckout(Cart& cart, Inventory& inventory, OrderQueue& queue, int orderId) {
    cout << "\n========== CHECKOUT ==========" << endl;
    if (cart.isEmpty()) {
        cout << "Your cart is empty. Please add products before checkout." << endl;
        return ERR_EMPTY_CART;
    }

    cout << "Checking your cart against the current inventory..." << endl;
    bool mustFixCart = false;     // true if an item still needs the customer's action
    bool priceChanged = false;

    CartNode* current = cart.getHead();
    while (current != nullptr) {
        // Remember the next node now, because 'current' may be deleted below
        CartNode* nextNode = current->next;
        int productId = current->productId;
        string name = current->name;
        int status = cart.checkItem(current, inventory);
        bool removed = false;

        if (status == ERR_NOT_FOUND || status == ERR_OUT_OF_STOCK) {
            // Product deleted from the shop, or stock is zero
            if (status == ERR_NOT_FOUND) {
                cout << "\n[!] \"" << name << "\" (ID " << productId
                     << ") has been removed from the shop and is no longer available." << endl;
            } else {
                cout << "\n[!] \"" << name << "\" (ID " << productId
                     << ") is currently unavailable (out of stock)." << endl;
            }
            cout << "    It must be removed from your cart before checkout can continue." << endl;
            if (readYesNo("    Remove it from your cart now? (Y/N): ")) {
                cart.removeItem(productId);
                removed = true;
                cout << "    Item removed from cart." << endl;
            } else {
                mustFixCart = true;
            }
        } else if (status == ERR_INSUFFICIENT_STOCK) {
            Product product = inventory.getProductAt(inventory.binarySearchById(productId));
            cout << "\n[!] Not enough stock for \"" << name << "\" (ID " << productId << ")." << endl;
            cout << "    You requested " << current->quantity << " but only "
                 << product.stock << " are available." << endl;
            if (readYesNo("    Reduce the quantity to " + to_string(product.stock) + "? (Y/N): ")) {
                cart.updateQuantity(productId, product.stock);
                cout << "    Quantity reduced to " << product.stock << "." << endl;
            } else if (readYesNo("    Remove this item from your cart instead? (Y/N): ")) {
                cart.removeItem(productId);
                removed = true;
                cout << "    Item removed from cart." << endl;
            } else {
                mustFixCart = true;
                cout << "    Item left unchanged." << endl;
            }
        }

        // Price check for items that are still in the cart and still in the shop
        if (!removed && status != ERR_NOT_FOUND) {
            Product product = inventory.getProductAt(inventory.binarySearchById(productId));
            if (!pricesEqual(current->price, product.price)) {
                cout << "\n[!] Price changed for \"" << name << "\": old price Rs. "
                     << formatMoney(current->price) << ", new price Rs. "
                     << formatMoney(product.price) << endl;
                cart.updatePrice(productId, product.price);
                priceChanged = true;
            }
        }

        current = nextNode;
    }

    if (mustFixCart) {
        cout << "\nCheckout cannot continue until the items above are fixed." << endl;
        cout << "Use 'Update cart quantity' or 'Remove cart item' from the Customer Menu." << endl;
        return ERR_CART_NEEDS_REVIEW;
    }
    if (cart.isEmpty()) {
        cout << "\nYour cart is now empty. Checkout cancelled." << endl;
        return ERR_EMPTY_CART;
    }

    cout << "\n---------- FINAL CART ----------" << endl;
    cart.display();
    if (priceChanged) {
        cout << "NOTE: Some prices have changed. The total above uses the latest prices."
             << " Please review it before confirming." << endl;
    }

    string customerName = readText("Enter your name: ");
    double total = cart.calculateTotal();
    if (!readYesNo("Confirm order for a total of Rs. " + formatMoney(total) + "? (Y/N): ")) {
        cout << "Checkout cancelled. No order was placed and no stock was changed." << endl;
        cout << "Your cart has been kept so you can continue shopping or edit it." << endl;
        return ERR_CANCELLED;
    }

    int result = placeOrder(cart, inventory, queue, customerName, orderId);
    if (result == SUCCESS) {
        cout << "\nOrder placed successfully!" << endl;
        cout << "Order ID: " << orderId << "    Customer: " << trim(customerName)
             << "    Total: Rs. " << formatMoney(total) << endl;
        cout << "Your order is number " << queue.getSize() << " in the processing queue." << endl;
    } else {
        cout << resultMessage(result) << endl;
    }
    return result;
}
