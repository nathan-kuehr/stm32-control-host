#ifndef __NEI_DEFINES_H__
#define __NEI_DEFINES_H__

#define NEI_BUFSIZE_256B 0x100
#define NEI_BUFSIZE_512B 0x200
#define NEI_BUFSIZE_1KB  0x400
#define NEI_BUFSIZE_2KB  0x800

#define NEI_TICKDELAY_1S pdMS_TO_TICKS(1000)

/// @brief Swaps a and b
/// @note Avoid to put rvalues and costly evaluations, as parameters could be
/// multiply evaluated
#define swap(a, b)                                                             \
    do {                                                                       \
        typeof(a) _swap_tmp = (a);                                             \
        (a)                 = (b);                                             \
        (b)                 = _swap_tmp;                                       \
    } while (0)

/// @brief Returns the minimum from @p a and @p b
#define min(a, b) ((a) < (b) ? (a) : (b))
/// @brief Returns the maximum from @p a and @p b
#define max(a, b) ((a) > (b) ? (a) : (b))

#endif //__NEI_DEFINES_H__
