// Naomi Beck
#include <stdio.h>
#include <stdarg.h>  // for variadic fucntions: va_list, va_start
#include <ctype.h>  // for isspace()...

/* -----------------------------
Spec struct and parse_spec
----------------------------- */

typedef struct {
char conv;   // just the conversion letter for now: 'd','s','c','x','f'
} Spec;

static int parse_spec(const char **pp, Spec *out) {
// placeholder: minimal parse just reads one char as the conversion
if (**pp == '\0') return 0;
out->conv = **pp;
(*pp)++;
return 1;
}

/* -----------------------------
helper functions
----------------------------- */
static int has_buf = 0;
static int buf_ch = 0;

static int nextch(void) {
if (has_buf) {
    has_buf = 0;
    return buf_ch;
}
return getchar(); 
}

static void unreadch(int c) {
if (c == EOF) return;
has_buf = 1;
buf_ch = c;
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
    (void)sp;  // not used yet

    int c = nextch();
    if (c == EOF) return 0;

    char *out = va_arg(*ap, char*);
    *out = (char)c;
    return 1;
}


static int scan_s(const Spec *sp, va_list *ap) {
    (void)sp; // not using modifiers yet

    // %s skips leading whitespace
    skip_input_ws();

    char *out = va_arg(*ap, char*);
    int i = 0;

    int c = nextch();
    if (c == EOF) return 0;

    // If the next character is whitespace (or EOF), %s fails
    if (isspace((unsigned char)c)) {
        unreadch(c);
        return 0;
    }

    // Read until whitespace or EOF
    while (c != EOF && !isspace((unsigned char)c)) {
        out[i++] = (char)c;
        c = nextch();
    }

    // Put back the delimiter (whitespace) so the next conversion can see it
    if (c != EOF) unreadch(c);

    out[i] = '\0';
    return 1;
}


static int scan_d(const Spec *sp, va_list *ap) {
    (void)sp; // modifiers not used yet

    // %d skips leading whitespace
    skip_input_ws();

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
        value = value * 10 + (c - '0');
        c = nextch();
    }

    // we've read one char too far (non-digit or EOF)
    if (c != EOF) unreadch(c);

    int *out = va_arg(*ap, int*);
    *out = (int)(sign * value);

    return 1;
}


static int hex_value(int c) {
    if ('0' <= c && c <= '9') return c - '0';
    if ('a' <= c && c <= 'f') return 10 + (c - 'a');
    if ('A' <= c && c <= 'F') return 10 + (c - 'A');
    return -1;
}

static int scan_x(const Spec *sp, va_list *ap) {
    (void)sp; // modifiers not used yet

    // %x skips leading whitespace
    skip_input_ws();

    int c = nextch();
    if (c == EOF) return 0;

    // Optional 0x / 0X prefix
    if (c == '0') {
        int c2 = nextch();
        if (c2 == 'x' || c2 == 'X') {
            c = nextch(); // move to first hex digit after prefix
            if (c == EOF) return 0;
        } else {
            // not actually a prefix; put it back and keep c = '0'
            unreadch(c2);
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
        value = value * 16 + (unsigned long)hv;
        c = nextch();
    }

    if (c != EOF) unreadch(c);

    int *out = va_arg(*ap, int*);
    *out = (int)value;  // may overflow for huge hex inputs; okay for basic version

    return 1;
}

static int scan_f(const Spec *sp, va_list *ap) { (void)sp; (void)ap; return 0; }

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

        int ok = 0;
        switch (sp.conv) {
            case 'c': ok = scan_c(&sp, &ap); break;
            case 's': ok = scan_s(&sp, &ap); break;
            case 'd': ok = scan_d(&sp, &ap); break;
            case 'x': ok = scan_x(&sp, &ap); break;
            case 'f': ok = scan_f(&sp, &ap); break;
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
// return 0;

    int x;
    printf("Enter a hex number: ");
    int n = my_scanf("%x", &x);
    printf("n=%d x=%d (decimal)\n", n, x);
    return 0;

}
