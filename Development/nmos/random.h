#ifndef NMOS_RANDOM_H
#define NMOS_RANDOM_H

#include <algorithm>
#include <random>

namespace nmos
{
    namespace details
    {
        // this isn't a proper SeedSequence because two instances of random_device aren't going to produce the same values
        // so the constructors, and size and param member functions are provided only to meet the syntactic requirements
        class seed_generator
        {
        public:
            seed_generator() {}
            template <typename InputIterator> seed_generator(InputIterator first, InputIterator last) {}
            template <typename T> seed_generator(const std::initializer_list<T>&) {}

            template <typename RandomAccessorIterator>
            void generate(const RandomAccessorIterator first, const RandomAccessorIterator last)
            {
                std::uniform_int_distribution<std::uint32_t> uint32s;
                std::generate(first, last, [&] { return uint32s(device); });
            }

            size_t size() const { return 0; }
            template <typename OutputIterator> void param(OutputIterator) {}

        private:
            std::random_device device;
        };
    }
}

#endif
