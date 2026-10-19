/*
 * Bond Interest Rate Calculator (v3)
 * Given: principal (P), maturity amount (A) and tenure,
 * find the rate of interest.
 *
 * Features:
 *  - Tenure can be entered in days (d), months (m) or years (y)
 *  - Every number can be typed as an expression, e.g. 100000+50000,
 *    (2*75000)-500, 1200/12. Supports + - * / ( ) and decimals.
 *    You can also use x for multiply, and commas like 1,00,000.
 *
 * Bond (no compounding), so the main answer is the simple interest rate:
 *   r = (A - P) / (P * t)        (t in years)
 * For comparison with mutual funds it also shows the yearly compounded
 * equivalent (CAGR):  r = (A/P)^(1/t) - 1
 *
 * Compile : gcc interest_rate.c -o interest_rate -lm
 * Run     : ./interest_rate
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

/* ---------- tiny expression evaluator ---------- */

static const char *p;           /* current position in the text */
static int parse_error;
static const char *error_msg;

static double parse_expr(void);

static void set_error(const char *msg)
{
    if (!parse_error) {
        parse_error = 1;
        error_msg = msg;
    }
}

static void skip_spaces(void)
{
    while (*p == ' ' || *p == '\t')
        p++;
}

static double parse_number(void)
{
    char buf[64];
    int i = 0;

    skip_spaces();
    while (isdigit((unsigned char)*p) || *p == '.' || *p == ',') {
        if (*p != ',' && i < 63)
            buf[i++] = *p;
        p++;
    }
    buf[i] = '\0';

    if (i == 0) {
        set_error("expected a number");
        return 0;
    }

    char *end;
    double v = strtod(buf, &end);
    if (*end != '\0') {
        set_error("bad number");
        return 0;
    }
    return v;
}

/* number, (expression) or a +/- sign in front of one */
static double parse_factor(void)
{
    skip_spaces();

    if (*p == '+') { p++; return parse_factor(); }
    if (*p == '-') { p++; return -parse_factor(); }

    if (*p == '(') {
        p++;
        double v = parse_expr();
        skip_spaces();
        if (*p != ')') {
            set_error("missing closing bracket )");
            return 0;
        }
        p++;
        return v;
    }
    return parse_number();
}

/* handles * and / (higher priority than + and -) */
static double parse_term(void)
{
    double v = parse_factor();

    for (;;) {
        skip_spaces();
        if (*p == '*' || *p == 'x' || *p == 'X') {
            p++;
            v *= parse_factor();
        } else if (*p == '/') {
            p++;
            double d = parse_factor();
            if (d == 0) {
                set_error("division by zero");
                return 0;
            }
            v /= d;
        } else {
            break;
        }
    }
    return v;
}

/* handles + and - */
static double parse_expr(void)
{
    double v = parse_term();

    for (;;) {
        skip_spaces();
        if (*p == '+') {
            p++;
            v += parse_term();
        } else if (*p == '-') {
            p++;
            v -= parse_term();
        } else {
            break;
        }
    }
    return v;
}

static int evaluate(const char *text, double *out)
{
    p = text;
    parse_error = 0;
    error_msg = "";

    double v = parse_expr();
    skip_spaces();

    if (!parse_error && *p != '\0')
        set_error("unexpected character");
    if (parse_error)
        return 0;

    *out = v;
    return 1;
}

/* ---------- input helpers ---------- */

static void read_line(char *line, size_t size)
{
    if (!fgets(line, (int)size, stdin)) {
        printf("\nInput ended.\n");
        exit(1);
    }
    line[strcspn(line, "\r\n")] = '\0';
}

/* asks until the user enters a valid number/expression */
static double read_value(const char *prompt)
{
    char line[256];

    for (;;) {
        printf("%s", prompt);
        read_line(line, sizeof line);

        double v;
        if (!evaluate(line, &v)) {
            printf("  Could not understand that (%s). Try again.\n", error_msg);
            continue;
        }
        if (v <= 0) {
            printf("  Value must be greater than 0. Try again.\n");
            continue;
        }
        printf("  = %.2f\n", v);
        return v;
    }
}

static char read_unit(void)
{
    char line[64];

    for (;;) {
        printf("Tenure in (d)ays, (m)onths or (y)ears? [d/m/y]: ");
        read_line(line, sizeof line);

        const char *s = line;
        while (*s == ' ' || *s == '\t')
            s++;

        char c = (char)tolower((unsigned char)*s);
        if (c == 'd' || c == 'm' || c == 'y')
            return c;
        printf("  Please type d, m or y.\n");
    }
}

/* ---------- main program ---------- */

int main(void)
{
    char again[64];

    printf("=== Bond Interest Rate Calculator ===\n");
    printf("Tip: you can type sums like 100000+50000 or 12*5000.\n\n");

    do {
        double principal = read_value("Enter principal amount (invested): ");
        double maturity  = read_value("Enter maturity amount (final value): ");

        char unit = read_unit();
        const char *unit_name = (unit == 'd') ? "days"
                              : (unit == 'm') ? "months" : "years";
        char prompt[64];
        snprintf(prompt, sizeof prompt, "Enter tenure in %s: ", unit_name);
        double tenure = read_value(prompt);

        /* convert tenure to years */
        double years;
        if (unit == 'd')      years = tenure / 365.0;
        else if (unit == 'm') years = tenure / 12.0;
        else                  years = tenure;

        double ratio = maturity / principal;
        double simple_rate = ((maturity - principal) / (principal * years)) * 100.0;
        double cagr = (pow(ratio, 1.0 / years) - 1.0) * 100.0;

        printf("\n--- Result ---\n");
        printf("Principal       : %.2f\n", principal);
        printf("Maturity amount : %.2f\n", maturity);
        printf("Profit / Loss   : %.2f\n", maturity - principal);
        printf("Total return    : %.2f%%\n", (ratio - 1.0) * 100.0);
        printf("Tenure          : %.2f %s (%.4f years)\n", tenure, unit_name, years);
        printf("Interest rate   : %.2f%% per year (simple, no compounding)\n", simple_rate);
        printf("Yearly compound equivalent (CAGR, for comparison): %.2f%%\n", cagr);

        if (years < 1.0)
            printf("\nNote: tenure is under 1 year, so the yearly rate is only an "
                   "annualised estimate and can look very high or low.\n");

        printf("\nCalculate again? (y/n): ");
        read_line(again, sizeof again);
        printf("\n");
    } while (again[0] == 'y' || again[0] == 'Y');

    printf("Done. Thank you!\n");
    return 0;
}
