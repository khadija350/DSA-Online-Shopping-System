// ============================================================================
// Utils.h
// Small helper functions: text handling, number parsing, safe keyboard input.
// ============================================================================
#ifndef UTILS_H
#define UTILS_H

#include <string>

// ---------- Text helpers ----------
std::string trim(const std::string& text);              // remove spaces at both ends
std::string toLowerCase(const std::string& text);       // "Mouse" -> "mouse"
bool containsIgnoreCase(const std::string& text, const std::string& key); // substring search
bool containsChar(const std::string& text, char ch);    // true if ch appears in text

// ---------- Conversion helpers (no exceptions) ----------
bool parseInt(const std::string& text, int& value);      // "25" -> 25
bool parseDouble(const std::string& text, double& value);// "1500.50" -> 1500.5
int splitLine(const std::string& line, char delimiter, std::string parts[], int maxParts);
std::string formatMoney(double amount);                 // 1500 -> "1500.00"
double roundToCents(double amount);                     // 19.999 -> 20.00
bool pricesEqual(double a, double b);                   // compare two prices safely

// ---------- Messages and display ----------
std::string resultMessage(int code);                    // result code -> message
void printLine(int length = 72);                        // prints a line of dashes

// ---------- Safe keyboard input (never freezes on bad input) ----------
int readInt(const std::string& prompt);
int readIntInRange(const std::string& prompt, int minValue, int maxValue);
double readDouble(const std::string& prompt);
std::string readText(const std::string& prompt);        // non-empty, no '|'
bool readYesNo(const std::string& prompt);              // Y/N question

#endif
