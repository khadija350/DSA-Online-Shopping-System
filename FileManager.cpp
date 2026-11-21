// ============================================================================
// FileManager.cpp
// File handling for products, pending orders and completed orders.
//
// Safe saving: data is first written to a temporary file (e.g.
// products.txt.tmp). Only if writing succeeds does the temporary file
// replace the real file. If anything fails, the old saved data is kept.
// ============================================================================
#include "FileManager.h"
#include "Utils.h"
#include <iostream>
#include <iomanip>
#include <filesystem>
using namespace std;
namespace fs = std::filesystem;

// Creates an empty file if it does not exist yet
static bool createFileIfMissing(const string& path) {
    error_code ec;
    if (fs::exists(path, ec)) {
        return true;
    }
    ofstream file(path);
    if (!file) {
        cout << "ERROR: Could not create file " << path << endl;
        return false;
    }
    return true;
}

// If the line is an ORDER header, reads its order ID and returns true
static bool getOrderIdFromLine(const string& line, int& orderId) {
    string parts[5];
    int fields = splitLine(trim(line), '|', parts, 5);
    if (fields >= 2 && parts[0] == "ORDER") {
        return parseInt(parts[1], orderId);
    }
    return false;
}

// ----------------------------------------------------------------------------
// Setup
// ----------------------------------------------------------------------------

// Creates the data folder and the three files if they are missing
bool FileManager::setupStorage(const string& folder, const string& productsPath,
                               const string& pendingPath, const string& completedPath) {
    error_code ec;
    if (!fs::exists(folder, ec)) {
        fs::create_directories(folder, ec);
        if (ec) {
            cout << "ERROR: Could not create folder " << folder << endl;
            return false;
        }
    }
    bool ok = true;
    if (!createFileIfMissing(productsPath)) ok = false;
    if (!createFileIfMissing(pendingPath)) ok = false;
    if (!createFileIfMissing(completedPath)) ok = false;
    return ok;
}

// Moves the temporary file over the real file
bool FileManager::replaceFile(const string& tempPath, const string& finalPath) {
    error_code ec;
    fs::rename(tempPath, finalPath, ec);
    if (!ec) {
        return true;
    }
    // Some systems will not overwrite an existing file: remove it, try again
    ec.clear();
    fs::remove(finalPath, ec);
    ec.clear();
    fs::rename(tempPath, finalPath, ec);
    if (ec) {
        cout << "ERROR: Could not replace " << finalPath
             << ". The new data is still in " << tempPath << endl;
        return false;
    }
    return true;
}

// ----------------------------------------------------------------------------
// Products
// ----------------------------------------------------------------------------

// Reads products from the file into the inventory.
// A missing file simply means there are no products yet.
// Invalid lines are skipped with a warning.
bool FileManager::loadProducts(const string& path, Inventory& inventory) {
    error_code ec;
    if (!fs::exists(path, ec)) {
        return true;
    }
    ifstream in(path);
    if (!in) {
        cout << "ERROR: Could not open " << path << " for reading." << endl;
        return false;
    }

    string line;
    int lineNumber = 0;
    while (getline(in, line)) {
        lineNumber++;
        line = trim(line);
        if (line == "") {
            continue;
        }

        string parts[5];
        int fields = splitLine(line, '|', parts, 5);
        Product product;
        bool ok = (fields == 5)
                  && parseInt(parts[0], product.id)
                  && parseDouble(parts[3], product.price)
                  && parseInt(parts[4], product.stock);

        int result = ERR_INVALID_DATA;
        if (ok) {
            product.name = trim(parts[1]);
            product.category = trim(parts[2]);
            result = inventory.addProduct(product);  // also keeps the array sorted
        }
        if (result != SUCCESS) {
            cout << "WARNING: Skipped product on line " << lineNumber << " of " << path
                 << " (" << resultMessage(result) << ")" << endl;
        }
    }
    return true;
}

// Writes every product to the file (via a temporary file)
bool FileManager::saveProducts(const string& path, const Inventory& inventory) {
    string tempPath = path + ".tmp";
    ofstream out(tempPath);
    if (!out) {
        cout << "ERROR: Could not open " << tempPath << " for writing." << endl;
        return false;
    }

    out << fixed << setprecision(2);
    for (int i = 0; i < inventory.getCount(); i++) {
        Product p = inventory.getProductAt(i);
        out << p.id << '|' << p.name << '|' << p.category << '|'
            << p.price << '|' << p.stock << '\n';
    }
    out.close();

    if (out.fail()) {
        cout << "ERROR: Writing to " << tempPath << " failed. Old data kept." << endl;
        error_code ec;
        fs::remove(tempPath, ec);
        return false;
    }
    return replaceFile(tempPath, path);
}

// ----------------------------------------------------------------------------
// Orders
// ----------------------------------------------------------------------------

// Writes one order (header line + item lines)
void FileManager::writeOrder(ofstream& out, const Order* order) {
    out << "ORDER|" << order->orderId << '|' << order->customerName << '|'
        << order->itemCount << '|' << order->totalAmount << '\n';
    OrderItem* item = order->itemsHead;
    while (item != nullptr) {
        out << "ITEM|" << item->productId << '|' << item->name << '|'
            << item->price << '|' << item->quantity << '\n';
        item = item->next;
    }
}

// Reads all orders from a file and enqueues them in the same order as they
// appear in the file (so FIFO order is preserved).
bool FileManager::readOrdersFromFile(const string& path, OrderQueue& queue) {
    error_code ec;
    if (!fs::exists(path, ec)) {
        return true;                 // no file = no orders
    }
    ifstream in(path);
    if (!in) {
        cout << "ERROR: Could not open " << path << " for reading." << endl;
        return false;
    }

    Order* currentOrder = nullptr;   // order that ITEM lines belong to
    string line;
    int lineNumber = 0;
    while (getline(in, line)) {
        lineNumber++;
        line = trim(line);
        if (line == "") {
            continue;
        }

        string parts[5];
        int fields = splitLine(line, '|', parts, 5);
        bool ok = false;

        if (fields == 5 && parts[0] == "ORDER") {
            int orderId, itemCount;
            double total;
            if (parseInt(parts[1], orderId) && parseInt(parts[3], itemCount)
                && parseDouble(parts[4], total) && trim(parts[2]) != "") {
                currentOrder = createOrder(orderId, trim(parts[2]));
                queue.enqueue(currentOrder);
                ok = true;
            } else {
                currentOrder = nullptr;  // ignore items of a broken order
            }
        } else if (fields == 5 && parts[0] == "ITEM" && currentOrder != nullptr) {
            int productId, quantity;
            double price;
            if (parseInt(parts[1], productId) && parseDouble(parts[3], price)
                && parseInt(parts[4], quantity)) {
                // the total is recalculated from the items
                addItemToOrder(currentOrder, productId, trim(parts[2]), price, quantity);
                ok = true;
            }
        }

        if (!ok) {
            cout << "WARNING: Skipped invalid line " << lineNumber << " in " << path << endl;
        }
    }
    return true;
}

// Loads pending orders into the queue (front of file = front of queue).
// Any order that is already in the completed file (for example if the
// program closed while processing it) is skipped, so it is never processed
// or recorded twice. The pending file is then cleaned up.
bool FileManager::loadPendingOrders(const string& pendingPath, const string& completedPath,
                                    OrderQueue& queue) {
    OrderQueue fromFile;
    if (!readOrdersFromFile(pendingPath, fromFile)) {
        return false;
    }

    bool skippedAny = false;
    while (!fromFile.isEmpty()) {
        Order* order = fromFile.dequeue();
        if (orderExistsInFile(completedPath, order->orderId)) {
            cout << "NOTE: Order " << order->orderId
                 << " was already completed. Removed from pending orders." << endl;
            freeOrder(order);
            skippedAny = true;
        } else {
            queue.enqueue(order);
        }
    }

    if (skippedAny) {
        savePendingOrders(pendingPath, queue);
    }
    return true;
}

// Rewrites the pending orders file from the queue, front to rear
bool FileManager::savePendingOrders(const string& path, const OrderQueue& queue) {
    string tempPath = path + ".tmp";
    ofstream out(tempPath);
    if (!out) {
        cout << "ERROR: Could not open " << tempPath << " for writing." << endl;
        return false;
    }

    out << fixed << setprecision(2);
    Order* current = queue.peek();
    while (current != nullptr) {
        writeOrder(out, current);
        current = current->next;
    }
    out.close();

    if (out.fail()) {
        cout << "ERROR: Writing to " << tempPath << " failed. Old data kept." << endl;
        error_code ec;
        fs::remove(tempPath, ec);
        return false;
    }
    return replaceFile(tempPath, path);
}

// Adds one order to the END of the completed orders file (history)
bool FileManager::appendCompletedOrder(const string& path, const Order* order) {
    ofstream out(path, ios::app);
    if (!out) {
        cout << "ERROR: Could not open " << path << " for writing." << endl;
        return false;
    }
    out << fixed << setprecision(2);
    writeOrder(out, order);
    out.close();
    if (out.fail()) {
        cout << "ERROR: Writing to " << path << " failed." << endl;
        return false;
    }
    return true;
}

// Returns true if an ORDER line with this ID exists in the file
bool FileManager::orderExistsInFile(const string& path, int orderId) {
    ifstream in(path);
    if (!in) {
        return false;
    }
    string line;
    while (getline(in, line)) {
        int id;
        if (getOrderIdFromLine(line, id) && id == orderId) {
            return true;
        }
    }
    return false;
}

// Returns the largest order ID in a file (0 if there are no orders)
int FileManager::findHighestOrderId(const string& path) {
    int highest = 0;
    ifstream in(path);
    if (!in) {
        return 0;
    }
    string line;
    while (getline(in, line)) {
        int id;
        if (getOrderIdFromLine(line, id) && id > highest) {
            highest = id;
        }
    }
    return highest;
}

// Next order ID = highest ID found in pending AND completed files + 1.
// The first order ever gets ID 1001.
int FileManager::getNextOrderId(const string& pendingPath, const string& completedPath) {
    int highest = 1000;
    int pendingHighest = findHighestOrderId(pendingPath);
    int completedHighest = findHighestOrderId(completedPath);
    if (pendingHighest > highest) highest = pendingHighest;
    if (completedHighest > highest) highest = completedHighest;
    return highest + 1;
}

// Shows every order in the completed history file
void FileManager::displayCompletedOrders(const string& path) {
    OrderQueue history;
    if (!readOrdersFromFile(path, history)) {
        return;
    }
    if (history.isEmpty()) {
        cout << "No completed orders yet." << endl;
        return;
    }
    history.displayAll();
    cout << "\nTotal completed orders: " << history.getSize()
         << "    Total sales: Rs. " << formatMoney(history.calculateTotalAmount()) << endl;
}

// Processes the order at the FRONT of the queue (FIFO):
//  1. Append it to the completed file (skipped if it is already there).
//  2. Remove it from the queue.
//  3. Rewrite the pending file from the remaining queue.
// If step 1 fails, the order stays in the queue and nothing is lost.
bool FileManager::processNextOrder(OrderQueue& queue, const string& pendingPath,
                                   const string& completedPath) {
    if (queue.isEmpty()) {
        cout << "No pending orders to process. The order queue is empty." << endl;
        return false;
    }

    Order* nextOrder = queue.peek();
    int orderId = nextOrder->orderId;
    cout << "Processing the order at the front of the queue:" << endl;
    displayOrder(nextOrder);

    if (orderExistsInFile(completedPath, orderId)) {
        cout << "Note: this order is already in the completed history; it will not be added twice." << endl;
    } else if (!appendCompletedOrder(completedPath, nextOrder)) {
        cout << "ERROR: Order could not be recorded. It remains in the pending queue." << endl;
        return false;
    }

    Order* finished = queue.dequeue();
    freeOrder(finished);

    if (!savePendingOrders(pendingPath, queue)) {
        cout << "WARNING: The pending orders file could not be updated. The order is recorded"
             << " as completed and will be removed from pending orders at the next start." << endl;
    }
    cout << "Order " << orderId << " processed and moved to completed orders." << endl;
    return true;
}
