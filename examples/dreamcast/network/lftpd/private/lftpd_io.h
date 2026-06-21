#pragma once

/**
 * @brief Given a base path and a name, attempts to combine base and
 * name and produce an absolute path. If either base or name are NULL
 * they are treated as empty strings.
 * The following rules are then applied:
 * 1. If name does not begin with / it is appended to base with
 *    / as a separator. In other words, name is treated as relative
 *    to base and the two are combined.
 * 2. The path is then broken into segments by splitting on /.
 * 3. Path segments of . are removed entirely.
 * 4. Path segments of .. are resolved by removing the parent segment.
 * 5. The segments are then joined with / and the result is returned.
 */
char *lftpd_io_canonicalize_path(const char *base, const char *name);
