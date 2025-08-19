
/* KallistiOS ##version##

   include/kos/version.h
   Copyright (C) 2024 Falco Girgis
   Copyright (C) 2024 Donald Haase
   Copyright (C) 2024 Luke Benstead

*/

/** \file
    \brief API versioning and requirements checks.
    \ingroup version

    This file contains the current KOS version information as well as utilities
    for enforcing and checking for certain version ranges.

    \author Falco Girgis
    \author Donald Haase
    \author Luke Benstead
*/

#ifndef __KOS_VERSION_H
#define __KOS_VERSION_H

/** \defgroup version Versioning
    \brief            KOS version information and utility API

    This API provides both access to the current KOS version as well as
    utilities that can be used to check for or require a particular version.

    ## Format
    KOS's versioning scheme follows the following format: `major.minor.patch`
    where a change in the revision number of any component typically suggests:

    |Component|Description
    |---------|-----------
    |Major    |Massive, sweeping changes to major APIs and subsystems.
    |Minor    |Small, incremental updates and new features.
    |Patch    |Usually simply bugfixes.

    ## Version Types
    The versioning information is split into two different groups:

    |Version Type|Description
    |------------|-----------
    |\ref version_comptime|The version of KOS your code is being compiled against.
    |\ref version_runtime |The version of KOS your code has been linked against.

    Ideally, these two versions would be the same; however, it is possible for
    them to differ in certain circumstances:

    - You pulled down a new version of KOS, rebuilt your code, but did not
        rebuild the KOS library.
    - You use dynamically loaded libraries which may have been built
        against different versions of KOS than what you are currently
        linking against.

    ## Version Checking
    Version checks are handled differently depending on whether you want to use
    the version at compile-time or run-time.

    |Version Type|Mechanism
    |------------|-----------
    |\ref version_comptime_check "Compile-Time"|Preprocessor directives, conditional compilation.
    |\ref version_runtime_check "Run-Time"|if statements, conditional branches

    \warning
    It is very important that you use the provided version-check mechanisms when
    comparing two different versions together, as no assurance is made that
    versions can be correctly compared as integers.

    ## App Versioning
    The same \ref version_utils used to implement the KOS versioning
    API are available as part of the public API so that they may be used to
    implement your own similar versioning scheme at the app-level.

    @{
*/

/** \defgroup version_comptime  Compile-Time
    \brief    API providing compile-time KOS version and utilties.

    This API is is specifically for the compile-time versioning. As such,
    its API is implemented via C macros, which can easily be used with the
    preprocessor for conditional compilation.

    @{
*/

/** \defgroup version_comptime_current Current
    \brief    Current compile-time version of KOS

    These macros provide information about the current version of KOS at
    compile-time.

    @{
*/

#define KOS_VERSION_MAJOR   2   /**< KOS's current major revision number. */
#define KOS_VERSION_MINOR   2   /**< KOS's current minor revision number. */
#define KOS_VERSION_PATCH   1   /**< KOS's current patch revision number. */

/** KOS's current version as an integer ID. */
#define KOS_VERSION \
    KOS_VERSION_MAKE(KOS_VERSION_MAJOR, KOS_VERSION_MINOR, KOS_VERSION_PATCH)

/** KOS's current version as a string literal. */
#define KOS_VERSION_STRING \
    KOS_VERSION_MAKE_STRING(KOS_VERSION_MAJOR, \
                            KOS_VERSION_MINOR, \
                            KOS_VERSION_PATCH)
/** @} */

/** \defgroup version_comptime_check Checks
    \brief    Compile-time checks against KOS's current version.

    This API provides several utility macros to check for a particular
    exact, min, or max compile-time version for KOS.

    They are meant to be used with the preprocessor like so:

    \code{.c}
        #if KOS_VERSION_MIN(2, 0, 0)
            // Do something requiring at least KOS 2.0.0 to compile.
        #elif KOS_VERSION_BELOW(2, 5, 1)
            // Do something that was deprecated in KOS 2.5.1.
        #elif KOS_VERSION_IS(3, 1, 2)
            // Do something for an exact version match.
        #endif
    \endcode

    @{
*/

/** Compile-time check for being above a given KOS version.

    Checks to see whether the current KOS version is higher than the given
    version.

    \param  major    Major version component.
    \param  minor    Minor version component.
    \param  patch    Patch version component.

    \retval true     KOS's version is higher.
    \retval false    KOS's version is the same or lower.

*/
#define KOS_VERSION_ABOVE(major, minor, patch) \
    KOS_VERSION_MAKE_ABOVE(major, minor, patch, KOS_VERSION)

/** Compile-time check for a minimum KOS version.

    Checks to see whether the current KOS version is the same or higher than
    the given version.

    \param  major    Major version component.
    \param  minor    Minor version component.
    \param  patch    Patch version component.

    \retval true     KOS's version is the same or higher.
    \retval false    KOS's version is lower.

*/
#define KOS_VERSION_MIN(major, minor, patch) \
    KOS_VERSION_MAKE_MIN(major, minor, patch, KOS_VERSION)

/** Compile-time check for an exact KOS version.

    Checks to see whether the current KOS version matches the given version.

    \param  major    Major version component.
    \param  minor    Minor version component.
    \param  patch    Patch version component.

    \retval true     KOS's version is the same.
    \retval false    KOS's version is different.
*/
#define KOS_VERSION_IS(major, minor, patch) \
    KOS_VERSION_MAKE_IS(major, minor, patch, KOS_VERSION)

/** Compile-time check for a maximum KOS version.

    Checks to see whether the current KOS version is the same or lower than the
    given version.

    \param  major    Major version component.
    \param  minor    Minor version component.
    \param  patch    Patch version component.

    \retval true     KOS's version is the the same or lower.
    \retval false    KOS's version is higher.
*/
#define KOS_VERSION_MAX(major, minor, patch) \
    KOS_VERSION_MAKE_MAX(major, minor, patch, KOS_VERSION)

/** Compile-time check for being below a given KOS version.

    Checks to see whether the current KOS version is lower than the given
    version.

    \param  major    Major version component.
    \param  minor    Minor version component.
    \param  patch    Patch version component.

    \retval true     KOS's version is lower.
    \retval false    KOS's version is the same or higher.

*/
#define KOS_VERSION_BELOW(major, minor, patch) \
    KOS_VERSION_MAKE_BELOW(major, minor, patch, KOS_VERSION)

/** @} */

/** @} */

/** \defgroup version_utils Utilities
    \brief    Utilities for creating version info and checks.

    These are generalized utilities for construction of version info at
    compile-time. They are what KOS uses internally, but they can also
    be used externally to generate version info and version check utilities
    for your application.

    \note
    The ranges for the components of a version ID are as follows:

    |Component|Bits|Range
    |---------|----|------
    |Major    |8   |0-255
    |Minor    |8   |0-255
    |Patch    |8   |0-255

    @{
*/

/** \name  Version Encoding
    \brief Utilities for encoding a version from its components.
    @{
*/
/** Creates a version identifier from its constituents.

    Used to create a version identifier at compile-time from its components.

    \param major    Major version component.
    \param minor    Minor version component.
    \param patch    Patch version component.

    \returns        Packed version identifier.
*/
#define KOS_VERSION_MAKE(major, minor, patch) \
    (((major) << 16) | ((minor) << 8) | (patch))

/** Creates a version string from its constituents.

    Used to create a compile-time string literal from version components.

    \param major    Major version component.
    \param minor    Minor version component.
    \param patch    patch versoin component.

    \returns        `NULL`-terminated C string literal in the format
                    `major.minor.patch`
*/
#define KOS_VERSION_MAKE_STRING(major, minor, patch) \
    __stringify(major) "." \
    __stringify(minor) "." \
    __stringify(patch)
/** @} */

/** \name  Version Checking
    \brief Utilities for creating version checks.
    @{
*/
/** Creates a generic check against a given version.

    Bottom-level macro used to create all compile-time comparison checks against
    other versions.

    \param major    Major version component.
    \param minor    Minor version component.
    \param patch    Patch version component.
    \param op       Integer comparison operator (`<=`, `>=`, `!=`, `==`, etc)/
    \param version  Encoded version to compare against.

    \returns        Boolean value for the given version compared against
                    \p version using \p op.
*/
#define KOS_VERSION_MAKE_COMPARISON(major, minor, patch, op, version) \
    (KOS_VERSION_MAKE(major, minor, patch) op (version & 0xffffff))

/** Creates a check for being above a given version.

    Used to create a compile-time greater-than check against another version.

    \note
    Simply wrap this in a function to implement a runtime check.

    \param major    Major version component.
    \param minor    Minor version component.
    \param patch    Patch version component.
    \param version  The reference version ID.

    \retval true    The given version is above \p version.
    \retval false   The given version is at or below \p version.
*/
#define KOS_VERSION_MAKE_ABOVE(major, minor, patch, version) \
    (KOS_VERSION_MAKE_COMPARISON(major, minor, patch, <, version))

/** Creates a minimum version check.

    Used to create a compile-time minimum version check.

    \note
    Simply wrap this in a function to implement a runtime check.

    \param major    Major version component.
    \param minor    Minor version component.
    \param patch    Patch version component.
    \param version  The minimum version ID.

    \retval true    The given version is at or above \p version.
    \retval false   The given version is below \p version.
*/
#define KOS_VERSION_MAKE_MIN(major, minor, patch, version) \
    (KOS_VERSION_MAKE_COMPARISON(major, minor, patch, <=, version))

/** Creates an exact version check.

    Used to create a compile-time exact version check.

    \note
    Simply wrap this in a function to implement a runtime check.

    \param major    Major version component.
    \param minor    Minor version component.
    \param patch    Patch version component.
    \param version  The exact version ID to match.

    \retval true    The given version matches \p version exactly.
    \retval false   The given version does not match \p version.
*/
#define KOS_VERSION_MAKE_IS(major, minor, patch, version) \
    (KOS_VERSION_MAKE_COMPARISON(major, minor, patch, ==, version))

/** Creates a maximum version check.

    Used to create a compile-time maximum version check.

    \note
    Simply wrap this in a function to implement a runtime check.

    \param major    Major version component.
    \param minor    Minor version component.
    \param patch    Patch version component.
    \param version  The maximum version ID.

    \retval true    The given version is at or below \p version.
    \retval false   The given version is above \p version.

*/
#define KOS_VERSION_MAKE_MAX(major, minor, patch, version) \
    (KOS_VERSION_MAKE_COMPARISON(major, minor, patch, >=, version))

/** Creates a check for being below a given version.

    Used to create a compile-time less-than check against another version.

    \note
    Simply wrap this in a function to implement a runtime check.

    \param major    Major version component.
    \param minor    Minor version component.
    \param patch    Patch version component.
    \param version  The reference version ID.

    \retval true    The given version is below \p version.
    \retval false   The given version is at or above \p version.
*/
#define KOS_VERSION_MAKE_BELOW(major, minor, patch, version) \
    (KOS_VERSION_MAKE_COMPARISON(major, minor, patch, >, version))
/** @} */

/** @} */

#include <kos/cdefs.h>
__BEGIN_DECLS

#include <stdint.h>
#include <stdbool.h>

/** \defgroup version_runtime  Run-Time
    \brief    API providing run-time KOS version and utilties.

    This API is is specifically for the run-time versioning. As such this
    API operates on the KOS version that was linked against and is not suited
    for use at the preprocessor level.

    @{
*/

/** Type of a KOS version identifier.

    This identifier packs the 3 version components into a single opaque ID.

    \warning
    It is not safe to compare two different versions together as if they were
    regular integral types. You must use the Run-time
    \ref version_runtime_check API.
*/
typedef uint32_t kos_version_t;

/** \defgroup version_runtime_current Current
    \brief    Current run-time version of KOS

    These functions provide information about the current version of KOS at
    run-time (ie the version you have linked against).

    @{
*/

/** Returns the current KOS version ID at run-time.

    This function is used to fetch the current KOS version ID at run-time,
    meaning it will return the version of KOS you have \e linked to, rather
    than the one you were necessarily compiling against.

    \returns     KOS's current version identifier
*/
kos_version_t kos_version(void);

/** Returns the string representation of the current KOS version at run-time

    This function fetches the current run-time version of KOS as a string.

    \note
    The string is a NULL-terminated character array in the format:
    `major.minor.patch`.

    \returns     KOS's current version as a printable string.
*/
const char* kos_version_string(void);

/** @} */

/** \defgroup version_runtime_check Checks
    \brief    Run-time checks against KOS's current version.

    This API provides several utility functions to check for a particular
    exact, min, or max run-time version for KOS.

    They are meant to be used as conditional expressions as such:

    \code{.c}
        if(kos_version_min(2, 0, 0))
            // Do something requiring at least KOS 2.0.0 to compile.
        else if(kos_version_below(2, 5, 1))
            // Do something that was deprecated in KOS 2.5.1.
        else if(kos_version_is(3, 1, 2))
            // Do something for an exact version match.
    \endcode

    @{
*/

/** Above version run-time check for KOS.

    Check whether the current run-time version of KOS is above the
    given version.

    \param major    Major version component.
    \param minor    Minor version component.
    \param patch    Patch version component.

    \retval true    KOS is above the given version.
    \retval false   KOS is at or below the given version.
*/
bool kos_version_above(uint8_t major, uint16_t minor, uint8_t patch);

/** Minimum version run-time check for KOS

    Check whether the current run-time version of KOS is at least the
    given version.

    \param major    Major version component.
    \param minor    Minor version component.
    \param patch    patch version component.

    \retval true    KOS is at or above the minimum version.
    \retval false   KOS is below teh minimum version.
*/
bool kos_version_min(uint8_t major, uint16_t minor, uint8_t patch);

/** Exact version run-time check for KOS

    Checks whether the current run-time version of KOS matches the given
    version.

    \param major    Major version component.
    \param minor    Minor version component.
    \param patch    Patch version component.

    \retval true    The version matches exactly.
    \retval false   The version does not match.
*/
bool kos_version_is(uint8_t major, uint16_t minor, uint8_t patch);

/** Maximum version run-time check for KOS

    Checks whether the current run-time version of KOS is at most the
    given version.

    \param major    Major version component.
    \param minor    Minor version component.
    \param patch    Patch version component.

    \retval true    KOS is at or below the maximum version.
    \retval false   KOS is above the maximum version.

*/
bool kos_version_max(uint8_t major, uint16_t minor, uint8_t patch);

/** Below version run-time check for KOS

    Checks whether the current run-time version of KOS is below the given
    version.

    \param major    Major version component.
    \param minor    Minor version component.
    \param patch    Patch version component.

    \retval true    KOS is below the given version.
    \retval false   KOS is at or above the given version.

*/
bool kos_version_below(uint8_t major, uint16_t minor, uint8_t patch);

/** @} */

/** @} */

/** @} */

__END_DECLS

#endif /* __KOS_VERSION_H */
