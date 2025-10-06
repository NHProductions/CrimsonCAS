
#include <debug.h>
#include <Crimson.h>
#include <graphx.h>
#include <keypadc.h>
#include <sys/timers.h>
#define WIDTH 320
#define HEIGHT 240
#define RATE 100
Equation_Type Selected = ARITHMETIC;
string Current_Input;
string Ans;
void Draw_XSize(int sx, int sy, int W, int H, int Thickness) {
    for (int i = 0; i < Thickness; i++) {
        gfx_Rectangle(sx+i,sy+i,W,H);
    }
}
#define MAX_LINE_SIZE 38
void PrintStrCorrectly() {
    if (Current_Input.length < MAX_LINE_SIZE) {
        gfx_PrintStringXY(Current_Input.Content,2,34);
    }
    else {
        int Current_Y = 24;
        int Current_X = 2;
        for (int i = 0; i < Current_Input.length; i++) {
            if (i % MAX_LINE_SIZE == 0) {
                Current_Y += 10;
                Current_X = 2;
            }   
            gfx_SetTextXY(Current_X, Current_Y);
            gfx_PrintChar(Current_Input.Content[i]);
            Current_X += 8;
        }
    }

}
#define LINE_HEIGHT 15
#define CHAR_WIDTH 8
void PrintAns(string s) {
    // If the input string fits on one line, print it directly
    if (s.length < 18) {
        gfx_PrintStringXY(s.Content, 2, 100);
    } else {
        int Current_Y = 100;
        int Current_X = 2;

        for (int i = 0; i < s.length; i++) {
            // Only move down after a *full* line is printed
            if (i > 0 && i % MAX_LINE_SIZE == 0) {
                Current_Y += 15;   // move to next line
                Current_X = 2;     // reset X position
            }

            gfx_SetTextXY(Current_X, Current_Y);
            gfx_PrintChar(s.Content[i]);
            Current_X += 8; // advance X by 8 pixels per char (monospace)
        }
    }
}

void Draw_Static_UI() {
    gfx_FillScreen(96);
    gfx_SetColor(0); 
    Draw_XSize(0,0,320,16,3);
    char ToPrint[6][8] = {
        "Arith. ",
        "Linear ",
        "Quadr. ",
        "Absol. ",
        "System ",
    };
    int Spacing = 50;
    for (int i = 0; i < 6; i++) {
        gfx_PrintStringXY(ToPrint[i], Spacing*i+9, 6);
    }
    gfx_SetColor(231);
    Draw_XSize(Spacing*Selected+9,4,Spacing,12,1);
    PrintStrCorrectly();
    // credits/warning:
    gfx_PrintStringXY("CrimsonCAS", 2, 202);
    gfx_PrintStringXY("Made by NH",2,210);
    gfx_PrintStringXY("WARNING: extremely buggy, DO NOT TRUST", 2, 218);
    PrintAns(Ans);
}
void EraseChar() {
    if (Current_Input.length != 0) {
    string new = NewStr("");
    for (int i = 0; i < Current_Input.length-1; i++) {
        AddCharacter(&new, Current_Input.Content[i]);
    }
    Current_Input = new;
    }
}
int Current_KeyPress() {
    kb_Scan();
    if (kb_Data[6] & kb_Clear) {
        return 1;
    }
    if (kb_Data[7] & kb_Up) {
        Selected += 1;
        if (Selected > 4) {
            Selected = 0;
        }
        Draw_Static_UI();
    }
    if (kb_Data[7] & kb_Down) {
        if (Selected != 0) {
        Selected -= 1;
        if (Selected < 0) {
            Selected = 4;
        }
        }
        else {
            Selected = 4;
        }
        Draw_Static_UI();
    }
    if (kb_Data[3] & kb_0) {
        AddCharacter(&Current_Input, '0');
        Draw_Static_UI();
    }
    if (kb_Data[3] & kb_1) {
        AddCharacter(&Current_Input, '1');
        Draw_Static_UI();
    }
    if (kb_Data[3] & kb_4) {
        AddCharacter(&Current_Input, '4');
        Draw_Static_UI();
    }
    if (kb_Data[3] & kb_7) {
        AddCharacter(&Current_Input, '7');
        Draw_Static_UI();
    }
    if (kb_Data[4] & kb_2) {
        AddCharacter(&Current_Input, '2');
        Draw_Static_UI();
    }
    if (kb_Data[4] & kb_5) {
        AddCharacter(&Current_Input, '5');
        Draw_Static_UI();
    }
    if (kb_Data[4] & kb_8) {
        AddCharacter(&Current_Input, '8');
        Draw_Static_UI();
    }
    if (kb_Data[5] & kb_3) {
        AddCharacter(&Current_Input, '3');
        Draw_Static_UI();
    }
    if (kb_Data[5] & kb_6) {
        AddCharacter(&Current_Input, '6');
        Draw_Static_UI();
    }
    if (kb_Data[5] & kb_9) {
        AddCharacter(&Current_Input, '9');
        Draw_Static_UI();
    }
    if (kb_Data[5] & kb_RParen) {
        AddCharacter(&Current_Input, ')');
        Draw_Static_UI();
    }
    if (kb_Data[4] & kb_LParen) {
        AddCharacter(&Current_Input, '(');
        Draw_Static_UI();
    }
    if (kb_Data[3] & kb_Comma) {
        AddCharacter(&Current_Input, ';');
        Draw_Static_UI();
    }
    if (kb_Data[6] & kb_Enter) {
        AddCharacter(&Current_Input, '=');
        Draw_Static_UI();
    }
    if (kb_Data[6] & kb_Power) {
        AddCharacter(&Current_Input, '^');
        Draw_Static_UI();
    }
    if (kb_Data[6] & kb_Add) {
        AddCharacter(&Current_Input, '+');
        Draw_Static_UI();
    }
    if (kb_Data[6] & kb_Sub) {
        AddCharacter(&Current_Input, '-');
        Draw_Static_UI();
    }
    if (kb_Data[6] & kb_Mul) {
        AddCharacter(&Current_Input, '*');
        Draw_Static_UI();
    }
    if (kb_Data[6] & kb_Div) {
        AddCharacter(&Current_Input, '/');
        Draw_Static_UI();
    }
    if (kb_Data[4] & kb_DecPnt) {
        AddCharacter(&Current_Input, '.');
        Draw_Static_UI();
    }
    if (kb_Data[1] & kb_Del) {
        EraseChar();
        Draw_Static_UI();
    }
    if (kb_Data[2] & kb_Math) {
        AddCharacter(&Current_Input, 'a');
        AddCharacter(&Current_Input, 'b');
        AddCharacter(&Current_Input, 's');
        AddCharacter(&Current_Input, '(');
        Draw_Static_UI();
    }
    if (kb_Data[3] & kb_Sin) {
        AddCharacter(&Current_Input, 'x');
        Draw_Static_UI();
    }
    if (kb_Data[4] & kb_Cos) {
        AddCharacter(&Current_Input, 'y');
        Draw_Static_UI();
    }
    if (kb_Data[5] & kb_Tan) {
        AddCharacter(&Current_Input, 'z');
        Draw_Static_UI();
    }
    if (kb_Data[2] & kb_Recip) {
        AddCharacter(&Current_Input, '^');
        AddCharacter(&Current_Input, '-');
        AddCharacter(&Current_Input, '1');
        Draw_Static_UI();
    }
    if (kb_Data[2] & kb_Square) {
        AddCharacter(&Current_Input, '^');
        AddCharacter(&Current_Input, '2');
        Draw_Static_UI();
    }
    if (kb_Data[5] & kb_Vars) {
        Ans = EvaluateEquation(Current_Input,Selected);
        printf("ANSWER: %s", Ans.Content);
        Draw_Static_UI();
    }
    return 0;
}

int main() {
    Current_Input = NewStr("");
    Ans = NewStr("");
    gfx_Begin();
    dbg_printf("WORKING!");
    Draw_Static_UI();
    pool_reset();
    while (1==1) {
        if (Current_KeyPress() == 1) {
            break;
        }
        msleep(RATE);
    }

    return 0;
}