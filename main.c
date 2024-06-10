#include <stdio.h>
#include <stdbool.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>

#define REFRESH 16
#define AXIS_WIDTH 2
#define CTRL_RADIUS 8
#define CTRL_PADDING (CTRL_RADIUS + CTRL_RADIUS/2)

#define MAX_CURVE_RES 64
#define MIN_CURVE_RES 1

#define ANTI_ALIAS_CURV 0

// Quadratic Curves
#define CTRL_PTS 4 

#define LENGTH(x) ((sizeof(x))/(sizeof(x[0])))


static int GRID_SCALE = 50;
static int CARTESIAN_GRID_MULT = 1;

static int curve_res = 4;

typedef struct {
        float x;
        float y;
} Vec2;

static SDL_Window *window;
static SDL_Renderer *renderer;

SDL_Point cart_to_scrn(Vec2 c);
Vec2 scrn_to_cart(SDL_Point p);

// Some random default location
static Vec2 bezier_ctrl_pts[CTRL_PTS] = {
        {-4, -2},
        {-2, 2},
        {2, 1},
        {4, -2},
};

Vec2 vec2lerp(Vec2 p_i, Vec2 p_f, float t)
{
        return (Vec2) {
                p_i.x + (p_f.x - p_i.x) * t,
                p_i.y + (p_f.y - p_i.y) * t,
        };
}

void draw_cubic_curve(Vec2 p[], int s)
{
        Vec2 b[s+1];
        float t = 1.0/s;

        float x = 0;
        int i = 0;
        for (i = 0, x = 0; x < 1 || i < (s+1); x += t, ++i) {
                Vec2 g[3] = {0};
                for (int j = 0; j < 3; j++)
                        g[j] = vec2lerp(p[j], p[j+1], x);

                Vec2 h[2] = {0};
                for (int j = 0; j < 2; j++)
                        h[j] = vec2lerp(g[j], g[j+1], x);
                
                b[i] = vec2lerp(h[0], h[1], x);
        }

        SDL_Point c[s+1];
        for (int i = 0; i < s+1; i++) {
                c[i] = cart_to_scrn(b[i]);
        }
        
        SDL_SetRenderDrawColor(renderer, 0x4F, 0xAF, 0xDF, 0xFF);
        if (ANTI_ALIAS_CURV) {
                for (int j = 0; j < s; j++) 
                        aalineRGBA(renderer, c[j].x, c[j].y, c[j+1].x, c[j+1].y,
                                0x4F, 0xAF, 0xDF, 0xFF);
        } else {
                for (int j = 0; j < s; j++) 
                        thickLineRGBA(renderer, c[j].x, c[j].y, c[j+1].x, c[j+1].y, 3,
                                        0x4F, 0xAF, 0xDF, 0xFF);
        }
}

Vec2 scrn_to_cart(SDL_Point p)
{
        SDL_Rect vp;
        SDL_RenderGetViewport(renderer, &vp);
        const SDL_Point o = {vp.w / 2, vp.h / 2};

        Vec2 c = {
                (p.x - o.x) * ((float)CARTESIAN_GRID_MULT/(float)GRID_SCALE),
                -(p.y - o.y) * ((float)CARTESIAN_GRID_MULT/(float)GRID_SCALE),
        };

        return c;
}

SDL_Point cart_to_scrn(Vec2 c)
{
        SDL_Rect vp;
        SDL_RenderGetViewport(renderer, &vp);
        const SDL_Point o = {.x = vp.w / 2, .y = vp.h / 2};
        
        SDL_Point p = {
                c.x * ((float)GRID_SCALE/(float)CARTESIAN_GRID_MULT) + o.x,
                -c.y * ((float)GRID_SCALE/(float)CARTESIAN_GRID_MULT) + o.y,
        };

        return p;
}

void init(void)
{
        if (SDL_Init(SDL_INIT_VIDEO) != 0) {
                fprintf(stderr, "Error initializing SDL: %s\n", SDL_GetError());
                exit(1);
        }

        window = SDL_CreateWindow("Curves",
                        SDL_WINDOWPOS_UNDEFINED,
                        SDL_WINDOWPOS_UNDEFINED,
                        800, 600,
                        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
        if (window == NULL) {
                fprintf(stderr, "Error creating window: %s\n", SDL_GetError());
                SDL_Quit();
                exit(1);
        }

        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        if (renderer == NULL) {
                printf("Error creating renderer: %s\n", SDL_GetError());
                SDL_DestroyWindow(window);
                SDL_Quit();
                exit(1);
        }
}

int grid_skip(int scrn)
{
        int middle_grid_pos = (scrn / (GRID_SCALE * 2)) * GRID_SCALE;
        int actual_center = scrn/2;
        return actual_center - middle_grid_pos;
}

void draw_grid(void)
{
        SDL_Rect vp;
        SDL_RenderGetViewport(renderer, &vp);
        
        SDL_Rect x_axis = {
                .x = vp.x,
                .y = vp.h/2,
                .w = vp.w,
                .h = AXIS_WIDTH,
        };

        SDL_Rect y_axis = { 
                .x = vp.w/2,
                .y = vp.y,
                .w = AXIS_WIDTH,
                .h = vp.h,
        };

        SDL_SetRenderDrawColor(renderer, 0x3F, 0x3F, 0x4F, 0xFF);

        // Skip certain amount of pixels to align grid to center
        int xskip = grid_skip(vp.w);
        for (int i = xskip; i < vp.w; i += GRID_SCALE) {
                SDL_RenderDrawLine(renderer, i, 0, i, vp.h);
        }

        int yskip = grid_skip(vp.h);
        for (int i = yskip; i < vp.h; i += GRID_SCALE) {
                SDL_RenderDrawLine(renderer, 0, i, vp.w, i);
        }

        SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
        SDL_RenderFillRect(renderer, &x_axis);
        SDL_RenderFillRect(renderer, &y_axis);
}

void draw_ctrl_points(void)
{
        for (int i = 0; i < CTRL_PTS; i++) {
                SDL_Point p = cart_to_scrn(bezier_ctrl_pts[i]);
                aacircleRGBA(renderer, p.x, p.y, CTRL_RADIUS, 155, 250, 0, 255);
                filledCircleRGBA(renderer, p.x, p.y, CTRL_RADIUS * 0.5, 0, 210, 240, 255);
        }

        for (int i = 0; i < CTRL_PTS - 1; i++) {
                SDL_Point p = cart_to_scrn(bezier_ctrl_pts[i]);
                SDL_Point q = cart_to_scrn(bezier_ctrl_pts[i+1]);
                if (ANTI_ALIAS_CURV)
                        aalineRGBA(renderer, p.x, p.y, q.x, q.y, 0xFC, 0x5A, 0x6B, 0x7F);
                else
                        thickLineRGBA(renderer, p.x, p.y, q.x, q.y, 3, 0xFC, 0x5A, 0x6B, 0x7F);
        }
}

bool place_point(const SDL_Point p)
{
        static int i = 0;
        if (i < CTRL_PTS) {
                bezier_ctrl_pts[i++] = scrn_to_cart(p);
                return true;
        } else {
                return false;
        }
}

void move_point(Vec2 *p)
{
        SDL_Point m;
        SDL_GetMouseState(&m.x, &m.y);
        *p = scrn_to_cart(m);
}

bool mouse_over_point(const SDL_Point c)
{
        SDL_Point m;
        SDL_GetMouseState(&m.x, &m.y);
        return (m.x > c.x - CTRL_PADDING && m.x < c.x + CTRL_PADDING) &&
                (m.y > c.y - CTRL_PADDING && m.y < c.y + CTRL_PADDING);
}

// Is pointer to point for allowing NULL
void pan_camera(SDL_Point *set)
{
        // TODO: Ability to pan camera
}

int main(void) 
{
        init();
        SDL_Event ev;

        bool quit = 0;
        short mouse_drag = 0;

        while (!quit) {
                while (SDL_PollEvent(&ev)) {
                        switch (ev.type) {
                        case SDL_MOUSEBUTTONDOWN: 
                                if (SDL_GetMouseState(NULL, NULL) & SDL_BUTTON_LEFT) {
                                      for (int i = 0; i < CTRL_PTS; i++) {
                                              if (mouse_over_point(cart_to_scrn(bezier_ctrl_pts[i])))
                                                      mouse_drag = i + 1;
                                      }
                                }
                                break;
                        case SDL_MOUSEBUTTONUP:
                                mouse_drag = 0;
                                break;
                        case SDL_KEYDOWN: {
                                const uint8_t *keys = SDL_GetKeyboardState(NULL);
                                if (keys[SDL_SCANCODE_EQUALS]) GRID_SCALE += 10;
                                if (keys[SDL_SCANCODE_MINUS] && GRID_SCALE > 10) GRID_SCALE -= 10;

                                if (keys[SDL_SCANCODE_Z] && curve_res < MAX_CURVE_RES) curve_res++;
                                if (keys[SDL_SCANCODE_X] && curve_res > MIN_CURVE_RES) curve_res--;
                                break;
                                }
                        case SDL_QUIT: 
                                quit = 1;
                                break;
                        }
                }

                SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
                SDL_RenderClear(renderer);

                draw_grid();
                draw_ctrl_points();

                if (mouse_drag) {
                        move_point(&bezier_ctrl_pts[mouse_drag-1]);
                }

                // SDL_Point p = cart_to_scrn(v);
                // filledCircleRGBA(renderer, p.x, p.y, 4, 255, 255, 255, 255);

                draw_cubic_curve(bezier_ctrl_pts, curve_res);

                SDL_RenderPresent(renderer);
                SDL_Delay(REFRESH);
        }

        // Clean up
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();

        return 0;
}
