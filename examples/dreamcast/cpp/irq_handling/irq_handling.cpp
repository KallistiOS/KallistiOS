/* KallistiOS ##version##

   examples/dreamcast/cpp/irq_handling/irq_handling.cpp

   Copyright (C) 2024 Falco Girgis

*/

/*
    This file serves as both an example of and a demonstration for installing
    custom IRQ handlers to do exception handling and propagation using the new
    and updated IRQ chaining mechanism in KOS.

    The example is being written in unapologetically modern C++ to provide
    some useful utilties for installing arbitrary high-level C++ callables like
    stateful lambdas as IRQ handlers, which can easily be warezed by C++
    developers and integrated within their own projects.
*/

#include <concepts>
#include <functional>
#include <type_traits>
#include <print>
#include <format>
#include <string>
#include <cfloat>
#include <cstdlib>
#include <atomic>

#include <kos.h>

// Provide a custom formatter so we can pass irq_t to println(), inheriting
// from a const char* formatter so we can use reuse its implementation.
template <>
struct std::formatter<irq_t>: formatter<const char *> {
    auto format(irq_t &code, format_context &ctx) const {
        // Call into the base class to format the enum like a const char*.
        switch(code) {
            case EXC_FPU:
                return formatter<const char *>::format("EXC_FPU",         ctx);
            case EXC_GENERAL_FPU:
                return formatter<const char *>::format("EXC_GENERAL_FPU", ctx);
            case EXC_SLOT_FPU:
                return formatter<const char *>::format("EXC_SLOT_FPU",    ctx);
            default:
                return formatter<const char *>::format("UNKNOWN",         ctx);
        }
    }
};

// Utility namespace for our generic, reusable IRQ-related C++ utility API.
namespace irq {

    // We have to inherit from the C handler so C++ doesn't whine about
    // ignoring its attributes in a template argument. This is fine as long as
    // we do not add anything to it.
    struct context: public irq_context_t {};

    // Sanity check for our context type.
    static_assert(sizeof(context) == sizeof(irq_context_t),
                  "Could not safely inherit iq_context_t in irq::context!");

    // Create a concept that represents a generalized C++ callable which is
    // capable of being invoked as an interrupt handler.
    template<class F> 
    concept handler = 
        std::invocable<F, irq_t, context *> &&
        std::same_as<std::invoke_result_t<F, irq_t, context *>, bool>;

    // Create a type-erased container that can store any handler concept.
    using erased_handler = std::function<bool(irq_t, context *)>;

    // Anonymous namespace used to prevent our internals from being exported.
    namespace {

        // C function used as the irq_handler callback to the C API, whose job
        // it is to bridge the call to our type-erased C++ callable.
        void cpp_handler_adapter(irq_t code, irq_context_t *ctx, void* data) {
            // Cast the void* data pointer to our std::function<> container
            auto *handler = static_cast<erased_handler *>(data);

            // Call our type-erased std::function container with the given args
            // then use its return value to tell the C API whether it handled
            // the given exception code or not.
            irq_handle_int((*handler)(code, static_cast<context *>(ctx)));
        }
    }

    // Function template for registering a single handler to handle multiple
    // IRQ codes simultaneously.
    template<irq_t... codes>
    bool set_handler(handler auto &&callback) {
        // Convert whatever specific type of callable C++ object we get
        // (function pointer, member function, function object, etc) into a
        // generic, type-erased form we can then pass back to our C callback
        // as a void* userdata.
        auto* erased = 
            new erased_handler(std::forward<decltype(callback)>(callback));

        // Perform parameter pack expansion, registering the same handler to
        // handle each of the given IRQ codes, bitwise AND-ing their return
        // values into a single value.
        return (irq_set_handler(codes, cpp_handler_adapter, erased) & ...);
    }

    // Function for installing a handler as the global IRQ handler using the
    // same mechanism described above for individual IRQ codes.
    bool set_global_handler(handler auto &&callback) {
        auto* erased = 
            new erased_handler(std::forward<decltype(callback)>(callback));
        return irq_set_global_handler(cpp_handler_adapter, erased);
    }
}

// Utility structure which gets captured by our meta handler wrapper.
struct handler_ctrl {
    std::atomic<size_t> called_count  = 0;    // Serves as an IRQ counter.
    std::atomic<bool>   should_handle = true; // Whether to handle the IRQ.
};
 
// Function template for creating a "meta handler" around a given handler,
// allowing us to inject some extra logic into each handler using the
// handler_ctrl struct for this meta data and logic.
template<irq_t... codes>
auto meta_handler(std::string name, handler_ctrl& ctrl, auto &&handler)
    requires std::invocable<std::remove_reference_t<decltype(handler)>,
                            irq_t, irq::context *> 
{
    // Creates a new lambda, capturing the given values, serving as our
    // irq::handler that gets registered.
    return ([&ctrl, name=std::move(name), &handler]
            (irq_t code, irq::context *ctx) 
    {
        // Increment our counter for the number of times the IRQ was called.
        ++ctrl.called_count;
        // Log the exception code and which handler received it.
        std::println("Caught exception: {} from {}!", code, name);
        // Proxy the call to our inner handler which performs the real logic.
        std::invoke(std::forward<decltype(handler)>(handler), code, ctx);
        // Use hanler_ctrl::should_handle as our return value, which determines
        // whether to handle and accept or ignore and reject the interrupt.
        return static_cast<bool>(ctrl.should_handle);
    });
}

// Evil function that will raise a divide-by-zero FPU exception.
static void divide_by_zero_exception() {
    // Enable divide-by-zero exceptions in the FPU status register.
    __builtin_sh_set_fpscr(__builtin_sh_get_fpscr() | 0b111100000000);
    {   // RIP FPU!
        volatile double d = 0.0, c = 0.0;
        volatile double e = d / c;
        (void)e;
    }
}

// Our test scenario: Essentially constructing a chain of IRQ handlers then
// raising IRQs to ensure that they can terminate and propagate properly
// through the entire chain.
int main(int argc, char* argv[]) {
    bool success = true;

    // Control structures for our IRQ handlers
    handler_ctrl fpu_ctrl, unhandled_ctrl, global_ctrl;

    // Install a global IRQ handler which will catch everything.
    irq::set_global_handler(
        meta_handler("Global Handler", global_ctrl, [](irq_t, irq::context *ctx)
            {
                        // Advance to the next instruction to not infinite loop.
                        ctx->pc += 2;
                    }));

    // Install a single handler for EXC_FPU, EXC_GENERAL_FPU, and EXC_SLOT_FPU
    irq::set_handler<EXC_FPU, EXC_GENERAL_FPU, EXC_SLOT_FPU>(
        meta_handler("Single Handler", fpu_ctrl, 
                [](irq_t, irq::context *ctx) {
                ctx->pc += 2;   // Advance to the next instruction.
            }));

    // Install an unhandled exception handler at the end of the chain.
    irq::set_handler<EXC_UNHANDLED_EXC>(
        meta_handler("Unhandled Handler", unhandled_ctrl, [](irq_t, irq::context *ctx) {
            ctx->pc += 2;   // Advance to the next instruction.
        }));
    
    std::println("Testing accepting the exception in GLOBAL handler...");

    // The global exception handler should get invoked first.
    // Flag it to accept the IRQ, so it should terminate there.
    global_ctrl.should_handle = true;

    divide_by_zero_exception();

    // Verify only the global handler got the IRQ and then it terminated.
    if(global_ctrl.called_count    != 1 ||
       fpu_ctrl.called_count       != 0 ||
       unhandled_ctrl.called_count != 0)
    {
        std::println("GLOBAL handler failed to accept exception!");
        success = false;
    }

    std::println("\nTesting accepting the exception in SINGLE handler...");

    // Tell the global handler to ignore it so it propagates onward.
    global_ctrl.should_handle = false;
    // Tell the single/specific FPU handler to accept and terminate it.
    fpu_ctrl.should_handle    = true;

    divide_by_zero_exception();

    // Verify the IRQ propagated global -> single/FPU but terminated after.
    if(global_ctrl.called_count    != 2 ||
       fpu_ctrl.called_count       != 1 ||
       unhandled_ctrl.called_count != 0)
    {
        std::println("SINGLE handler failed to accept exception!");
        success = false;
    }

    std::println("\nTesting accepting the exception in UNHANDLED handler...");

    // Tell the single/specific FPU handler to ignore and propagate the IRQ.
    fpu_ctrl.should_handle       = false;
    // Tell the unhandled IRQ handler to accept and terminate the IRQ
    // Otherwise it'll propage to finally causing a kernal panic and abort!
    unhandled_ctrl.should_handle = true;

    divide_by_zero_exception();

    // Verify the IRQ propagated global -> single/FPU -> unhandled and then
    // terminated before we caused a kernel panic and aborted the program.
    if(global_ctrl.called_count    != 3 ||
       fpu_ctrl.called_count       != 2 ||
       unhandled_ctrl.called_count != 1)
    {
        std::println("UNHANDLED handler failed to accept exception!");
        success = false;
    }
    
    // Print out our results and return with a standard exit code.
    if(success) { 
        std::println(stdout, 
                     "\n========= C++ IRQ HANDLING TEST: PASSED! ==========");
        return EXIT_SUCCESS;
    } else {
        std::println(stderr, 
                     "\n!!!!!!!!! C++ IRQ HANDLING TEST: FAILED! !!!!!!!!!");
        return EXIT_FAILURE;
    }
}
