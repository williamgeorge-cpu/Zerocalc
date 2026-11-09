#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <graphx.h>
#include <keypadc.h>

/* Visual Constants for a classic C64 terminal palette */
#define COLOR_LIGHT_BLUE 0x3B /* Text and inner background */
#define COLOR_DARK_BLUE 0x9D /* Outer frame background */
#define COLOR_WHITE 0x9D 
#define CHAR_WIDTH 8
#define CHAR_HEIGHT 8

/* State variables for custom input typing buffer */
char input_buffer[32];
uint8_t buffer_len = 0;

/* Helper to map the keypad scan matrix directly to characters */
char get_char_from_key(void) {
    kb_Scan();
    /* Number Row 1-3 */
    if (kb_Data[3] & kb_1) return '1';
    if (kb_Data[4] & kb_2) return '2';
    if (kb_Data[5] & kb_3) return '3';
    /* Number Row 4-6 */
    if (kb_Data[3] & kb_4) return '4';
    if (kb_Data[4] & kb_5) return '5';
    if (kb_Data[5] & kb_6) return '6';
    /* Number Row 7-9 */
    if (kb_Data[3] & kb_7) return '7';
    if (kb_Data[4] & kb_8) return '8';
    if (kb_Data[5] & kb_9) return '9';
    /* Zero, Decimals, and Negatives */
    if (kb_Data[3] & kb_0) return '0';
    if (kb_Data[4] & kb_DecPnt) return '.';
    if (kb_Data[5] & kb_Chs) return '-';
    return 0;
}

/* Debounced key reader to eliminate fast looping multi-triggers */
char read_key_debounced(void) {
    char key = 0;
    while (1) {
        kb_Scan();
        
        /* 1. Check control keys directly without blocking inside a helper */
        if (kb_Data[6] & kb_Clear) {
            while (kb_Data[6] & kb_Clear) { kb_Scan(); } /* Wait for Clear release safely */
            return 27;
        }
        if (kb_Data[6] & kb_Enter) {
            while (kb_Data[6] & kb_Enter) { kb_Scan(); } /* Wait for Enter release safely */
            return '\n';
        }
        if (kb_Data[1] & kb_Del) {
            while (kb_Data[1] & kb_Del) { kb_Scan(); } /* Wait for Del release safely */
            return '\b';
        }

        /* 2. Check character keys */
        key = get_char_from_key();
        if (key != 0) {
            while (get_char_from_key() != 0); /* Wait for release */
            return key;
        }
    }
}

/* Redraws the terminal frame base mimicking a C64 window layout and establishes text styling */
void draw_c64_screen(void) {
    gfx_FillScreen(COLOR_DARK_BLUE);
    gfx_SetColor(COLOR_LIGHT_BLUE);
    gfx_FillRectangle(24, 24, 272, 192);
    
    /* Re-verify color properties every clear to prevent invisible text */
    gfx_SetTextFGColor(COLOR_WHITE);
    gfx_SetTextBGColor(COLOR_LIGHT_BLUE);
    gfx_SetTextTransparentColor(COLOR_LIGHT_BLUE);
}

/* Custom terminal string display worker */
void term_print(const char *str, uint16_t *x, uint8_t *y) {
    gfx_SetTextXY(*x, *y);
    gfx_PrintString(str);
    *y += CHAR_HEIGHT + 2;
    if (*y > 200) {
        draw_c64_screen();
        *y = 32;
    }
}

/* Collects numeric entries seamlessly while rendering characters in real time */
float get_numeric_input(const char *prompt, uint16_t *x, uint8_t *y) {
    char key;
    buffer_len = 0;
    memset(input_buffer, 0, sizeof(input_buffer));
    term_print(prompt, x, y);
    
    uint8_t input_y = *y - (CHAR_HEIGHT + 2);
    uint16_t prompt_width = gfx_GetStringWidth(prompt);
    uint16_t input_x = *x + prompt_width + 4;
    
    while (1) {
        /* Clear old workspace dynamically inside the terminal area */
        gfx_SetColor(COLOR_LIGHT_BLUE);
        gfx_FillRectangle(input_x, input_y, 120, CHAR_HEIGHT);
        gfx_SetTextXY(input_x, input_y);
        gfx_PrintString(input_buffer);
        
        key = read_key_debounced();
        if (key == 27) {
            gfx_End();
            exit(0);
        }
        if (key == '\n' && buffer_len > 0) {
            break;
        }
        if (key == '\b' && buffer_len > 0) {
            input_buffer[--buffer_len] = '\0';
        }
        if (key >= '-' && key <= '9' && buffer_len < 15 && key != '/') {
            input_buffer[buffer_len++] = key;
            input_buffer[buffer_len] = '\0';
        }
    }
    
    float val;
    sscanf(input_buffer, "%f", &val);
    return val;
}

int main(void) {
    float a, b, c;
    float discriminant, root1, root2, real_part, imag_part, vert_X, vert_Y;
    char out_buf[40];
    uint16_t tx = 32;
    uint8_t ty = 32;
    
    gfx_Begin();
    draw_c64_screen();
    
    term_print("**** TI84+CE QUADRATIC SOLVER ****", &tx, &ty);
    term_print("PRESS [CLEAR] AT ANY TIME TO EXIT", &tx, &ty);
    ty += 4;
    
    while (1) {
        a = get_numeric_input("ENTER A: ", &tx, &ty);
        if (a == 0.0f) {
            term_print("ERROR: A CANNOT BE ZERO.", &tx, &ty);
            ty += 6;
            continue;
        }
        b = get_numeric_input("ENTER B: ", &tx, &ty);
        c = get_numeric_input("ENTER C: ", &tx, &ty);
        
        discriminant = (b * b) - (4.0f * a * c);
        ty += 2;
        
        if (discriminant > 0.0f) {
            root1 = (-b + sqrtf(discriminant)) / (2.0f * a);
            root2 = (-b - sqrtf(discriminant)) / (2.0f * a);
            sprintf(out_buf, "X1 = %.4f", root1);
            term_print(out_buf, &tx, &ty);
            sprintf(out_buf, "X2 = %.4f", root2);
            term_print(out_buf, &tx, &ty);
        } else if (discriminant == 0.0f) {
            root1 = (-b) / (2.0f * a);
            sprintf(out_buf, "X = %.4f (DOUBLE ROOT)", root1);
            term_print(out_buf, &tx, &ty);
        } else {
            real_part = (-b) / (2.0f * a);
            imag_part = sqrtf(-discriminant) / (2.0f * a);
            sprintf(out_buf, "X1 = %.4f + %.4fi", real_part, imag_part);
            term_print(out_buf, &tx, &ty);
            sprintf(out_buf, "X2 = %.4f - %.4fi", real_part, imag_part);
            term_print(out_buf, &tx, &ty);
		}

		vert_X = -b / (2.0f * a);

		vert_Y = (a * (vert_X * vert_X)) + (b * vert_X) + c;

		char open_p = 0x28;  // ASCII code for (
		char close_p = 0x29; // ASCII code for )
		
		sprintf(out_buf, "VERTEX = %c%.4f,%.4f%c", open_p, vert_X, vert_Y, close_p);

		term_print(out_buf, &tx, &ty);
        
        ty += 7;
        term_print("PRESS [ENTER] TO RESTART...", &tx, &ty);
        
        /* Loop until enter is hit, but safely capture clear exits here too */
        while (1) {
            char next_action = read_key_debounced();
            if (next_action == '\n') break;
            if (next_action == 27) {
                gfx_End();
                return 0;
            }
        }
        
        /* Clear and recycle workspace */
        draw_c64_screen();
        tx = 32;
        ty = 32;
        term_print("**** TI84+CE QUADRATIC SOLVER ****", &tx, &ty);
        ty += 4;
    }
    
    gfx_End();
    return 0;
}
