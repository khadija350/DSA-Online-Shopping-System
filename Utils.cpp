// ============================================================================
// Utils.cpp
// Implementation of the helper functions declared in Utils.h.
// All keyboard input is read one full line at a time with getline(), then
// checked. This way a wrong entry (e.g. letters instead of a number) never
// puts cin into a failed state and the menu never freezes.
// ============================================================================
#include "Utils.h"
#include "Product.h"
#include <iostream>
#include <cstdlib>
using namespace std;

// Returns true for space-like characters (space, tab, carriage return, newline)
static bool isSpaceChar(char ch) {
    return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
}

// Removes spaces from the start and end of the text
string trim(const string& text) {
    int start = 0;
    int end = (int)text.length() - 1;
    while (start <= end && isSpaceChar(text[start])) {
        start++;
    }
    while (end >= start && isSpaceChar(text[end])) {
        end--;
    }
    return text.substr(start, end - start + 1);
}

// Converts every capital letter to a small letter
string toLowerCase(const string& text) {
    string result = text;
    for (int i = 0; i < (int)result.length(); i++) {
        if (result[i] >= 'A' && result[i] <= 'Z') {
            result[i] = result[i] + ('a' - 'A');
        }
    }
    return result;
}

// Checks whether 'key' appears anywhere inside 'text' (capital letters ignored).
// Simple pattern search: try every starting position and compare characters.
bool containsIgnoreCase(const string& text, const string& key) {
    string t = toLowerCase(text);
    string k = toLowerCase(key);
    int textLength = (int)t.length();
    int keyLength = (int)k.length();

    for (int start = 0; start + keyLength <= textLength; start++) {
        int matched = 0;
        while (matched < keyLength && t[start + matched] == k[matched]) {
            matched++;
        }
        if (matched == keyLength) {
            return true;
        }
    }
    return false;
}

// Returns true if the character appears in the text
bool containsChar(const string& text, char ch) {
    for (int i = 0; i < (int)text.length(); i++) {
        if (text[i] == ch) {
            return true;
        }
    }
    return false;
}

// Converts text such as "25" or "-3" to an int.
// Returns false (instead of crashing) if the text is not a valid whole number.
bool parseInt(const string& text, int& value) {
    string s = trim(text);
    int length = (int)s.length();
    if (length == 0) {
        return false;
    }

    int i = 0;
    bool negative = false;
    if (s[0] == '-' || s[0] == '+') {
        negative = (s[0] == '-');
        i = 1;
    }
    if (i >= length) {
        return false;            // only a sign, no digits
    }

    long long result = 0;
    for (; i < length; i++) {
        if (s[i] < '0' || s[i] > '9') {
            return false;        // not a digit
        }
        result = result * 10 + (s[i] - '0');
        if (result > 2147483647LL) {
            return false;        // too large for an int
        }
    }

    if (negative) {
        value = (int)(-result);
    } else {
        value = (int)result;
    }
    return true;
}

// Converts text such as "1500.50" to a double.
// Returns false if the text is not a valid decimal number.
bool parseDouble(const string& text, double& value) {
    string s = trim(text);
    int length = (int)s.length();
    if (length == 0) {
        return false;
    }

    int i = 0;
    bool negative = false;
    if (s[0] == '-' || s[0] == '+') {
        negative = (s[0] == '-');
        i = 1;
    }

    double result = 0.0;
    double placeValue = 0.1;   // value of the next digit after the decimal point
    bool digitFound = false;
    bool dotFound = false;

    for (; i < length; i++) {
        if (s[i] >= '0' && s[i] <= '9') {
            digitFound = true;
            int digit = s[i] - '0';
            if (!dotFound) {
                result = result * 10 + digit;
            } else {
                result = result + digit * placeValue;
                placeValue = placeValue / 10;
            }
        } else if (s[i] == '.' && !dotFound) {
            dotFound = true;
        } else {
            return false;        // invalid character
        }
    }

    if (!digitFound || result > 1000000000000.0) {
        return false;            // no digits, or unreasonably large
    }

    if (negative) {
        value = -result;
    } else {
        value = result;
    }
    return true;
}

// Splits a line like "101|Mouse|Electronics" into parts using the delimiter.
// Stores at most maxParts parts, but returns the TOTAL number of fields found,
// so the caller can detect lines with too many or too few fields.
int splitLine(const string& line, char delimiter, string parts[], int maxParts) {
    int count = 0;
    string current = "";
    for (int i = 0; i < (int)line.length(); i++) {
        if (line[i] == delimiter) {
            if (count < maxParts) {
                parts[count] = current;
            }
            count++;
            current = "";
        } else {
            current += line[i];
        }
    }
    // the last field (after the final delimiter)
    if (count < maxParts) {
        parts[count] = current;
    }
    count++;
    return count;
}

// Rounds an amount to 2 decimal places (whole paisa)
double roundToCents(double amount) {
    if (amount < 0) {
        return -roundToCents(-amount);
    }
    long long cents = (long long)(amount * 100 + 0.5);
    return cents / 100.0;
}

// Turns an amount into text with exactly 2 decimals, e.g. 1500 -> "1500.00"
string formatMoney(double amount) {
    string sign = "";
    if (amount < 0) {
        sign = "-";
        amount = -amount;
    }
    long long cents = (long long)(amount * 100 + 0.5);
    long long whole = cents / 100;
    int fraction = (int)(cents % 100);

    string result = sign + to_string(whole) + ".";
    if (fraction < 10) {
        result += "0";
    }
    result += to_string(fraction);
    return result;
}

// Two prices are treated as equal if they differ by less than half a paisa.
// (Decimal numbers stored in a double may differ by a tiny amount.)
bool pricesEqual(double a, double b) {
    double difference = a - b;
    if (difference < 0) {
        difference = -difference;
    }
    return difference < 0.005;
}

// Converts a result code into a message the user can understand
string resultMessage(int code) {
    switch (code) {
        case SUCCESS:                return "Success.";
        case ERR_DUPLICATE_ID:       return "Error: A product with this ID already exists.";
        case ERR_INVENTORY_FULL:     return "Error: Inventory is full (maximum 100 products).";
        case ERR_INVALID_DATA:       return "Error: Invalid data entered.";
        case ERR_NOT_FOUND:          return "Error: Product not found.";
        case ERR_OUT_OF_STOCK:       return "Error: This product is out of stock.";
        case ERR_INVALID_QUANTITY:   return "Error: Quantity must be at least 1.";
        case ERR_INSUFFICIENT_STOCK: return "Error: Not enough stock available.";
        case ERR_EMPTY_CART:         return "Error: The cart is empty.";
        case ERR_CART_NEEDS_REVIEW:  return "Error: The cart has items that must be fixed before checkout.";
        case ERR_CANCELLED:          return "Checkout was cancelled.";
        default:                     return "Unknown result.";
    }
}

// Prints a horizontal line of dashes
void printLine(int length) {
    cout << string(length, '-') << endl;
}

// Reads one full line from the keyboard.
// If input has ended (e.g. the console was closed), the program exits safely.
// All data is saved immediately after each change, so nothing is lost.
static string readLine() {
    string line;
    if (!getline(cin, line)) {
        cout << "\nInput ended. Exiting program." << endl;
        exit(0);
    }
    return line;
}

// Keeps asking until the user types a valid whole number
int readInt(const string& prompt) {
    while (true) {
        cout << prompt;
        int value;
        if (parseInt(readLine(), value)) {
            return value;
        }
        cout << "Invalid input. Please enter a whole number." << endl;
    }
}

// Keeps asking until the user types a whole number between minValue and maxValue
int readIntInRange(const string& prompt, int minValue, int maxValue) {
    while (true) {
        int value = readInt(prompt);
        if (value >= minValue && value <= maxValue) {
            return value;
        }
        cout << "Please enter a number from " << minValue << " to " << maxValue << "." << endl;
    }
}

// Keeps asking until the user types a valid decimal number
double readDouble(const string& prompt) {
    while (true) {
        cout << prompt;
        double value;
        if (parseDouble(readLine(), value)) {
            return value;
        }
        cout << "Invalid input. Please enter a number (e.g. 1500 or 99.50)." << endl;
    }
}

// Keeps asking until the user types non-empty text without the '|' character
// ('|' is not allowed because it separates fields in the data files)
string readText(const string& prompt) {
    while (true) {
        cout << prompt;
        string text = trim(readLine());
        if (text == "") {
            cout << "This field cannot be empty." << endl;
        } else if (containsChar(text, '|')) {
            cout << "The '|' character is not allowed." << endl;
        } else {
            return text;
        }
    }
}

// Asks a yes/no question. Returns true for Y/yes and false for N/no.
bool readYesNo(const string& prompt) {
    while (true) {
        cout << prompt;
        string answer = toLowerCase(trim(readLine()));
        if (answer == "y" || answer == "yes") {
            return true;
        }
        if (answer == "n" || answer == "no") {
            return false;
        }
        cout << "Please enter Y or N." << endl;
    }
}
