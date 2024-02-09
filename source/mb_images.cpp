#include "mb_images.h"

static const char * const heart =
    "000,255,000,255,000\n"
    "255,255,255,255,255\n"
    "255,255,255,255,255\n"
    "000,255,255,255,000\n"
    "000,000,255,000,000\n";
const MicroBitImage IMG_HEART(heart);

static const char * const happy =
    "000,000,000,000,000\n"
    "000,255,000,255,000\n"
    "000,000,000,000,000\n"
    "255,000,000,000,255\n"
    "000,255,255,255,000\n";
const MicroBitImage IMG_HAPPY(happy);

static const char * const diamond =
    "000,000,255,000,000\n"
    "000,255,000,255,000\n"
    "255,000,000,000,255\n"
    "000,255,000,255,000\n"
    "000,000,255,000,000\n";
const MicroBitImage IMG_DIAMOND(diamond);

static const char * const square =
    "000,000,000,000,000\n"
    "000,255,255,255,000\n"
    "000,255,000,255,000\n"
    "000,255,255,255,000\n"
    "000,000,000,000,000\n";
const MicroBitImage IMG_SQUARE(square);

static const char * const stairs =
    "000,000,000,000,255\n"
    "000,000,255,255,255\n"
    "000,000,255,000,000\n"
    "255,255,255,000,000\n"
    "255,000,000,000,000\n";
const MicroBitImage IMG_STAIRS(stairs);

static const char * const dot =
    "000,000,000,000,000\n"
    "000,000,000,000,000\n"
    "000,000,255,000,000\n"
    "000,000,000,000,000\n"
    "000,000,000,000,000\n";
const MicroBitImage IMG_DOT(dot);

static const char * const empty =
    "000,000,000,000,000\n"
    "000,000,000,000,000\n"
    "000,000,000,000,000\n"
    "000,000,000,000,000\n"
    "000,000,000,000,000\n";
const MicroBitImage IMG_EMPTY(empty);

static const char * const full =
    "255,255,255,255,255\n"
    "255,255,255,255,255\n"
    "255,255,255,255,255\n"
    "255,255,255,255,255\n"
    "255,255,255,255,255\n";
const MicroBitImage IMG_FULL(full);
