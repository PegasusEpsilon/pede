#define Button9 9 // Not enough buttons defined for my mouse...
#define SIDE_TOP_BIT 0
#define SIDE_RIGHT_BIT 1
#define SIDE_BOTTOM_BIT 2
#define SIDE_LEFT_BIT 3
#define SIDE_TOP_MASK (1 << SIDE_TOP_BIT)
#define SIDE_RIGHT_MASK (1 << SIDE_RIGHT_BIT)
#define SIDE_BOTTOM_MASK (1 << SIDE_BOTTOM_BIT)
#define SIDE_LEFT_MASK (1 << SIDE_LEFT_BIT)
#define SIDE_CENTER_MASK 0xF
#define SIDE_TOP(x) (SIDE_TOP_MASK & x)
#define SIDE_RIGHT(x) (SIDE_RIGHT_MASK & x)
#define SIDE_BOTTOM(x) (SIDE_BOTTOM_MASK & x)
#define SIDE_LEFT(x) (SIDE_LEFT_MASK & x)
#define SIDE_CENTER(x) (SIDE_CENTER_MASK == x)
