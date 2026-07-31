// The first "test" is of course whether the header compiles standalone
#include "nmos/filesystem_route.h"

#include "bst/test/test.h"

////////////////////////////////////////////////////////////////////////////////////////////
// Test the lexically_normalize_path function (accessing internal implementation)
namespace nmos
{
    namespace experimental
    {
        namespace details
        {
            // Expose the internal function for testing
            bool lexically_normalize_path(const utility::string_t& relative_path, utility::string_t& normalized_out);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testLexicallyNormalizePathBasicPaths)
{
    utility::string_t normalized;

    // Test: Simple path normalization
    BST_REQUIRE(nmos::experimental::details::lexically_normalize_path(U("/foo/bar"), normalized));
    BST_REQUIRE_EQUAL(U("/foo/bar"), normalized);

    // Test: Root path
    BST_REQUIRE(nmos::experimental::details::lexically_normalize_path(U("/"), normalized));
    BST_REQUIRE_EQUAL(U("/"), normalized);

    // Test: Path with trailing slash
    BST_REQUIRE(nmos::experimental::details::lexically_normalize_path(U("/foo/bar/"), normalized));
    BST_REQUIRE_EQUAL(U("/foo/bar/"), normalized);
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testLexicallyNormalizePathDotSegments)
{
    utility::string_t normalized;

    // Test: Single dot (current directory) - should be removed
    BST_REQUIRE(nmos::experimental::details::lexically_normalize_path(U("/foo/./bar"), normalized));
    BST_REQUIRE_EQUAL(U("/foo/bar"), normalized);

    // Test: Multiple single dots
    BST_REQUIRE(nmos::experimental::details::lexically_normalize_path(U("/foo/./././bar"), normalized));
    BST_REQUIRE_EQUAL(U("/foo/bar"), normalized);

    // Test: Path with only dot segments
    BST_REQUIRE(nmos::experimental::details::lexically_normalize_path(U("/./././"), normalized));
    BST_REQUIRE_EQUAL(U("/"), normalized);
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testLexicallyNormalizePathDoubleDotSegments)
{
    utility::string_t normalized;

    // Test: Normal parent directory navigation
    BST_REQUIRE(nmos::experimental::details::lexically_normalize_path(U("/foo/bar/../baz"), normalized));
    BST_REQUIRE_EQUAL(U("/foo/baz"), normalized);

    // Test: Multiple parent directory navigations
    BST_REQUIRE(nmos::experimental::details::lexically_normalize_path(U("/foo/bar/baz/../../qux"), normalized));
    BST_REQUIRE_EQUAL(U("/foo/qux"), normalized);

    // Test: Navigate to root
    BST_REQUIRE(nmos::experimental::details::lexically_normalize_path(U("/foo/../"), normalized));
    BST_REQUIRE_EQUAL(U("/"), normalized);

    // Test: Complex navigation
    BST_REQUIRE(nmos::experimental::details::lexically_normalize_path(U("/a/b/../c/./d/../e"), normalized));
    BST_REQUIRE_EQUAL(U("/a/c/e"), normalized);
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testLexicallyNormalizePathDirectoryTraversalAttacks)
{
    utility::string_t normalized;

    // Test: Attempt to traverse above root (single level)
    BST_REQUIRE(!nmos::experimental::details::lexically_normalize_path(U("/../secret"), normalized));

    // Test: Attempt to traverse above root (multiple levels)
    BST_REQUIRE(!nmos::experimental::details::lexically_normalize_path(U("/../../etc/passwd"), normalized));

    // Test: Attempt to traverse after valid path
    BST_REQUIRE(!nmos::experimental::details::lexically_normalize_path(U("/foo/../../bar"), normalized));

    // Test: Complex traversal attack
    BST_REQUIRE(!nmos::experimental::details::lexically_normalize_path(U("/a/b/c/../../../../../../../etc/shadow"), normalized));

    // Test: Traversal at the boundary (should fail)
    BST_REQUIRE(!nmos::experimental::details::lexically_normalize_path(U("/../"), normalized));

    // Test: Multiple consecutive parent references
    BST_REQUIRE(!nmos::experimental::details::lexically_normalize_path(U("/../../../"), normalized));
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testLexicallyNormalizePathEdgeCases)
{
    utility::string_t normalized;

    // Test: Empty segments (double slashes)
    BST_REQUIRE(nmos::experimental::details::lexically_normalize_path(U("/foo//bar"), normalized));
    BST_REQUIRE_EQUAL(U("/foo/bar"), normalized);

    // Test: Multiple consecutive slashes
    BST_REQUIRE(nmos::experimental::details::lexically_normalize_path(U("/foo///bar////baz"), normalized));
    BST_REQUIRE_EQUAL(U("/foo/bar/baz"), normalized);

    // Test: Trailing dots and slashes
    BST_REQUIRE(nmos::experimental::details::lexically_normalize_path(U("/foo/bar/./"), normalized));
    BST_REQUIRE_EQUAL(U("/foo/bar/"), normalized);

    // Test: Path with filename
    BST_REQUIRE(nmos::experimental::details::lexically_normalize_path(U("/foo/bar/index.html"), normalized));
    BST_REQUIRE_EQUAL(U("/foo/bar/index.html"), normalized);

    // Test: Path with file extension containing dots
    BST_REQUIRE(nmos::experimental::details::lexically_normalize_path(U("/foo/file.min.js"), normalized));
    BST_REQUIRE_EQUAL(U("/foo/file.min.js"), normalized);
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testLexicallyNormalizePathUnicodeAndSpecialCharacters)
{
    utility::string_t normalized;

    // Test: Paths with spaces (after URL decode)
    BST_REQUIRE(nmos::experimental::details::lexically_normalize_path(U("/my folder/my file.txt"), normalized));
    BST_REQUIRE_EQUAL(U("/my folder/my file.txt"), normalized);

    // Test: Paths with hyphens and underscores
    BST_REQUIRE(nmos::experimental::details::lexically_normalize_path(U("/my-folder/my_file.txt"), normalized));
    BST_REQUIRE_EQUAL(U("/my-folder/my_file.txt"), normalized);

    // Test: Paths with numbers
    BST_REQUIRE(nmos::experimental::details::lexically_normalize_path(U("/folder123/file456.txt"), normalized));
    BST_REQUIRE_EQUAL(U("/folder123/file456.txt"), normalized);

    // Test: Paths with mixed case
    BST_REQUIRE(nmos::experimental::details::lexically_normalize_path(U("/MyFolder/MyFile.TXT"), normalized));
    BST_REQUIRE_EQUAL(U("/MyFolder/MyFile.TXT"), normalized);
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testURLEncodedBackslashAttackVector)
{
    // Simulate the complete attack vector: %5C..%5C..%5Csecret.txt

    // Step 1: After URL decode, %5C becomes `\`
    utility::string_t decoded_path = U("\\..\\..\\secret.json");

    // Step 2: Normalize backslashes to forward slashes (as done in make_filesystem_route)
    std::replace(decoded_path.begin(), decoded_path.end(), U('\\'), U('/'));
    BST_REQUIRE_EQUAL(U("/../../secret.json"), decoded_path);

    // Step 3: Lexical normalization should REJECT this
    utility::string_t normalized;
    BST_REQUIRE(!nmos::experimental::details::lexically_normalize_path(decoded_path, normalized));
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testComplexMixedAttackVectors)
{
    utility::string_t normalized;

    // Test: Mixed forward and backward slashes (after conversion to forward)
    {
        utility::string_t mixed = U("/foo\\bar/../baz");
        std::replace(mixed.begin(), mixed.end(), U('\\'), U('/'));
        BST_REQUIRE(nmos::experimental::details::lexically_normalize_path(mixed, normalized));
        BST_REQUIRE_EQUAL(U("/foo/baz"), normalized);
    }

    // Test: Complex attack with multiple techniques
    {
        utility::string_t attack = U("/foo\\..\\..\\..\\secret");
        std::replace(attack.begin(), attack.end(), U('\\'), U('/'));
        BST_REQUIRE(!nmos::experimental::details::lexically_normalize_path(attack, normalized));
    }

    // Test: URL-encoded double dot with backslash
    {
        utility::string_t attack = U("\\..\\admin\\config");
        std::replace(attack.begin(), attack.end(), U('\\'), U('/'));
        BST_REQUIRE(!nmos::experimental::details::lexically_normalize_path(attack, normalized));
    }
}
