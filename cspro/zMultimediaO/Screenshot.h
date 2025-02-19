#pragma once

#include <zMultimediaO/zMultimediaO.h>
#include <zMultimediaO/BmpFile.h>


// Returns the contents of the clipboard as a screenshot bitmap, throwing an exception if the clipboard does not contain an image.
ZMULTIMEDIAO_API Multimedia::BmpFile GetClipboardAsScreenshot();
