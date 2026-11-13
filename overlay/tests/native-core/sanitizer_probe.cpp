/*
 * P710 native notice preparation: tests/native-core/sanitizer_probe.cpp
 * New candidate test/build/documentation/metadata path.
 * Native adaptation recorded in the 2026-11-12 checkpoint.
 * Exact first-edit/creation date is not established; this is a checkpoint-record date.
 * Scope: add intentional diagnostic-only ASan/UBSan canaries.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Notice inserted: 2026-11-12.
 * Existing source bytes and original notices retained; no additional rights are granted.
 * This record does not establish license applicability, compliance or distribution permission.
 */

#include <cstring>
#include <limits>

// Intentionally invalid operations in this diagnostic executable only.
// CTest accepts them only when the expected sanitizer diagnostic is present.
int main(int argc, char** argv)
{
    if (argc != 2) return 2;
    if (std::strcmp(argv[1], "asan") == 0)
    {
        auto* bytes = new unsigned char[8];
        volatile int offset = argc + 7;
        bytes[offset] = 42;
        delete[] bytes;
        return 0;
    }
    if (std::strcmp(argv[1], "ubsan") == 0)
    {
        volatile int maximum = std::numeric_limits<int>::max();
        volatile int one = 1;
        volatile int result = maximum + one;
        (void)result;
        return 0;
    }
    return 2;
}
