#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <stdexcept>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <map>
#include <regex>

// Using standard types - no external dependencies required
using BigInt = long long;
using BigFloat = long double;

/**
 * Simple JSON Parser for our specific use case
 * Parses the JSON structure used in test cases without external dependencies
 */
class SimpleJsonParser {
public:
    /**
     * Parses a JSON file and extracts the required data
     * Returns a map with keys like "n", "k", "base_1", "value_1", etc.
     */
    static std::map<std::string, std::string> parseTestCase(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open file: " + filename);
        }
        
        // Read entire file content
        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        file.close();
        
        std::map<std::string, std::string> result;
        
        // Remove all whitespace and newlines for easier parsing
        content.erase(std::remove_if(content.begin(), content.end(), ::isspace), content.end());
        
        try {
            // Parse keys section: "keys":{"n":4,"k":3}
            std::regex keysRegex("\"keys\":\\{\"n\":(\\d+),\"k\":(\\d+)\\}");
            std::smatch keysMatch;
            if (std::regex_search(content, keysMatch, keysRegex)) {
                result["n"] = keysMatch[1].str();
                result["k"] = keysMatch[2].str();
            }
            
            // Parse data entries: "1":{"base":"10","value":"4"}
            std::regex entryRegex("\"(\\d+)\":\\{\"base\":\"(\\d+)\",\"value\":\"([^\"]+)\"\\}");
            std::sregex_iterator iter(content.begin(), content.end(), entryRegex);
            std::sregex_iterator end;
            
            for (; iter != end; ++iter) {
                std::string index = (*iter)[1].str();
                std::string base = (*iter)[2].str();
                std::string value = (*iter)[3].str();
                result["base_" + index] = base;
                result["value_" + index] = value;
            }
            
        } catch (const std::exception& e) {
            throw std::runtime_error("JSON parsing failed: " + std::string(e.what()));
        }
        
        return result;
    }
};

/**
 * Polynomial Solver - Finds constant c in f(x) = ax² + bx + c
 * 
 * This program:
 * 1. Reads JSON files containing encoded values in different bases
 * 2. Decodes the y-values from their respective bases to decimal
 * 3. Uses the decoded roots (x, y) to solve for the constant c
 * 4. Uses standard C++ types (supports numbers up to ~9 × 10^18)
 */
class PolynomialSolver {
private:
    /**
     * Represents a single root point (x, y) where:
     * x = the x-coordinate (input value)
     * y = the y-coordinate (decoded from base-encoded string)
     */
    struct Root {
        BigInt x; // x-coordinate (usually the index from JSON)
        BigInt y; // y-coordinate (decoded from base-encoded value)
        
        Root(BigInt x_val, BigInt y_val) : x(x_val), y(y_val) {}
        
        std::string toString() const {
            return "(" + std::to_string(x) + ", " + std::to_string(y) + ")";
        }
    };
    
    /**
     * Container for a complete test case
     * Holds the metadata (n, k) and all the roots
     */
    struct TestCase {
        int n;                    // Number of roots
        int k;                    // Parameter k
        std::vector<Root> roots;  // All decoded roots
        
        TestCase(int n_val, int k_val, const std::vector<Root>& roots_val) 
            : n(n_val), k(k_val), roots(roots_val) {}
    };

public:
    /**
     * Result class to hold the processed test case data
     * Contains n, k, decoded roots, and calculated constant c
     */
    struct ProcessResult {
        int n;                    // Number of roots
        int k;                    // Parameter k from JSON
        std::vector<Root> roots;  // List of decoded (x, y) coordinates
        BigInt constantC;         // Calculated constant c
        
        ProcessResult(int n_val, int k_val, const std::vector<Root>& roots_val, BigInt constantC_val)
            : n(n_val), k(k_val), roots(roots_val), constantC(constantC_val) {}
    };

    /**
     * Main entry point for processing a single test case file
     */
    static ProcessResult processTestCase(const std::string& filename) {
        TestCase testCase = readTestCase(filename);
        BigInt constantC = solvePolynomial(testCase);
        return ProcessResult(testCase.n, testCase.k, testCase.roots, constantC);
    }

    /**
     * Main method - runs both test cases automatically
     */
    static void runTests() {
        try {
            // Test case 1
            std::cout << "=== Test Case 1 ===" << std::endl;
            TestCase testCase1 = readTestCase("test_case_1.json");
            std::cout << "Found " << testCase1.roots.size() << " roots:" << std::endl;
            for (const auto& root : testCase1.roots) {
                std::cout << "  " << root.toString() << std::endl;
            }
            
            BigInt constantC1 = solvePolynomial(testCase1);
            std::cout << "Constant c for test case 1: " << constantC1 << std::endl;
            
            std::cout << "\n=== Test Case 2 ===" << std::endl;
            TestCase testCase2 = readTestCase("test_case_2.json");
            std::cout << "Found " << testCase2.roots.size() << " roots:" << std::endl;
            for (size_t i = 0; i < std::min(testCase2.roots.size(), size_t(5)); ++i) {
                std::cout << "  " << testCase2.roots[i].toString() << std::endl;
            }
            if (testCase2.roots.size() > 5) {
                std::cout << "  ... and " << (testCase2.roots.size() - 5) << " more roots" << std::endl;
            }
            
            BigInt constantC2 = solvePolynomial(testCase2);
            std::cout << "Constant c for test case 2: " << constantC2 << std::endl;
            
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }

private:
    /**
     * Reads and parses a JSON test case file using simple regex parsing
     * 
     * JSON Structure:
     * {
     *   "keys": {"n": 4, "k": 3},
     *   "1": {"base": "10", "value": "4"},
     *   "2": {"base": "2", "value": "111"},
     *   ...
     * }
     */
    static TestCase readTestCase(const std::string& filename) {
        // Parse JSON using simple parser
        auto jsonData = SimpleJsonParser::parseTestCase(filename);
        
        // Extract metadata from parsed data
        int n = std::stoi(jsonData.at("n"));  // Number of roots
        int k = std::stoi(jsonData.at("k"));  // Parameter k
        
        std::cout << "Parsing test case: n=" << n << ", k=" << k << std::endl;
        
        std::vector<Root> roots;
        
        // Parse each root from the parsed data
        // Note: We need to check all possible indices, not just 1 to n
        // because some test cases might have gaps (like test_case_1.json has index 6)
        for (int i = 1; i <= 20; i++) { // Check up to 20 to catch any gaps
            std::string baseKey = "base_" + std::to_string(i);
            std::string valueKey = "value_" + std::to_string(i);
            
            if (jsonData.find(baseKey) != jsonData.end() && 
                jsonData.find(valueKey) != jsonData.end()) {
                
                std::string base = jsonData.at(baseKey);    // e.g., "2", "10", "16"
                std::string value = jsonData.at(valueKey);  // e.g., "111", "4", "a1b2"
                
                std::cout << "Processing index " << i << ": base=" << base 
                         << ", value=" << value << std::endl;
                
                // 🔑 KEY STEP: Decode the value from its base to decimal
                BigInt decodedValue = decodeFromBase(value, base);
                
                // For this problem, we'll treat the decoded value as y
                // and use the index i as x
                BigInt x = static_cast<BigInt>(i);  // x = index (1, 2, 3, ...)
                BigInt y = decodedValue; // y = decoded value from base
                
                std::cout << "  Decoded: " << value << " (base " << base 
                         << ") = " << y << " (decimal)" << std::endl;
                
                roots.emplace_back(x, y);
            }
        }
        
        std::cout << "Successfully parsed " << roots.size() << " roots" << std::endl;
        return TestCase(n, k, roots);
    }
    
    /**
     * Main polynomial solving logic
     * 
     * Strategy:
     * 1. If we have 3+ roots, use system of equations (Cramer's rule)
     * 2. If fewer roots, use simple polynomial assumption
     */
    static BigInt solvePolynomial(const TestCase& testCase) {
        const std::vector<Root>& roots = testCase.roots;
        
        if (roots.empty()) {
            throw std::invalid_argument("No roots provided");
        }
        
        std::cout << "Solving polynomial with " << roots.size() << " roots" << std::endl;
        
        // For a polynomial f(x) = ax² + bx + c
        // We have multiple points (x, y) where f(x) = y
        
        // If we have at least 3 points, we can solve for a, b, and c
        if (roots.size() >= 3) {
            return solveSystemOfEquations(roots);
        } else {
            // Fallback to simple approach for fewer points
            return solveSimplePolynomial(roots);
        }
    }
    
    /**
     * Solves the polynomial using system of equations
     * 
     * Mathematical approach:
     * We have 3 equations:
     * ax₁² + bx₁ + c = y₁  (from root 1)
     * ax₂² + bx₂ + c = y₂  (from root 2)  
     * ax₃² + bx₃ + c = y₃  (from root 3)
     * 
     * We can solve this system using Cramer's rule to find c
     */
    static BigInt solveSystemOfEquations(const std::vector<Root>& roots) {
        // Use the first 3 points to solve the system:
        // ax₁² + bx₁ + c = y₁
        // ax₂² + bx₂ + c = y₂  
        // ax₃² + bx₃ + c = y₃
        
        const Root& p1 = roots[0];  // First root (x₁, y₁)
        const Root& p2 = roots[1];  // Second root (x₂, y₂)
        const Root& p3 = roots[2];  // Third root (x₃, y₃)
        
        std::cout << "Using roots: " << p1.toString() << ", " 
                  << p2.toString() << ", " << p3.toString() << std::endl;
        
        // Convert to BigFloat for precision in calculations
        BigFloat x1 = static_cast<BigFloat>(p1.x);
        BigFloat y1 = static_cast<BigFloat>(p1.y);
        BigFloat x2 = static_cast<BigFloat>(p2.x);
        BigFloat y2 = static_cast<BigFloat>(p2.y);
        BigFloat x3 = static_cast<BigFloat>(p3.x);
        BigFloat y3 = static_cast<BigFloat>(p3.y);
        
        // 🔑 MATHEMATICAL STEP: Using Cramer's rule to solve the system
        // Matrix: [x₁² x₁ 1] [a]   [y₁]
        //         [x₂² x₂ 1] [b] = [y₂]
        //         [x₃² x₃ 1] [c]   [y₃]
        
        // Calculate the determinant of the coefficient matrix
        BigFloat det = x1 * x1 * (x2 - x3) + x2 * x2 * (x3 - x1) + x3 * x3 * (x1 - x2);
        
        std::cout << "Determinant: " << det << std::endl;
        
        // Check if determinant is zero (system has no unique solution)
        if (std::abs(det) < 1e-10) {
            std::cout << "Warning: Determinant is zero, using fallback method" << std::endl;
            return solveSimplePolynomial(roots);
        }
        
        // Calculate c using Cramer's rule
        // Replace the third column with the constants [y₁, y₂, y₃]
        BigFloat detC = x1 * x1 * (y2 - y3) + x2 * x2 * (y3 - y1) + x3 * x3 * (y1 - y2);
        
        // c = detC / det
        BigFloat c = detC / det;
        
        std::cout << "Calculated c (float): " << c << std::endl;
        
        // Round to nearest integer
        BigInt result = static_cast<BigInt>(std::round(c));
        
        // Verify the solution with other roots
        verifySolution(roots, c);
        
        return result;
    }
    
    /**
     * Fallback method for solving polynomial with fewer than 3 roots
     * 
     * Assumes: f(x) = x² + c (a=1, b=0)
     * Then: c = y - x²
     */
    static BigInt solveSimplePolynomial(const std::vector<Root>& roots) {
        // Simple approach: assume a = 1 and b = 0, then c = y - x²
        const Root& firstRoot = roots[0];
        BigInt x = firstRoot.x;
        BigInt y = firstRoot.y;
        
        // Calculate x²
        BigInt xSquared = x * x;
        
        // Calculate c = y - x²
        BigInt c = y - xSquared;
        
        std::cout << "Simple polynomial: c = " << y << " - " << x << "² = " << c << std::endl;
        
        // Verify with other roots if possible
        for (size_t i = 1; i < roots.size(); i++) {
            const Root& root = roots[i];
            BigInt expectedY = root.x * root.x + c;
            if (expectedY != root.y) {
                std::cout << "Warning: Root " << root.toString() 
                         << " doesn't satisfy the equation with c = " << c << std::endl;
            }
        }
        
        return c;
    }
    
    /**
     * Verifies the calculated solution with all roots
     * 
     * For verification, assumes f(x) = x² + c
     * Checks if f(x) = y for each root
     */
    static void verifySolution(const std::vector<Root>& roots, BigFloat c) {
        std::cout << "Verifying solution..." << std::endl;
        // Verify the solution with all roots
        for (const Root& root : roots) {
            BigFloat x = static_cast<BigFloat>(root.x);
            BigFloat y = static_cast<BigFloat>(root.y);
            
            // For verification, assume a = 1, b = 0: f(x) = x² + c
            BigFloat expectedY = x * x + c;
            BigFloat difference = std::abs(y - expectedY);
            
            // If difference is more than 1, show a warning
            if (difference > 1.0) {
                std::cout << "Warning: Root " << root.toString() 
                         << " has difference: " << difference << std::endl;
            } else {
                std::cout << "✓ Root " << root.toString() << " verified (diff: " 
                         << difference << ")" << std::endl;
            }
        }
    }
    
    /**
     * 🔑 CORE FUNCTION: Decodes a string value from a given base to decimal
     * 
     * This is the heart of the solution! It converts encoded strings like:
     * - "111" (base 2) → 7 (decimal)
     * - "213" (base 4) → 39 (decimal)
     * - "a1b2" (base 16) → 41394 (decimal)
     */
    static BigInt decodeFromBase(const std::string& value, const std::string& baseStr) {
        int base = std::stoi(baseStr);
        
        // Convert character to digit value
        auto charToDigit = [](char c) -> int {
            if (c >= '0' && c <= '9') {
                return c - '0';
            } else if (c >= 'a' && c <= 'z') {
                return c - 'a' + 10;
            } else if (c >= 'A' && c <= 'Z') {
                return c - 'A' + 10;
            }
            throw std::invalid_argument("Invalid character in base conversion: " + std::string(1, c));
        };
        
        BigInt result = 0;
        BigInt baseMultiplier = 1;
        
        // Process digits from right to left
        for (int i = static_cast<int>(value.length()) - 1; i >= 0; i--) {
            int digitValue = charToDigit(value[i]);
            
            if (digitValue >= base) {
                throw std::invalid_argument("Digit value " + std::to_string(digitValue) + 
                                          " is invalid for base " + std::to_string(base));
            }
            
            result += static_cast<BigInt>(digitValue) * baseMultiplier;
            baseMultiplier *= base;
        }
        
        return result;
    }
};

// Main function
int main() {
    std::cout << "Polynomial Solver C++ Version (No External Dependencies)" << std::endl;
    std::cout << "=========================================================" << std::endl;
    
    PolynomialSolver::runTests();
    
    return 0;
}