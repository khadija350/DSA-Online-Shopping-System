// ============================================================================
// Product.h
// Defines the Product record and the result codes shared by all modules.
// ============================================================================
#ifndef PRODUCT_H
#define PRODUCT_H

#include <string>

// Maximum number of products the inventory array can hold
const int MAX_PRODUCTS = 100;

// One product record in the shop
struct Product {
    int id;               // unique product ID (positive number)
    std::string name;     // product name, e.g. "Wireless Mouse"
    std::string category; // category, e.g. "Electronics"
    double price;         // price of one unit in Rupees (must be > 0)
    int stock;            // units available in inventory (must be >= 0)
};

// ----------------------------------------------------------------------------
// Result codes returned by functions so the caller knows what happened.
// resultMessage() in Utils.cpp turns a code into a readable message.
// ----------------------------------------------------------------------------
const int SUCCESS                = 0;
const int ERR_DUPLICATE_ID       = 1;
const int ERR_INVENTORY_FULL     = 2;
const int ERR_INVALID_DATA       = 3;
const int ERR_NOT_FOUND          = 4;
const int ERR_OUT_OF_STOCK       = 5;
const int ERR_INVALID_QUANTITY   = 6;
const int ERR_INSUFFICIENT_STOCK = 7;
const int ERR_EMPTY_CART         = 8;
const int ERR_CART_NEEDS_REVIEW  = 9;
const int ERR_CANCELLED          = 10;

#endif
