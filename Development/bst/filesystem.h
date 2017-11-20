#ifndef BST_FILESYSTEM_H
#define BST_FILESYSTEM_H

// Provide bst::filesystem::path, etc. using either std:: or boost:: symbols

#if !defined(BST_FILESYSTEM_STD) && !defined(BST_FILESYSTEM_STD_EXPERIMENTAL) && !defined(BST_FILESYSTEM_MICROSOFT_TR2) && !defined(BST_FILESYSTEM_BOOST)

#if defined(__GNUC__)

#if __GNUC__ > 5 || (__GNUC__ == 5 && __GNUC_MINOR__ >= 3)
#define BST_FILESYSTEM_STD_EXPERIMENTAL
#else
#define BST_FILESYSTEM_BOOST
#endif

#elif defined(_MSC_VER)

#if _MSC_VER >= 1900
#define BST_FILESYSTEM_STD_EXPERIMENTAL
#else
#define BST_FILESYSTEM_MICROSOFT_TR2
#endif

#else

// Default to C++17
#define BST_FILESYSTEM_STD

#endif

#endif

#if defined(BST_FILESYSTEM_STD)

#include <filesystem>
namespace bst_filesystem = std::filesystem;

#elif defined(BST_FILESYSTEM_STD_EXPERIMENTAL)

#include <experimental/filesystem>
namespace bst_filesystem = std::experimental::filesystem;

#elif defined(BST_FILESYSTEM_MICROSOFT_TR2)

#include <filesystem>
namespace bst_filesystem = std::tr2::sys;

#elif defined(BST_FILESYSTEM_BOOST)

#include <boost/filesystem.hpp>
namespace bst_filesystem = boost::filesystem;

#endif

namespace bst
{
    namespace filesystem
    {
        // Subset of symbols that can be used across all the possible implementations

        // Note that with older implementations, e.g. Microsoft TR2 (and Boost.Filesystem v2), path does not have constructors and string() member functions that convert between the source/string character type and the native encoding
        using bst_filesystem::path;
        using bst_filesystem::filesystem_error;
        using bst_filesystem::directory_entry;
        using bst_filesystem::directory_iterator;
        using bst_filesystem::recursive_directory_iterator;
        using bst_filesystem::exists;
        using bst_filesystem::is_directory;
        using bst_filesystem::file_size;
        using bst_filesystem::create_directory;
        using bst_filesystem::remove_all;
    }
}

#endif
