#include <vector>
#include <iostream>
#include <cstdint>
#include <malloc.h>

#include <kos/init.h>

KOS_INIT_FLAGS(INIT_MALLOCSTATS);

void new_handler_cb() {
    std::cout << "new_handler callback invoked!" << std::endl;

    malloc_stats();

    // Unregister ourself as the new handler, so that next
    // iteration will hit the exception handler.
    std::set_new_handler(NULL);
}

int main(int argc, char **argv) {
    std::vector<uint8_t> bytes;
    bool failed_once = false;

    // Sets the global, static C++ handler for when calls to new fail
    // this can be used without exceptions enabled!
    std::set_new_handler(new_handler_cb);

    std::cout << "Beginning out-of-memory demonstration." << std::endl;

    while(true) {
        try {
            // Just keep adding bytes until something bad happens!
            bytes.push_back(0xff);

        } catch(std::bad_alloc const&) {

            if(!failed_once) {
                // std::bad_alloc is thrown when a call to new fails
                std::cout << "Caught std::bad_alloc! Current size: "
                          << static_cast<double>(bytes.capacity())
                                / 1024.0 / 1024.0 << "MB"
                          << std::endl;

                // Remember, std::vector typically requests RAM in
                // powers-of-two, so the actual requested allocation
                // was probably 2x the size of the vector.

                // Lets force the vector to shrink to free up some
                // space and ensure we can continue to allocate.

                malloc_stats();

                bytes.resize(0);
                bytes.shrink_to_fit();

                failed_once = true;
            } else {
                break;
            }
        }
    }

    std::cout << "All done. Thank you for the RAM!" << std::endl;

    return !failed_once;
}
