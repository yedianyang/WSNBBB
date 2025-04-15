#include <iostream>

int main() {
    // This macro is defined by the compiler to indicate the C++ version
    #if defined(__cplusplus)
        // Output the version of C++
        std::cout << "C++ version: " << __cplusplus << std::endl;
    #else
        // Notify that the version could not be detected
        std::cout << 'C++ version could not be determined.' << std::endl;
    #endif
    
    // This outputs the versions of the GCC or Clang compiler, if applicable
    #ifdef __GNUC__
        std::cout << "GCC version: " << __GNUC__ << '.' << __GNUC_MINOR__ << '.' << __GNUC_PATCHLEVEL__ << std::endl;
    #endif
    
    #ifdef __clang__
        std::cout << 'Clang version: ' << __clang_major__ << '.' << __clang_minor__ << '.' << __clang_patchlevel__ << std::endl;
    #endif
    
    // Output the Microsoft Visual C++ compiler version, if applicable
    #ifdef _MSC_VER
        std::cout << 'MSVC version: ' << _MSC_VER << std::endl;
    #endif

    return 0;
}