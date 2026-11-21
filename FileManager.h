// ============================================================================
// FileManager.h
// Saves and loads products and orders using text files with '|' separators.
// Every function receives the file path as a parameter, so the application
// uses the data/ folder and the test program uses the test_data/ folder.
//
// products.txt          101|Wireless Mouse|Electronics|1500.00|25
// *_orders.txt          ORDER|1001|Ali Khan|2|4500.00
//                       ITEM|101|Wireless Mouse|1500.00|1
//                       ITEM|205|Keyboard|1500.00|2
// ============================================================================
#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <string>
#include <fstream>
#include "Inventory.h"
#include "OrderQueue.h"

class FileManager {
public:
    // Creates the folder and the three data files if they do not exist
    bool setupStorage(const std::string& folder, const std::string& productsPath,
                      const std::string& pendingPath, const std::string& completedPath);

    // ----- products -----
    bool loadProducts(const std::string& path, Inventory& inventory);
    bool saveProducts(const std::string& path, const Inventory& inventory);

    // ----- orders -----
    bool loadPendingOrders(const std::string& pendingPath, const std::string& completedPath,
                           OrderQueue& queue);
    bool savePendingOrders(const std::string& path, const OrderQueue& queue);
    bool appendCompletedOrder(const std::string& path, const Order* order);
    bool orderExistsInFile(const std::string& path, int orderId);
    void displayCompletedOrders(const std::string& path);
    int getNextOrderId(const std::string& pendingPath, const std::string& completedPath);

    // Processes the front order: append to completed file, remove from queue,
    // rewrite the pending file. Returns false if nothing was processed.
    bool processNextOrder(OrderQueue& queue, const std::string& pendingPath,
                          const std::string& completedPath);

private:
    bool readOrdersFromFile(const std::string& path, OrderQueue& queue);
    void writeOrder(std::ofstream& out, const Order* order);
    bool replaceFile(const std::string& tempPath, const std::string& finalPath);
    int findHighestOrderId(const std::string& path);
};

#endif
