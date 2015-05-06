#!/bin/bash
# create multiresolution windows icon
ICON_DST=../../src/qt/res/icons/XCurrency.ico

convert ../../src/qt/res/icons/XCurrency-16.png ../../src/qt/res/icons/XCurrency-32.png ../../src/qt/res/icons/XCurrency-48.png ${ICON_DST}
