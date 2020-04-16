	XSync(display, False); printf("line %d request #%d\n", __LINE__, XNextRequest(display)); fflush(stdout);
