/*
 * 2D ASCII Graphics Editor
 * Supports: Circle, Rectangle, Line, Triangle
 * Fill characters: '*' and '_'
 * Compile: gcc -o graphics_editor graphics_editor.c -lm
 * Run:     ./graphics_editor
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ─── Canvas dimensions ─── */
#define MAX_W   80
#define MAX_H   40
#define MAX_OBJ 64

/* ─── Shape types ─── */
typedef enum { CIRCLE, RECT, LINE, TRIANGLE } ShapeType;

/* ─── Object descriptor ─── */
typedef struct {
    int       id;
    ShapeType type;
    char      ch;        /* '*' or '_' */
    /* Parameters (meaning depends on type) */
    int p[6];            /* circle: cx,cy,r  |  rect: x,y,w,h
                            line: x1,y1,x2,y2 | tri: x1,y1,x2,y2,x3,y3 */
    int active;
} Object;

/* ─── Global state ─── */
static char   canvas[MAX_H][MAX_W + 1]; /* +1 for '\0' */
static int    CW = 60, CH = 30;
static Object objects[MAX_OBJ];
static int    obj_count = 0;
static int    next_id   = 1;

/* ══════════════════════════════════════════
   CANVAS
   ══════════════════════════════════════════ */

void canvas_clear(void) {
    for (int y = 0; y < CH; y++) {
        memset(canvas[y], ' ', CW);
        canvas[y][CW] = '\0';
    }
}

void canvas_put(int x, int y, char ch) {
    if (x >= 0 && x < CW && y >= 0 && y < CH)
        canvas[y][x] = ch;
}

void display_canvas(void) {
    /* Top border */
    printf("+");
    for (int i = 0; i < CW; i++) printf("-");
    printf("+\n");

    for (int y = 0; y < CH; y++) {
        printf("|%s|\n", canvas[y]);
    }

    /* Bottom border */
    printf("+");
    for (int i = 0; i < CW; i++) printf("-");
    printf("+\n");
}

/* ══════════════════════════════════════════
   DRAWING PRIMITIVES
   ══════════════════════════════════════════ */

/* Circle — distance formula with 2:1 Y-stretch for char-cell aspect ratio */
void draw_circle(int cx, int cy, int r, char ch) {
    for (int y = 0; y < CH; y++) {
        for (int x = 0; x < CW; x++) {
            double dx = x - cx;
            double dy = (y - cy) * 2.0; /* compensate for char height */
            double d  = sqrt(dx * dx + dy * dy);
            if (fabs(d - r) < 0.65)
                canvas_put(x, y, ch);
        }
    }
}

/* Axis-aligned rectangle (outline only) */
void draw_rect(int x, int y, int w, int h, char ch) {
    for (int i = x; i < x + w; i++) {
        canvas_put(i, y,         ch); /* top    */
        canvas_put(i, y + h - 1, ch); /* bottom */
    }
    for (int j = y; j < y + h; j++) {
        canvas_put(x,         j, ch); /* left   */
        canvas_put(x + w - 1, j, ch); /* right  */
    }
}

/* Line — Bresenham's algorithm */
void draw_line(int x1, int y1, int x2, int y2, char ch) {
    int dx  =  abs(x2 - x1);
    int dy  =  abs(y2 - y1);
    int sx  = (x1 < x2) ? 1 : -1;
    int sy  = (y1 < y2) ? 1 : -1;
    int err = dx - dy;
    int x = x1, y = y1;

    while (1) {
        canvas_put(x, y, ch);
        if (x == x2 && y == y2) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x += sx; }
        if (e2 <  dx) { err += dx; y += sy; }
    }
}

/* Triangle — three connected lines */
void draw_triangle(int x1, int y1,
                   int x2, int y2,
                   int x3, int y3, char ch) {
    draw_line(x1, y1, x2, y2, ch);
    draw_line(x2, y2, x3, y3, ch);
    draw_line(x3, y3, x1, y1, ch);
}

/* ══════════════════════════════════════════
   OBJECT DISPATCH
   ══════════════════════════════════════════ */

void render_object(const Object *o) {
    switch (o->type) {
        case CIRCLE:
            draw_circle(o->p[0], o->p[1], o->p[2], o->ch);
            break;
        case RECT:
            draw_rect(o->p[0], o->p[1], o->p[2], o->p[3], o->ch);
            break;
        case LINE:
            draw_line(o->p[0], o->p[1], o->p[2], o->p[3], o->ch);
            break;
        case TRIANGLE:
            draw_triangle(o->p[0], o->p[1],
                          o->p[2], o->p[3],
                          o->p[4], o->p[5], o->ch);
            break;
    }
}

void redraw_all(void) {
    canvas_clear();
    for (int i = 0; i < obj_count; i++)
        if (objects[i].active)
            render_object(&objects[i]);
}

/* ══════════════════════════════════════════
   OBJECT MANAGEMENT
   ══════════════════════════════════════════ */

static const char *type_name(ShapeType t) {
    switch (t) {
        case CIRCLE:   return "Circle";
        case RECT:     return "Rectangle";
        case LINE:     return "Line";
        case TRIANGLE: return "Triangle";
    }
    return "?";
}

void list_objects(void) {
    int any = 0;
    printf("\n  ID  Type        Char  Parameters\n");
    printf("  --  ----------  ----  ---------------------------------\n");
    for (int i = 0; i < obj_count; i++) {
        if (!objects[i].active) continue;
        any = 1;
        const Object *o = &objects[i];
        printf("  %-3d %-10s   %c     ", o->id, type_name(o->type), o->ch);
        switch (o->type) {
            case CIRCLE:
                printf("cx=%d cy=%d r=%d", o->p[0], o->p[1], o->p[2]);
                break;
            case RECT:
                printf("x=%d y=%d w=%d h=%d", o->p[0], o->p[1], o->p[2], o->p[3]);
                break;
            case LINE:
                printf("(%d,%d)->(%d,%d)", o->p[0], o->p[1], o->p[2], o->p[3]);
                break;
            case TRIANGLE:
                printf("(%d,%d) (%d,%d) (%d,%d)",
                    o->p[0], o->p[1], o->p[2], o->p[3], o->p[4], o->p[5]);
                break;
        }
        printf("\n");
    }
    if (!any) printf("  (no objects)\n");
    printf("\n");
}

int find_object(int id) {
    for (int i = 0; i < obj_count; i++)
        if (objects[i].id == id && objects[i].active)
            return i;
    return -1;
}

void add_object(ShapeType type, char ch, int *params, int np) {
    if (obj_count >= MAX_OBJ) {
        printf("  [!] Max object limit (%d) reached.\n", MAX_OBJ);
        return;
    }
    Object *o = &objects[obj_count++];
    o->id     = next_id++;
    o->type   = type;
    o->ch     = ch;
    o->active = 1;
    memset(o->p, 0, sizeof(o->p));
    for (int i = 0; i < np; i++) o->p[i] = params[i];
    printf("  [+] Added %s with ID %d.\n", type_name(type), o->id);
}

void delete_object(int id) {
    int idx = find_object(id);
    if (idx < 0) { printf("  [!] ID %d not found.\n", id); return; }
    objects[idx].active = 0;
    printf("  [-] Deleted object ID %d.\n", id);
}

void modify_object(int id, int *params, int np, int change_ch, char new_ch) {
    int idx = find_object(id);
    if (idx < 0) { printf("  [!] ID %d not found.\n", id); return; }
    Object *o = &objects[idx];
    for (int i = 0; i < np; i++) o->p[i] = params[i];
    if (change_ch) o->ch = new_ch;
    printf("  [~] Modified object ID %d.\n", id);
}

/* ══════════════════════════════════════════
   INPUT HELPERS
   ══════════════════════════════════════════ */

static void flush_stdin(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

static char read_char(const char *prompt) {
    char ch;
    printf("%s", prompt);
    scanf(" %c", &ch);
    return ch;
}

static int read_int(const char *prompt) {
    int v;
    printf("%s", prompt);
    scanf("%d", &v);
    return v;
}

/* ══════════════════════════════════════════
   INTERACTIVE MENUS
   ══════════════════════════════════════════ */

void menu_add(void) {
    printf("\n  Add Shape\n");
    printf("  1) Circle\n  2) Rectangle\n  3) Line\n  4) Triangle\n");
    int choice = read_int("  Choice: ");

    char ch = read_char("  Fill char (* or _): ");
    if (ch != '*' && ch != '_') { printf("  [!] Invalid char. Using '*'.\n"); ch = '*'; }

    int p[6] = {0};
    int np = 0;

    switch (choice) {
        case 1:
            p[0] = read_int("  Center X: ");
            p[1] = read_int("  Center Y: ");
            p[2] = read_int("  Radius: ");
            np = 3;
            add_object(CIRCLE, ch, p, np);
            break;
        case 2:
            p[0] = read_int("  X (left): ");
            p[1] = read_int("  Y (top): ");
            p[2] = read_int("  Width: ");
            p[3] = read_int("  Height: ");
            np = 4;
            add_object(RECT, ch, p, np);
            break;
        case 3:
            p[0] = read_int("  X1: "); p[1] = read_int("  Y1: ");
            p[2] = read_int("  X2: "); p[3] = read_int("  Y2: ");
            np = 4;
            add_object(LINE, ch, p, np);
            break;
        case 4:
            p[0] = read_int("  X1: "); p[1] = read_int("  Y1: ");
            p[2] = read_int("  X2: "); p[3] = read_int("  Y2: ");
            p[4] = read_int("  X3: "); p[5] = read_int("  Y3: ");
            np = 6;
            add_object(TRIANGLE, ch, p, np);
            break;
        default:
            printf("  [!] Unknown choice.\n");
            return;
    }
    redraw_all();
}

void menu_delete(void) {
    list_objects();
    int id = read_int("  Enter ID to delete: ");
    delete_object(id);
    redraw_all();
}

void menu_modify(void) {
    list_objects();
    int id = read_int("  Enter ID to modify: ");
    int idx = find_object(id);
    if (idx < 0) { printf("  [!] ID %d not found.\n", id); return; }

    Object *o = &objects[idx];
    printf("  Modifying %s (ID %d). Enter new values:\n", type_name(o->type), id);

    int p[6] = {0};
    int np = 0;

    switch (o->type) {
        case CIRCLE:
            p[0] = read_int("  Center X: ");
            p[1] = read_int("  Center Y: ");
            p[2] = read_int("  Radius: ");
            np = 3;
            break;
        case RECT:
            p[0] = read_int("  X (left): ");
            p[1] = read_int("  Y (top): ");
            p[2] = read_int("  Width: ");
            p[3] = read_int("  Height: ");
            np = 4;
            break;
        case LINE:
            p[0] = read_int("  X1: "); p[1] = read_int("  Y1: ");
            p[2] = read_int("  X2: "); p[3] = read_int("  Y2: ");
            np = 4;
            break;
        case TRIANGLE:
            p[0] = read_int("  X1: "); p[1] = read_int("  Y1: ");
            p[2] = read_int("  X2: "); p[3] = read_int("  Y2: ");
            p[4] = read_int("  X3: "); p[5] = read_int("  Y3: ");
            np = 6;
            break;
    }

    char ans = read_char("  Change fill char? (y/n): ");
    int  change_ch = (ans == 'y' || ans == 'Y');
    char new_ch    = o->ch;
    if (change_ch) {
        new_ch = read_char("  New char (* or _): ");
        if (new_ch != '*' && new_ch != '_') {
            printf("  [!] Invalid char. Keeping '%c'.\n", o->ch);
            change_ch = 0;
        }
    }

    modify_object(id, p, np, change_ch, new_ch);
    redraw_all();
}

/* ══════════════════════════════════════════
   MAIN
   ══════════════════════════════════════════ */

int main(void) {
    canvas_clear();

    printf("╔══════════════════════════════════════╗\n");
    printf("║   2D ASCII Graphics Editor  (C)      ║\n");
    printf("║   Shapes: Circle  Rect  Line  Tri    ║\n");
    printf("║   Chars : *  _                       ║\n");
    printf("╚══════════════════════════════════════╝\n\n");

    /* Add a demo scene */
    int p[6];
    p[0]=29; p[1]=14; p[2]=10; add_object(CIRCLE,   '*', p, 3);
    p[0]=1;  p[1]=1;  p[2]=58; p[3]=28; add_object(RECT, '_', p, 4);
    p[0]=29; p[1]=2;  p[2]=10; p[3]=26; p[4]=48; p[5]=26; add_object(TRIANGLE,'*',p,6);
    redraw_all();
    display_canvas();

    int running = 1;
    while (running) {
        printf("┌─ Menu ───────────────────────────────┐\n");
        printf("│ 1) Display canvas                    │\n");
        printf("│ 2) Add shape                         │\n");
        printf("│ 3) Delete shape                      │\n");
        printf("│ 4) Modify shape                      │\n");
        printf("│ 5) List objects                      │\n");
        printf("│ 6) Clear all                         │\n");
        printf("│ 0) Quit                              │\n");
        printf("└──────────────────────────────────────┘\n");

        int choice = read_int("Choice: ");
        flush_stdin();

        switch (choice) {
            case 1:
                display_canvas();
                break;
            case 2:
                menu_add();
                display_canvas();
                break;
            case 3:
                menu_delete();
                display_canvas();
                break;
            case 4:
                menu_modify();
                display_canvas();
                break;
            case 5:
                list_objects();
                break;
            case 6:
                obj_count = 0;
                next_id   = 1;
                canvas_clear();
                printf("  [x] All objects cleared.\n");
                display_canvas();
                break;
            case 0:
                running = 0;
                printf("  Goodbye!\n");
                break;
            default:
                printf("  [!] Invalid option.\n");
        }
    }
    return 0;
}
