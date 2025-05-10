// THIS IS AN UGLY HACK TO ACCOUNT FOR THE LACK OF BEING
// ABLE TO COMPILE PHOBOS FOR THIS PLATFORM.
module gcc.config;

// Map from thread model to thread interface.
enum ThreadModel {
    Single,
    Posix,
    Win32,
}

enum GNU_ARM_EABI_Unwinder = false;
enum ThreadModel GNU_Thread_Model = ThreadModel.Posix;
enum OS_Have_Dlpi_Tls_Modid = false;
enum GNU_Have_Atomics = true;
enum GNU_Have_64Bit_Atomics = false;
enum GNU_Have_LibAtomic = false;
enum Have_Qsort_R = false;
enum GNU_Enable_CET = false;
