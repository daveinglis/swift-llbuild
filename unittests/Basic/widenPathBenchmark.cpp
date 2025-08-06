//===- unittests/Basic/widenPathBenchmark.cpp -----------------------------===//
//
// Performance benchmark for widenPath function on Windows
//
//===----------------------------------------------------------------------===//

#include "llvm/Support/Path.h"
#include "llvm/Support/ConvertUTF.h"
#include "llvm/ADT/SmallVector.h"

#include <chrono>
#include <iostream>
#include <vector>
#include <string>
#include <iomanip>

#if defined(_WIN32)

using namespace std::chrono;

struct BenchmarkResult {
    std::string testName;
    double avgTimePerCall;
    size_t totalCalls;
    long long totalTime;
    bool allSucceeded;
};

BenchmarkResult benchmarkPathType(const std::string& testName, 
                                  const std::vector<std::string>& paths, 
                                  int iterations) {
    BenchmarkResult result;
    result.testName = testName;
    result.totalCalls = iterations * paths.size();
    result.allSucceeded = true;
    
    // Warm up
    for (const auto& path : paths) {
        llvm::SmallVector<wchar_t, 260> widePath;
        llvm::sys::path::widenPath(path, widePath);
    }
    
    // Benchmark
    auto start = high_resolution_clock::now();
    
    for (int i = 0; i < iterations; ++i) {
        for (const auto& path : paths) {
            llvm::SmallVector<wchar_t, 260> widePath;
            std::error_code ec = llvm::sys::path::widenPath(path, widePath);
            if (ec) {
                result.allSucceeded = false;
            }
        }
    }
    
    auto end = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(end - start);
    
    result.totalTime = duration.count();
    result.avgTimePerCall = static_cast<double>(duration.count()) / result.totalCalls;
    
    return result;
}

void printResults(const std::vector<BenchmarkResult>& results) {
    std::cout << "\n" << std::string(80, '=') << "\n";
    std::cout << "widenPath Performance Benchmark Results\n";
    std::cout << std::string(80, '=') << "\n";
    
    std::cout << std::left << std::setw(25) << "Test Case" 
              << std::setw(12) << "Calls" 
              << std::setw(15) << "Total (μs)" 
              << std::setw(15) << "Avg (μs)" 
              << std::setw(10) << "Success" << "\n";
    std::cout << std::string(80, '-') << "\n";
    
    double totalAvg = 0;
    size_t totalCalls = 0;
    
    for (const auto& result : results) {
        std::cout << std::left << std::setw(25) << result.testName
                  << std::setw(12) << result.totalCalls
                  << std::setw(15) << result.totalTime
                  << std::setw(15) << std::fixed << std::setprecision(3) << result.avgTimePerCall
                  << std::setw(10) << (result.allSucceeded ? "✓" : "✗") << "\n";
        
        totalAvg += result.avgTimePerCall * result.totalCalls;
        totalCalls += result.totalCalls;
    }
    
    std::cout << std::string(80, '-') << "\n";
    std::cout << std::left << std::setw(25) << "OVERALL AVERAGE"
              << std::setw(12) << totalCalls
              << std::setw(15) << ""
              << std::setw(15) << std::fixed << std::setprecision(3) << (totalAvg / totalCalls)
              << "\n";
    std::cout << std::string(80, '=') << "\n";
}

int main() {
    const int iterations = 10000;
    std::vector<BenchmarkResult> results;
    
    // Test 1: Short absolute paths (should hit early return optimization)
    {
        std::vector<std::string> shortAbsPaths = {
            "C:\\Windows\\System32\\kernel32.dll",
            "C:\\Program Files\\test.exe",
            "D:\\temp\\file.txt",
            "C:\\Users\\user\\Documents\\file.doc",
            "E:\\projects\\src\\main.cpp"
        };
        results.push_back(benchmarkPathType("Short Absolute", shortAbsPaths, iterations));
    }
    
    // Test 2: Relative paths (require current directory lookup)
    {
        std::vector<std::string> relPaths = {
            "..\\..\\test.txt",
            "src\\main.cpp",
            "build\\debug\\output.exe",
            ".\\current\\file.txt",
            "..\\parent\\sibling.txt"
        };
        results.push_back(benchmarkPathType("Relative Paths", relPaths, iterations));
    }
    
    // Test 3: Long paths (require \\?\ prefix and canonicalization)
    {
        std::vector<std::string> longPaths = {
            "C:\\very\\long\\path\\that\\exceeds\\the\\normal\\MAX_PATH\\limit\\and\\requires\\special\\handling\\with\\the\\long\\path\\prefix\\to\\work\\correctly\\on\\windows\\systems\\file.txt",
            "C:\\another\\extremely\\long\\directory\\structure\\that\\goes\\beyond\\the\\traditional\\260\\character\\limit\\imposed\\by\\older\\windows\\apis\\and\\needs\\extended\\path\\support\\document.docx"
        };
        results.push_back(benchmarkPathType("Long Paths", longPaths, iterations / 10)); // Fewer iterations for expensive operations
    }
    
    // Test 4: Paths with . and .. (require canonicalization)
    {
        std::vector<std::string> complexPaths = {
            "C:\\Windows\\..\\Program Files\\..\\Windows\\System32\\kernel32.dll",
            "..\\src\\..\\build\\..\\src\\main.cpp",
            "C:\\temp\\..\\..\\Windows\\System32\\..\\..\\Program Files\\test.exe",
            ".\\..\\..\\src\\..\\build\\output.exe"
        };
        results.push_back(benchmarkPathType("Complex Paths", complexPaths, iterations / 2));
    }
    
    // Test 5: UNC paths
    {
        std::vector<std::string> uncPaths = {
            "\\\\server\\share\\file.txt",
            "\\\\localhost\\c$\\Windows\\System32\\kernel32.dll",
            "\\\\network\\storage\\documents\\file.doc"
        };
        results.push_back(benchmarkPathType("UNC Paths", uncPaths, iterations));
    }
    
    // Test 6: Already prefixed paths
    {
        std::vector<std::string> prefixedPaths = {
            "\\\\?\\C:\\already\\prefixed\\path.txt",
            "\\\\?\\D:\\another\\prefixed\\file.exe"
        };
        results.push_back(benchmarkPathType("Prefixed Paths", prefixedPaths, iterations));
    }
    
    // Test 7: Mixed workload (realistic usage pattern)
    {
        std::vector<std::string> mixedPaths = {
            "C:\\Windows\\System32\\kernel32.dll",  // Short absolute
            "..\\src\\main.cpp",                    // Relative
            "\\\\server\\share\\file.txt",          // UNC
            "C:\\temp\\..\\Windows\\notepad.exe",   // With dots
            "build\\debug\\output.exe"              // Relative
        };
        results.push_back(benchmarkPathType("Mixed Workload", mixedPaths, iterations));
    }
    
    printResults(results);
    
    // Performance analysis
    std::cout << "\nPerformance Analysis:\n";
    std::cout << "- Short absolute paths should be fastest (early return optimization)\n";
    std::cout << "- Relative paths will be slower (current directory lookup)\n";
    std::cout << "- Long paths will be slowest (canonicalization + prefix)\n";
    std::cout << "- Complex paths with .. require canonicalization\n";
    
    // Check if optimizations are working
    auto shortAbsAvg = results[0].avgTimePerCall;
    auto relativeAvg = results[1].avgTimePerCall;
    
    if (shortAbsAvg < relativeAvg) {
        std::cout << "\n✓ Early return optimization is working (short absolute < relative)\n";
    } else {
        std::cout << "\n⚠ Early return optimization may not be working effectively\n";
    }
    
    return 0;
}

#else
int main() {
    std::cout << "This benchmark is only available on Windows platforms.\n";
    return 0;
}
#endif