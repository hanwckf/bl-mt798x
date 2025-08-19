#ifndef SEC_HACC_EXPORT_H
#define SEC_HACC_EXPORT_H

// ========================================================
// HACC USER
// ========================================================
typedef enum{
    HACC_USER1 = 0,
    HACC_USER2,
    HACC_USER3,
    HACC_USER4
} HACC_USER;

// ========================================================
// SECURE DRIVER INTERNAL HACC FUNCTION
// ========================================================
extern unsigned sp_hacc_dec(unsigned char *buf, unsigned int size, HACC_USER user);
extern unsigned sp_hacc_enc(unsigned char *buf, unsigned int size, HACC_USER user);

#endif

