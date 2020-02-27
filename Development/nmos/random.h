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
            typedef std::uint32_t result_type;

            seed_generator() {}
            template <typename InputIterator> seed_generator(InputIterator first, InputIterator last) {}
            seed_generator(std::initializer_list<result_type>) {}

            template <typename RandomAccessorIterator>
            void generate(const RandomAccessorIterator first, const RandomAccessorIterator last)
            {
                std::uniform_int_distribution<result_type> results;
                std::generate(first, last, [&] { return results(device); });
            }

            size_t size() const { return 0; }
            template <typename OutputIterator> void param(OutputIterator) const {}

        private:
            std::random_device device;
        };
    }
}

#endif
