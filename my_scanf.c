// Naomi Beck
#include <stdio.h>
#include <stdarg.h>  // for variadic fucntions: va_list, va_start
#include <ctype.h>  // for isspace()...
#include "my_scanf.h"

/* -----------------------------
Spec struct and parse_spec
----------------------------- */

typedef enum {
    LEN_NONE,
    LEN_H,
    LEN_L,
    LEN_LL,
    LEN_CAP_L
} Length;

typedef struct {
    int width;      // 0 means “no width specified”
    Length len;     // h, l, ll, L
    char conv;      // 'd','s','c','x','f'

} Spec;

static int parse_spec(const char **pp, Spec *out) {
// static int parse_spec(const char **pp, Spec *out) {
    const char *p = *pp;

    out->width = 0;
    out->len = LEN_NONE;
    out->conv = '\0';

    // 1) width: one or more digits
    while (*p && isdigit((unsigned char)*p)) {
        out->width = out->width * 10 + (*p - '0');
        p++;
    }

    // 2) length modifier (NOT DONE)
    if (*p == 'h') {
        out->len = LEN_H;
        p++;
    } else if (*p == 'l') {
        p++;
        if (*p == 'l') {
            out->len = LEN_LL;
            p++;
        } else {
            out->len = LEN_L;
        }
    } else if (*p == 'L') {
        out->len = LEN_CAP_L;
        p++;
    }

    // 3) conversion character must exist
    if (*p == '\0') return 0;
    out->conv = *p;
    p++;

    *pp = p;   // advance format pointer past the specifier
    return 1;
}


/* -----------------------------
helper functions
----------------------------- */
#define UNREAD_MAX 16
static int ubuf[UNREAD_MAX];
static int ubuf_len = 0;

static int nextch(void) {
    if (ubuf_len > 0) {
        return ubuf[--ubuf_len];
    }
    return getchar();
}

static void unreadch(int c) {
    if (c == EOF) return;
    if (ubuf_len < UNREAD_MAX) {
        ubuf[ubuf_len++] = c;
    }
}

static void skip_input_ws(void) {
int c;
while ((c = nextch()) != EOF) {
    if (!isspace((unsigned char)c)) {
        unreadch(c);
        return;
    }
}
}


// scan_ helper functions

// Read one character from input and store it in the variable the user passed in
static int scan_c(const Spec *sp, va_list *ap) {
    int n = (sp->width == 0) ? 1 : sp->width;

    char *out = va_arg(*ap, char*);

    for (int i = 0; i < n; i++) {
        int c = nextch();
        if (c == EOF) return 0;
        out[i] = (char)c;
    }

    /* Custom extension: null-terminate %Nc buffers for convenience */
    out[n] = '\0';

    return 1;
    
}


static int scan_s(const Spec *sp, va_list *ap) {
    // %s skips leading whitespace
    skip_input_ws();

    char *out = va_arg(*ap, char*);
    int i = 0;
    int limit = sp->width;   // 0 means “no limit”

    int c = nextch();
    if (c == EOF) return 0;

    // If the next character is whitespace (or EOF), %s fails
    if (isspace((unsigned char)c)) {
        unreadch(c);
        return 0;
    }

    // Read until whitespace or EOF
    while (c != EOF && !isspace((unsigned char)c)) {
        if (limit != 0 && i >= limit) {
            unreadch(c);
            break;
        }
        out[i++] = (char)c;
        c = nextch();
    }


    // Put back the delimiter (whitespace) so the next conversion can see it
    if (c != EOF && isspace((unsigned char)c)) unreadch(c);

    out[i] = '\0';
    return 1;
}


static int scan_d(const Spec *sp, va_list *ap) {
    // %d skips leading whitespace
    skip_input_ws();

    int limit = sp->width;   // 0 = no limit
    int used = 0;

    int c = nextch();
    if (c == EOF) return 0;

    int sign = 1;

    // optional sign
    if (c == '+' || c == '-') {
        if (c == '-') sign = -1;
        c = nextch();
        if (c == EOF) return 0;
    }

    // must have at least one digit
    if (!isdigit((unsigned char)c)) {
        unreadch(c);
        return 0;
    }

    long value = 0; // use long internally to reduce overflow risk while building

    while (c != EOF && isdigit((unsigned char)c)) {
        if (limit != 0 && used >= limit) {
                unreadch(c);
                break;
            }
        value = value * 10 + (c - '0');
        used++;
        c = nextch();
    }

    // we've read one char too far (non-digit or EOF)
    if (c != EOF && !(limit != 0 && used >= limit)) unreadch(c);

    long signed_value = sign * value;

    switch (sp->len) {
        case LEN_NONE: {
            int *out = va_arg(*ap, int*);
            *out = (int)signed_value;
            break;
        }
        case LEN_L: {
            long *out = va_arg(*ap, long*);
            *out = (long)signed_value;
            break;
        }
        case LEN_LL: {
            long long *out = va_arg(*ap, long long*);
            *out = (long long)signed_value;
            break;
        }
        default:
            // unsupported length for %d
            return 0;
    }


    return 1;
}


static int hex_value(int c) {
    if ('0' <= c && c <= '9') return c - '0';
    if ('a' <= c && c <= 'f') return 10 + (c - 'a');
    if ('A' <= c && c <= 'F') return 10 + (c - 'A');
    return -1;
}

static int scan_x(const Spec *sp, va_list *ap) {
    // %x skips leading whitespace
    skip_input_ws();

    int limit = sp->width;  // 0 = no limit
    int used = 0;

    int c = nextch();
    if (c == EOF) return 0;

    // Optional 0x / 0X prefix
    if (c == '0') {
        if (limit == 0 || used < limit) {
            int c2 = nextch();
            if (c2 == 'x' || c2 == 'X') {
                used += 2;  // consumed '0' and 'x'
                if (limit != 0 && used > limit) {
                    // prefix exceeded width: treat as failure, push back
                    unreadch(c2);
                    unreadch(c);
                    return 0;
                }
                c = nextch();
                if (c == EOF) return 0;
            } else {
                unreadch(c2);
            }
        }
    }

    // Must have at least one hex digit
    int hv = hex_value(c);
    if (hv < 0) {
        unreadch(c);
        return 0;
    }

    unsigned long value = 0;

    while (c != EOF && (hv = hex_value(c)) >= 0) {
        if (limit != 0 && used >= limit) {
            unreadch(c);
            break;
        }
        value = value * 16 + (unsigned long)hv;
        used++;
        c = nextch();
    }

    if (c != EOF && hv < 0) unreadch(c);

    switch (sp->len) {
        case LEN_NONE: {
            unsigned int *out = va_arg(*ap, unsigned int*);
            *out = (unsigned int)value;
            break;
        }
        case LEN_L: {
            unsigned long *out = va_arg(*ap, unsigned long*);
            *out = (unsigned long)value;
            break;
        }
        case LEN_LL: {
            unsigned long long *out = va_arg(*ap, unsigned long long*);
            *out = (unsigned long long)value;
            break;
        }
        default:
            return 0;
    }

    return 1;
}

static long double pow10_ld(int exp) {
    long double p = 1.0L;
    if (exp >= 0) {
        for (int i = 0; i < exp; i++) p *= 10.0L;
    } else {
        for (int i = 0; i < -exp; i++) p /= 10.0L;
    }
    return p;
}

//static int scan_f(const Spec *sp, va_list *ap) {
static int scan_f(const Spec *sp, void *outp) {
    skip_input_ws();

    int limit = sp->width;   // 0 = no limit
    int used = 0;

    #define READC() ((limit!=0 && used>=limit) ? EOF : (used++, nextch()))
    #define UNRDC(ch) do { if ((ch)!=EOF) { unreadch(ch); used--; } } while(0)

    int c = READC();
    if (c == EOF) return 0;

    int sign = 1;
    if (c == '+' || c == '-') {
        if (c == '-') sign = -1;
        c = READC();
        if (c == EOF) return 0;
    }

    int saw_digit = 0;
    long double val = 0.0L;

    while (c != EOF && isdigit((unsigned char)c)) {
        saw_digit = 1;
        val = val * 10.0L + (long double)(c - '0');
        c = READC();
    }

    if (c == '.') {
        long double place = 0.1L;
        c = READC();
        while (c != EOF && isdigit((unsigned char)c)) {
            saw_digit = 1;
            val += (long double)(c - '0') * place;
            place *= 0.1L;
            c = READC();
        }
    }

    if (!saw_digit) {
        UNRDC(c);
        return 0;
    }

    if (c == 'e' || c == 'E') {
        int e_char = c;
        int exp_sign = 1;
        int exp_val = 0;

        int c2 = READC();
        if (c2 == '+' || c2 == '-') {
            if (c2 == '-') exp_sign = -1;
            int c3 = READC();
            if (c3 == EOF || !isdigit((unsigned char)c3)) {
                UNRDC(c3);
                UNRDC(c2);
                UNRDC(e_char);
            } else {
                exp_val = c3 - '0';
                int cx = READC();
                while (cx != EOF && isdigit((unsigned char)cx)) {
                    exp_val = exp_val * 10 + (cx - '0');
                    cx = READC();
                }
                UNRDC(cx);
                val *= pow10_ld(exp_sign * exp_val);
                c = READC();
            }
        } else if (c2 == EOF || !isdigit((unsigned char)c2)) {
            UNRDC(c2);
            UNRDC(e_char);
        } else {
            exp_val = c2 - '0';
            int cx = READC();
            while (cx != EOF && isdigit((unsigned char)cx)) {
                exp_val = exp_val * 10 + (cx - '0');
                cx = READC();
            }
            UNRDC(cx);
            val *= pow10_ld(exp_sign * exp_val);
            c = READC();
        }
    }

    UNRDC(c);

   val *= (long double)sign;

    // if (sp->len == LEN_NONE) {          // %f
    //     float *out = va_arg(*ap, float*);
    //     *out = (float)val;
    // } else if (sp->len == LEN_L) {      // %lf
    //     double *out = va_arg(*ap, double*);
    //     *out = (double)val;
    // } else if (sp->len == LEN_CAP_L) {  // %Lf
    //     long double *out = va_arg(*ap, long double*);
    //     *out = (long double)val;
    // } else {
    //     #undef READC
    //     #undef UNRDC
    //     return 0; // (h / ll not supported for %f)
    // }

    if (sp->len == LEN_NONE) {          // %f
        *(float*)outp = (float)val;
    } else if (sp->len == LEN_L) {      // %lf
        *(double*)outp = (double)val;
    } else if (sp->len == LEN_CAP_L) {  // %Lf
        *(long double*)outp = (long double)val;
    } else {
        #undef READC
        #undef UNRDC
        return 0;
    }
    
    #undef READC
    #undef UNRDC
    return 1;
}


/* -----------------------------
my_scanf: dispatcher
Returns number of successful assignments.
----------------------------- */
int my_scanf(const char *fmt, ...) {
va_list ap;
va_start(ap, fmt);

int assigned = 0;
const char *p = fmt;

while (*p) {
    if (*p == '%') {
        p++; // move past '%'

        if (*p == '%') {
            int c = nextch();
            if (c != '%') {
                if (c != EOF) unreadch(c);
                break;
            }
            p++;
            continue;
        }

        Spec sp;
        if (!parse_spec(&p, &sp)) break;

        printf("DEBUG: conv=%c width=%d len=%d\n", sp.conv, sp.width, sp.len);

        int ok = 0;
        switch (sp.conv) {
            case 'c': ok = scan_c(&sp, &ap); break;
            case 's': ok = scan_s(&sp, &ap); break;
            case 'd': ok = scan_d(&sp, &ap); break;
            case 'x': ok = scan_x(&sp, &ap); break;
            //case 'f': ok = scan_f(&sp, &ap); break;
            case 'f': {
                void *dst;
                if (sp.len == LEN_NONE) dst = va_arg(ap, float*);
                else if (sp.len == LEN_L) dst = va_arg(ap, double*);
                else if (sp.len == LEN_CAP_L) dst = va_arg(ap, long double*);
                else { ok = 0; break; }
                ok = scan_f(&sp, dst);
                break;
            }
            default:  ok = 0; break;
        }

        if (!ok) break;
        assigned++;

    } else if (isspace((unsigned char)*p)) {
        while (*p && isspace((unsigned char)*p)) p++;
        skip_input_ws();

    } else {
        int c = nextch();
        if (c != (unsigned char)*p) {
            if (c != EOF) unreadch(c);
            break;
        }
        p++;
    }
}

va_end(ap);
return assigned;
}


// quick manual test
int main(void) {
    // int a;
    // char s[64];
    // char ch;
    // int n = my_scanf("%d %s %c", &a, s, &ch);
    // printf("n=%d a=%d s=%s ch=%c\n", n, a, s, ch);

    // // tests char 
    // char ch;
    // printf("Enter a character: ");
    // my_scanf("%c", &ch);
    // printf("ch=%c\n", ch);

    // // tests string
    // char word[100];
    // printf("Enter a word: ");
    // int n = my_scanf("%s", word);
    // printf("n=%d word=\"%s\"\n", n, word);

    // // tests integer
    // int num;      
    // printf("Enter an integer: ");
    // int n = my_scanf("%d", &num);
    // printf("n=%d num=%d\n", n, num);

    // // tests hex
    // int x;
    // printf("Enter a hex number: ");
    // int n = my_scanf("%x", &x);
    // printf("n=%d x=%d (decimal)\n", n, x);

    // // test width modifier with %s
    // char str[10];
    // printf("Enter a word (width 4 chars): ");
    // int n = my_scanf("%4s", str);
    // printf("n=%d str=\"%s\"\n", n, str);

    // // test width modifier with %x
    // int x;
    // printf("Enter hex: ");
    // int n = my_scanf("%2x", &x);
    // printf("n=%d x=%d\n", n, x);

    // // %d with length modifiers
    // int a;
    // long b;
    // long long c;

    // printf("Enter three integers (int, long, long long): ");
    // my_scanf("%d %ld %lld", &a, &b, &c);  // 10 20 30
    // printf("a=%d b=%ld c=%lld\n", a, b, c);


    // // %f with length modifiers
    // float a;
    // double b;
    // long double c;

    // printf("Enter three numbers (float double longdouble): ");  
    // int n = my_scanf("%f %lf %Lf", &a, &b, &c); // 1.25 3e2 -0.0045

    // printf("n=%d\n", n);
    // printf("a=%f\n", a);
    // printf("b=%lf\n", b);
    // printf("c=%Lf\n", c);-0

    // test %Lf
    long double c = 123.0L;
    printf("Enter a long double: ");
    int n = my_scanf("%Lf", &c);
    printf("sizeof(long double)=%zu\n", sizeof(long double));
    printf("n=%d c=%.12Lf\n", n, c);

    return 0;
}

