/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* A mouse handling wrapper to DirectInput */

extern int MouseX,MouseY,MouseB0,MouseB1,MouseB2;
extern long MouseStatus;

void SetMouseRange(int x1, int y1, int x2, int y2);
void CenterMouse();
BOOL InitDirectInput(HINSTANCE g_hinst, HWND hwnd, int resx, int resy);
void DeInitDirectInput();
void DirectInputSyncAcquire(BOOL fActive);
void DirectInputCritical();



