/* Copyright 2009 INRIA, Université Bordeaux 1  */

#include <topology.h>

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <windows.h>

#include "lstopo.h"

/* windows back-end.  */

static struct color {
  int r, g, b;
  HGDIOBJ brush;
} *colors;

static int numcolors;

static HGDIOBJ
rgb_to_brush(int r, int g, int b)
{
  int i;

  for (i = 0; i < numcolors; i++)
    if (colors[i].r == r && colors[i].g == g && colors[i].b == b)
      return colors[i].brush;

  fprintf(stderr, "color #%02x%02x%02x not declared\n", r, g, b);
  exit(EXIT_FAILURE);
}

struct draw_methods windows_draw_methods;

topo_topology_t the_topology;

static LRESULT CALLBACK
WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
  switch (message) {
    case WM_PAINT: {
      HDC hdc;
      PAINTSTRUCT ps;
      hdc = BeginPaint(hwnd, &ps);
      output_draw(&windows_draw_methods, the_topology, &ps);
      EndPaint(hwnd, &ps);
      break;
    }
    case WM_DESTROY:
      PostQuitMessage(0);
      return 0;
  }
  return DefWindowProc(hwnd, message, wparam, lparam);
}

static void *
windows_start(void *output_, int width, int height)
{
  WNDCLASS wndclass = {
    .hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH),
    .hCursor = LoadCursor(NULL, IDC_ARROW),
    .hIcon = LoadIcon(NULL, IDI_APPLICATION),
    .lpfnWndProc = WndProc,
    .lpszClassName = "lstopo",
  };
  HWND toplevel;

  RegisterClass(&wndclass);
  toplevel = CreateWindow("lstopo", "lstopo", WS_OVERLAPPEDWINDOW,
		  CW_USEDEFAULT, CW_USEDEFAULT,
		  width + 2*GetSystemMetrics(SM_CXSIZEFRAME),
		  height + 2*GetSystemMetrics(SM_CYSIZEFRAME) + GetSystemMetrics(SM_CYSMCAPTION),
		  NULL, NULL, NULL, NULL);

  ShowWindow(toplevel, SW_SHOWDEFAULT);

  return toplevel;
}

static void
windows_declare_color(void *output_, int r, int g, int b)
{
  HBRUSH brush;
  COLORREF color = RGB(r, g, b);

  brush = CreateSolidBrush(color);
  if (!brush) {
    fprintf(stderr,"Could not allocate color %02x%02x%02x\n", r, g, b);
    exit(EXIT_FAILURE);
  }

  colors = realloc(colors, sizeof(*colors) * (numcolors + 1));
  colors[numcolors].r = r;
  colors[numcolors].g = g;
  colors[numcolors].b = b;
  colors[numcolors].brush = (HGDIOBJ) brush;
  numcolors++;
}

static void
windows_box(void *output, int r, int g, int b, unsigned depth, unsigned x, unsigned width, unsigned y, unsigned height)
{
  PAINTSTRUCT *ps = output;
  SelectObject(ps->hdc, rgb_to_brush(r, g, b));
  SetBkColor(ps->hdc, RGB(r, g, b));
  Rectangle(ps->hdc, x, y, x + width, y + height);
}

static void
windows_text(void *output, int r, int g, int b, int size, unsigned depth, unsigned x, unsigned y, const char *text)
{
  PAINTSTRUCT *ps = output;
  HFONT font;
  SetTextColor(ps->hdc, RGB(r, g, b));
  font = CreateFont(size, 0, 0, 0, 0, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, NULL);
  SelectObject(ps->hdc, (HGDIOBJ) font);
  TextOut(ps->hdc, x, y, text, strlen(text));
  DeleteObject(font);
}

struct draw_methods windows_draw_methods = {
  .start = windows_start,
  .declare_color = windows_declare_color,
  .box = windows_box,
  .text = windows_text,
};

void
output_windows (topo_topology_t topology, FILE *output, int verbose_mode)
{
  HWND toplevel;
  the_topology = topology;
  toplevel = output_draw_start(&windows_draw_methods, topology, output);
  UpdateWindow(toplevel);
  MSG msg;
  while (GetMessage(&msg, NULL, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
}
