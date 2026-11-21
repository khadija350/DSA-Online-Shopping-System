# PROJECT STATUS – Online Shopping and Order Management System

_Last updated: 2026-11-20_

## Overall status
**Implementation complete.** Both programs compile with no errors or warnings, and all 168 automated tests pass.
Report/submission documentation has **not** been started yet (planned as a separate task).

## Files
| File | Purpose |
|---|---|
| `Product.h` | `Product` struct, `MAX_PRODUCTS = 100`, shared result codes |
| `Utils.h/.cpp` | trim/lowercase/substring search, manual int/double parsing, line splitting, money formatting, safe input (`readInt`, `readIntInRange`, `readDouble`, `readText`, `readYesNo`), `resultMessage` |
| `Inventory.h/.cpp` | Product array always sorted by ID; add/update (incl. ID change)/delete/reduceStock; binary search (ID); linear search (name, category, price range); insertion sort (price asc/desc, name); bubble sort (stock asc/desc) on a temporary copy |
| `Cart.h/.cpp` | Singly linked list cart; add (merges duplicates)/remove/update qty/update price/total/clear; inventory-checked `addFromInventory`, `changeQuantity`, `checkItem`, `isReadyForCheckout` |
| `OrderQueue.h/.cpp` | `OrderItem`/`Order` structs, linked-list queue (front/rear) with enqueue/dequeue/peek/display/total/clear; `placeOrder` (only place stock is deducted); interactive `runCheckout` |
| `FileManager.h/.cpp` | Creates folders/files; load/save products; load/save pending orders; append/display completed orders; next-order-ID recovery; `processNextOrder`. Saves via temp file + replace so old data is never destroyed on failure |
| `main.cpp` | Main/Admin/Customer menus using `data/` |
| `tests.cpp` | 168 automated tests using `test_data/` only |
| `data/` | Application data (created automatically, currently empty) |
| `test_data/` | Test data (recreated by `tests.exe` on every run) |

## Build commands (GCC 14.2, MSYS2)
```
g++ -std=c++17 main.cpp Inventory.cpp Cart.cpp OrderQueue.cpp FileManager.cpp Utils.cpp -o shop.exe
g++ -std=c++17 tests.cpp Inventory.cpp Cart.cpp OrderQueue.cpp FileManager.cpp Utils.cpp -o tests.exe
```
Run both from the project folder (paths `data/` and `test_data/` are relative):
```
.\shop.exe
.\tests.exe
```

## Build results
- `shop.exe`: compiled successfully, 0 errors, 0 warnings (also checked with `-Wall -Wextra`).
- `tests.exe`: compiled successfully, 0 errors, 0 warnings (also checked with `-Wall -Wextra`).

## Test results
**168 tests: 168 passed, 0 failed.**

| Section | Covers |
|---|---|
| Inventory | valid add, sorted insertion, duplicate ID, invalid values (negative/zero price, negative stock, empty name, ID 0, `|`), full array (100), update existing/missing, ID change success/duplicate, delete existing/missing/empty, binary search (first/middle/last/missing/empty), name/category/price-range search incl. boundaries and invalid range |
| Sorting | price asc/desc, name A–Z, stock asc/desc, main array still sorted by ID, empty and single-item inventory |
| Cart | add, add same item (merge), total, qty 0/negative, missing product, out-of-stock, qty > stock (incl. qty already in cart), stock not reduced, update qty valid/too high/0, remove existing/missing/from empty, clear |
| Orders | order total & item count, enqueue, peek, FIFO dequeue, pending total, empty queue dequeue/peek/total, re-use after empty, clear |
| Checkout | empty cart, deleted product (refuse remove → blocked; remove → succeeds), unavailable product, insufficient stock (declined → qty unchanged; accepted → reduced), invalid quantities, empty name, outdated price refused by `placeOrder`, old/new price shown, updated total shown, cancel (no stock change, no order, cart kept), confirm (latest price charged, stock deducted, cart cleared) |
| File handling | auto-create folder/files, missing files, save/load products, invalid lines skipped, save/load pending (FIFO + details), process order (appended to completed, removed from pending), completed history order, empty-queue processing, duplicate-completed prevention, next-ID recovery |
| Integration | two orders placed → restart → stock/pending/next-ID restored → process → restart → history correct |
| Data isolation | `data/` folder and all three files byte-identical before/after tests |

Interactive checkout tests feed scripted answers to the real `runCheckout` function (via `cin` redirection) and inspect its printed output.

## Manual end-to-end verification (done)
Three scripted sessions of `shop.exe` with restarts checked: product add with invalid inputs, duplicate ID, cart add/limits, checkout,
price-change notice, insufficient-stock prompt (declined → blocked), cancel (cart kept), confirmed order at new price, pending total,
FIFO processing, empty-queue message, completed history, ID change keeping the array sorted, next order ID after restart (1003).
The demo data was then removed; `data/` now contains three empty files.

## Remaining issues / notes
- None known.
- Note for testing by script in Windows PowerShell 5.1: piping text into `shop.exe` with `|` adds a byte-order mark to the first
  line, so the first menu choice is rejected as invalid. Typing normally or using `cmd /c "shop.exe < input.txt"` with an ASCII file works.
- `tests.cpp` uses `<sstream>` only for test-harness purposes (capturing output / scripted input); the application itself does not.

## Next steps (separate tasks)
- Project report, test case table for submission, viva preparation.
